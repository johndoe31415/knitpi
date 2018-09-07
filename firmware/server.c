/*
 *	knitpi - Raspberry Pi interface for Brother KH-930 knitting machine
 *	Copyright (C) 2018-2018 Johannes Bauer
 *
 *	This file is part of knitpi.
 *
 *	knitpi is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; this program is ONLY licensed under
 *	version 3 of the License, later versions are explicitly excluded.
 *
 *	knitpi is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with knitpi; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	Johannes Bauer <JohannesBauer@gmx.de>
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <strings.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <stdarg.h>
#include "sled.h"
#include "json.h"
#include "knitcore.h"
#include "membuf.h"
#include "atomic.h"
#include "logging.h"
#include "server.h"
#include "tools.h"
#include "pgmopts.h"
#include "tokenizer.h"
#include "png_reader.h"
#include "png_writer.h"
#include "isleep.h"

#define MAX_CMD_ARG_COUNT		8

enum execution_state_t {
	SUCCESS,
	SILENT_FAILED,		/* Recoverable: Keep connection going and do not log */
	FAILED,				/* Recoverable: Keep connection going, but log */
	FATAL_ERROR,		/* Non-recoverable, disconnect. */
};

enum cmdtype_t {
	PLAIN_COMMAND = 0,
	RECV_BINDATA_COMMAND = 1,
	SEND_BINDATA_COMMAND = 2,
};

struct client_thread_data_t {
	FILE *f;
	unsigned int client_id;
	struct server_state_t *server_state;
};

typedef bool (*argument_parser_fnc)(union token_t *token);
typedef enum execution_state_t (*handler_fnc)(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);

struct argument_t {
	const char *name;
	argument_parser_fnc parser;
};

struct command_t {
	const char *cmdname;
	unsigned int arg_count;
	handler_fnc handler;
	struct argument_t arguments[MAX_CMD_ARG_COUNT];
	enum cmdtype_t cmd_type;
};

static bool argument_parse_int(union token_t *token) {
	int result;
	if (safe_atoi(token->string, &result)) {
		token->integer = result;
		return true;
	} else {
		return false;
	}
}

static bool argument_parse_bool(union token_t *token) {
	if ((!strcasecmp(token->string, "1")) || (!strcasecmp(token->string, "on")) || (!strcasecmp(token->string, "true")) || (!strcasecmp(token->string, "yes"))) {
		token->boolean = true;
		return true;
	} else if ((!strcasecmp(token->string, "0")) || (!strcasecmp(token->string, "off")) || (!strcasecmp(token->string, "false")) || (!strcasecmp(token->string, "no"))) {
		token->boolean = false;
		return true;
	}
	return false;
}

static enum execution_state_t handler_status(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_statuswait(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_setpattern(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_getpattern(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_editpattern(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_setrow(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_setoffset(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_setknitmode(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_setrepeatmode(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_hwmock(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);

static bool determine_movement_direction(struct client_thread_data_t *worker, bool send_error_response);

static struct command_t known_commands[] = {
	{
		.cmdname = "status",
		.handler = handler_status,
	},
	{
		.cmdname = "statuswait",
		.handler = handler_statuswait,
		.arg_count = 1,
		.arguments = {
			{ .name = "timeout_millisecs/int", .parser = argument_parse_int },
		},
	},
	{
		.cmdname = "setpattern",
		.handler = handler_setpattern,
		.cmd_type = RECV_BINDATA_COMMAND,
		.arg_count = 4,
		.arguments = {
			{ .name = "offsetx/int", .parser = argument_parse_int },
			{ .name = "offsety/int", .parser = argument_parse_int },
			{ .name = "merge/bool", .parser = argument_parse_bool },
			{ .name = "bindata_length/int", .parser = argument_parse_int },
		},
	},
	{
		.cmdname = "getpattern",
		.handler = handler_getpattern,
		.cmd_type = SEND_BINDATA_COMMAND,
		.arg_count = 1,
		.arguments = {
			{ .name = "rawdata/bool", .parser = argument_parse_bool },
		},
	},
	{
		.cmdname = "editpattern",
		.handler = handler_editpattern,
		.arg_count = 1,
		.arguments = {
			{ .name = "[clr|trim|center]/str" },
		},
	},
	{
		.cmdname = "setrow",
		.handler = handler_setrow,
		.arg_count = 1,
		.arguments = {
			{ .name = "rowid/int", .parser = argument_parse_int },
		},
	},
	{
		.cmdname = "setoffset",
		.handler = handler_setoffset,
		.arg_count = 1,
		.arguments = {
			{ .name = "offset/int", .parser = argument_parse_int },
		},
	},
	{
		.cmdname = "setknitmode",
		.handler = handler_setknitmode,
		.arg_count = 1,
		.arguments = {
			{ .name = "[on|off]/str" },
		},
	},
	{
		.cmdname = "setrepeatmode",
		.handler = handler_setrepeatmode,
		.arg_count = 1,
		.arguments = {
			{ .name = "[oneshot|repeat|manual]/str" },
		},
	},
	{
		.cmdname = "hwmock",
		.handler = handler_hwmock,
		.arg_count = 2,
		.arguments = {
			{ .name = "[setpos]/str" },
			{ .name = "parameter/int", .parser = argument_parse_int },
		},
	},
};
#define KNOWN_COMMAND_COUNT		(sizeof(known_commands) / sizeof(struct command_t))

static const char *knitting_mode_to_str(enum knitting_mode_t mode) {
	switch (mode) {
		case MODE_OFF:	return "off";
		case MODE_ON:	return "on";
	}
	return "unknown";
}

static const char *repeat_mode_to_str(enum repeat_mode_t mode) {
	switch (mode) {
		case RPTMODE_ONESHOT:	return "oneshot";
		case RPTMODE_REPEAT:	return "repeat";
		case RPTMODE_MANUAL:	return "manual";
	}
	return "unknown";
}

static enum execution_state_t handler_status(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	struct json_dict_entry_t json_dict[] = {
		JSON_DICTENTRY_STR("msg_type", "status"),
		JSON_DICTENTRY_STR("knitting_mode", knitting_mode_to_str(worker->server_state->knitting_mode)),
		JSON_DICTENTRY_STR("repeat_mode", repeat_mode_to_str(worker->server_state->repeat_mode)),
		JSON_DICTENTRY_BOOL("carriage_position_valid", worker->server_state->carriage_position_valid),
		JSON_DICTENTRY_BOOL("even_rows_left_to_right", worker->server_state->even_rows_left_to_right),
		JSON_DICTENTRY_INT("carriage_position", worker->server_state->carriage_position),
		JSON_DICTENTRY_INT("skipped_needles_cnt", sled_get_skipped_needles_cnt()),
		JSON_DICTENTRY_INT("pattern_row", worker->server_state->pattern_row),
		JSON_DICTENTRY_INT("pattern_offset", worker->server_state->pattern_offset),
		JSON_DICTENTRY_INT("pattern_min_x", worker->server_state->pattern ? worker->server_state->pattern->min_x : 0),
		JSON_DICTENTRY_INT("pattern_min_y", worker->server_state->pattern ? worker->server_state->pattern->min_y : 0),
		JSON_DICTENTRY_INT("pattern_max_x", worker->server_state->pattern ? worker->server_state->pattern->max_x : -1),
		JSON_DICTENTRY_INT("pattern_max_y", worker->server_state->pattern ? worker->server_state->pattern->max_y : -1),
		JSON_DICTENTRY_INT("pattern_width", worker->server_state->pattern ? worker->server_state->pattern->width : 0),
		JSON_DICTENTRY_INT("pattern_height", worker->server_state->pattern ? worker->server_state->pattern->height : 0),
		{ 0 },
	};
	json_print_dict(worker->f, json_dict);
	return SUCCESS;
}

static enum execution_state_t handler_statuswait(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	if (tokens->token[1].integer > 0) {
		isleep(&worker->server_state->event_notification, tokens->token[1].integer);
	}
	return handler_status(worker, tokens, membuf);
}

static void center_pattern(struct client_thread_data_t *worker) {
	int actual_width = worker->server_state->pattern->max_x - worker->server_state->pattern->min_x + 1;
	if (actual_width > 0) {
		worker->server_state->pattern_offset = (200 / 2) - (actual_width / 2);
	} else {
		worker->server_state->pattern_offset = 0;
	}
}

static enum execution_state_t handler_setpattern(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	int offsetx = tokens->token[1].integer;
	int offsety = tokens->token[2].integer;
	bool merge = tokens->token[3].boolean;
	struct pattern_t *pattern = png_read_pattern(membuf, offsetx, offsety);
	if (!pattern) {
		logmsg(LLVL_ERROR, "(%d) Failed read PNG image from client.", worker->client_id);
		json_respond_simple(worker->f, "error", "Could not read PNG image.");
		return FAILED;
	} else {
		if (!merge) {
			pattern_update_min_max(pattern);
			pattern_free(worker->server_state->pattern);
			worker->server_state->pattern = pattern;
		} else {
			struct pattern_t *merge_pattern = pattern_merge(worker->server_state->pattern, pattern);
			if (!merge_pattern) {
				logmsg(LLVL_WARN, "(%d) Failed merge patterns.", worker->client_id);
				json_respond_simple(worker->f, "error", "Could not merge patterns.");
				pattern_free(pattern);
				return FAILED;
			}
			pattern_update_min_max(merge_pattern);

			pattern_free(pattern);
			pattern_free(worker->server_state->pattern);
			worker->server_state->pattern = merge_pattern;
		}
	}

	worker->server_state->knitting_mode = MODE_OFF;
	center_pattern(worker);
	sled_update(worker->server_state);
	isleep_interrupt(&worker->server_state->event_notification);
	json_respond_simple(worker->f, "ok", "New pattern set.");
	return SUCCESS;
}

static enum execution_state_t handler_getpattern(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	bool rawdata = tokens->token[1].boolean;
	if (!worker->server_state->pattern) {
		json_respond_simple(worker->f, "error", "No pattern is set.");
		return SILENT_FAILED;
	}
	const struct png_write_options_t raw_write_options = {
		.pixel_width = 1,
		.pixel_height = 1,
		.grid_width = 0,
		.color_scheme = COLSCHEME_RAW,
	};
	if (!png_write_pattern_mem(worker->server_state->pattern, membuf, rawdata ? &raw_write_options : NULL)) {
		json_respond_simple(worker->f, "error", "Unable to convert pattern to PNG.");
		return FAILED;
	}
	return SUCCESS;
}

static enum execution_state_t handler_editpattern(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	if (!strcasecmp(tokens->token[1].string, "clr")) {
		pattern_free(worker->server_state->pattern);
		worker->server_state->pattern = NULL;
		worker->server_state->pattern_offset = 0;
	} else if (!strcasecmp(tokens->token[1].string, "trim")) {
		if (!worker->server_state->pattern) {
			json_respond_simple(worker->f, "error", "Cannot trim without pattern.");
			return FAILED;
		}
		logmsg(LLVL_DEBUG, "(%d) Trimming %d x %d pattern, old minmax (%d, %d), (%d, %d)", worker->client_id, worker->server_state->pattern->width, worker->server_state->pattern->height, worker->server_state->pattern->min_x, worker->server_state->pattern->min_y, worker->server_state->pattern->max_x, worker->server_state->pattern->max_y);
		struct pattern_t *trimmed = pattern_trim(worker->server_state->pattern);
		if (!trimmed) {
			json_respond_simple(worker->f, "error", "Trimming of pattern failed.");
			return FAILED;
		}
		pattern_free(worker->server_state->pattern);
		worker->server_state->pattern = trimmed;
		logmsg(LLVL_DEBUG, "(%d) Trimmed %d x %d pattern, new minmax (%d, %d), (%d, %d)", worker->client_id, worker->server_state->pattern->width, worker->server_state->pattern->height, worker->server_state->pattern->min_x, worker->server_state->pattern->min_y, worker->server_state->pattern->max_x, worker->server_state->pattern->max_y);
		if (worker->server_state->pattern_row >= worker->server_state->pattern->height) {
			worker->server_state->pattern_row = worker->server_state->pattern->height - 1;
		}
	} else if (!strcasecmp(tokens->token[1].string, "center")) {
		if (!worker->server_state->pattern) {
			json_respond_simple(worker->f, "error", "Cannot center without pattern.");
			return FAILED;
		}
		center_pattern(worker);
		logmsg(LLVL_DEBUG, "(%d) Centered %d x %d pattern with minmax (%d, %d), (%d, %d) to %d", worker->client_id, worker->server_state->pattern->width, worker->server_state->pattern->height, worker->server_state->pattern->min_x, worker->server_state->pattern->min_y, worker->server_state->pattern->max_x, worker->server_state->pattern->max_y, worker->server_state->pattern_offset);
	} else {
		json_respond_simple(worker->f, "error", "Invalid choice: %s", tokens->token[1].string);
		return FAILED;
	}
	sled_update(worker->server_state);
	isleep_interrupt(&worker->server_state->event_notification);
	json_respond_simple(worker->f, "ok", "Pattern edited.");
	return SUCCESS;
}

static enum execution_state_t handler_setrow(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	if ((worker->server_state->pattern) && (tokens->token[1].integer >= 0) && (tokens->token[1].integer < worker->server_state->pattern->height)) {
		int current_row = worker->server_state->pattern_row;
		worker->server_state->pattern_row = tokens->token[1].integer;
		if (determine_movement_direction(worker, true)) {
			sled_update(worker->server_state);
			isleep_interrupt(&worker->server_state->event_notification);
			json_respond_simple(worker->f, "ok", "New row set.");
			return SUCCESS;
		} else {
			worker->server_state->pattern_row = current_row;
			return FAILED;
		}
	} else {
		json_respond_simple(worker->f, "error", "No pattern set or given index %d out of bounds for pattern.", tokens->token[1].integer);
		return FAILED;
	}
}

static enum execution_state_t handler_setoffset(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	worker->server_state->pattern_offset = tokens->token[1].integer;
	sled_update(worker->server_state);
	isleep_interrupt(&worker->server_state->event_notification);
	json_respond_simple(worker->f, "ok", "New offset set to %d.", worker->server_state->pattern_offset);
	return SUCCESS;
}

static bool determine_movement_direction(struct client_thread_data_t *worker, bool send_error_response) {
	if (!worker->server_state->pattern) {
		logmsg(LLVL_WARN, "(%d) Cannot determine movement direction without a pattern set.", worker->client_id);
		if (send_error_response) {
			json_respond_simple(worker->f, "error", "Cannot determine movement direction without a pattern set.");
		}
		return false;
	}

	if (!worker->server_state->carriage_position_valid) {
		logmsg(LLVL_WARN, "(%d) Cannot determine movement direction without a valid carriage position.", worker->client_id);
		if (send_error_response) {
			json_respond_simple(worker->f, "error", "Cannot determine movement direction without a valid carriage position.");
		}
		return false;
	}

	if (worker->server_state->carriage_position <= worker->server_state->pattern->min_x + worker->server_state->pattern_offset) {
		/* We're left of pattern */
		worker->server_state->even_rows_left_to_right = ((worker->server_state->pattern_row % 2) == 0);
	} else if (worker->server_state->carriage_position >= worker->server_state->pattern->max_x + worker->server_state->pattern_offset) {
		/* We're right of pattern */
		worker->server_state->even_rows_left_to_right = ((worker->server_state->pattern_row % 2) != 0);
	} else {
		logmsg(LLVL_ERROR, "(%d) Cannot determine movement direction at this carriage postion: carriage at %d and pattern from %d to %d.", worker->client_id, worker->server_state->carriage_position, worker->server_state->pattern->min_x + worker->server_state->pattern_offset, worker->server_state->pattern->max_x + worker->server_state->pattern_offset);
		if (send_error_response) {
			json_respond_simple(worker->f, "error", "Cannot determine movement direction at this carriage postion: carriage at %d and pattern from %d to %d.", worker->server_state->carriage_position, worker->server_state->pattern->min_x + worker->server_state->pattern_offset, worker->server_state->pattern->max_x + worker->server_state->pattern_offset);
		}
		return false;
	}

	return true;
}

static enum execution_state_t handler_setknitmode(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	if (!strcasecmp(tokens->token[1].string, "on")) {
		if (determine_movement_direction(worker, true)) {
			worker->server_state->knitting_mode = MODE_ON;
		} else {
			return FAILED;
		}
	} else if (!strcasecmp(tokens->token[1].string, "off")) {
		worker->server_state->knitting_mode = MODE_OFF;
	} else {
		json_respond_simple(worker->f, "error", "Invalid choice: %s", tokens->token[1].string);
		return FAILED;
	}
	sled_update(worker->server_state);
	isleep_interrupt(&worker->server_state->event_notification);
	json_respond_simple(worker->f, "ok", "New knitting mode: %s", knitting_mode_to_str(worker->server_state->knitting_mode));
	return SUCCESS;
}

static enum execution_state_t handler_setrepeatmode(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	if (!strcasecmp(tokens->token[1].string, "oneshot")) {
		worker->server_state->repeat_mode = RPTMODE_ONESHOT;
	} else if (!strcasecmp(tokens->token[1].string, "repeat")) {
		worker->server_state->repeat_mode = RPTMODE_REPEAT;
	} else if (!strcasecmp(tokens->token[1].string, "manual")) {
		worker->server_state->repeat_mode = RPTMODE_MANUAL;
	} else {
		json_respond_simple(worker->f, "error", "Invalid choice: %s", tokens->token[1].string);
		return FAILED;
	}
	sled_update(worker->server_state);
	isleep_interrupt(&worker->server_state->event_notification);
	json_respond_simple(worker->f, "ok", "New repeat mode: %s", repeat_mode_to_str(worker->server_state->repeat_mode));
	return SUCCESS;
}

static enum execution_state_t handler_hwmock(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	if (!pgm_opts->no_hardware) {
		json_respond_simple(worker->f, "error", "Hardware mock commands are disallowed when actual hardware is used.");
		return FAILED;
	}
	if (!strcasecmp(tokens->token[1].string, "setpos")) {
		worker->server_state->carriage_position_valid = true;
		worker->server_state->carriage_position = tokens->token[2].integer;
		json_respond_simple(worker->f, "ok", "Set position.");
	} else {
		json_respond_simple(worker->f, "error", "Invalid choice: %s", tokens->token[1].string);
		return FAILED;
	}
	sled_update(worker->server_state);
	isleep_interrupt(&worker->server_state->event_notification);
	return SUCCESS;
}


static enum execution_state_t parse_execute_command(struct client_thread_data_t *worker, char *line) {
	enum execution_state_t result = SUCCESS;
	trim_crlf(line);

	struct tokens_t* tokens = tok_create(line);
	if (!tokens) {
		logmsg(LLVL_ERROR, "(%d) Failed to tokenize client input: %s", worker->client_id, strerror(errno));
		json_respond_simple(worker->f, "error", "Failed to tokenize client command.");
		return FATAL_ERROR;
	}

	if (tokens->token_cnt == 0) {
		json_respond_simple(worker->f, "error", "No commands given.");
		result = FAILED;
	} else {
		const char *command_name = tokens->token[0].string;
		struct command_t *command = NULL;
		for (int i = 0; i < KNOWN_COMMAND_COUNT; i++) {
			if (!strcmp(known_commands[i].cmdname, command_name)) {
				command = &known_commands[i];
				break;
			}
		}
		if (!command) {
			logmsg(LLVL_WARN, "(%d) Client issued unknown command: %s", worker->client_id, command_name);
			json_respond_simple(worker->f, "error", "No such command: %s", command_name);
			result = FAILED;
		} else {
			if (tokens->token_cnt - 1 == command->arg_count) {
				/* Argument count matches, try to parse them all */
				for (int i = 0; i < command->arg_count; i++) {
					if (command->arguments[i].parser) {
						bool parse_result = command->arguments[i].parser(&tokens->token[i + 1]);
						if (!parse_result) {
							logmsg(LLVL_WARN, "(%d) Could not parse client's argument %s for command %s: %s", worker->client_id, command->arguments[i].name, command_name, tokens->token[i + 1].string);
							json_respond_simple(worker->f, "error", "Could not parse argument %s of command %s.", command->arguments[i].name, command_name);
							result = FAILED;
							break;
						}
					}
				}
				if (result == SUCCESS) {
					/* All arguments could be successfully parsed. Execute! */
					struct membuf_t membuf = MEMBUF_INITIALIZER;
					if (command->cmd_type == RECV_BINDATA_COMMAND) {
						/* The last argument must always be the amount of
						 * binary data we read */
						int bindata_length = tokens->token[command->arg_count].integer;
						if ((bindata_length < 0) || (bindata_length > pgm_opts->max_bindata_recv_bytes)) {
							logmsg(LLVL_ERROR, "(%d) Tried sending %d bytes of binary data, maximum of %d bytes allowed.", worker->client_id, bindata_length, pgm_opts->max_bindata_recv_bytes);
							json_respond_simple(worker->f, "error", "Binary data length %d is invalid. Maximum of %d bytes is permissible.", bindata_length, pgm_opts->max_bindata_recv_bytes);
							result = FATAL_ERROR;
						} else {
							/* Try to allocate data buffer */
							if (membuf_resize(&membuf, bindata_length)) {
								/* Then try reading from client */
								logmsg(LLVL_TRACE, "(%d) Receiving %d bytes of binary data.", worker->client_id, bindata_length);
								if (fread(membuf.data, bindata_length, 1, worker->f) != 1) {
									logmsg(LLVL_WARN, "(%d) Short read while receiving %d bytes of binary data.", worker->client_id, bindata_length);
									json_respond_simple(worker->f, "error", "Short read while receiving binary data.");
									result = FATAL_ERROR;
								}
							} else {
								logmsg(LLVL_WARN, "(%d) Failed to allocate %d bytes of binary data membuf: %s", worker->client_id, bindata_length, strerror(errno));
								json_respond_simple(worker->f, "error", "Failed ot allocate binary data buffer for reception of data.");
								result = FATAL_ERROR;
							}
						}
					}
					if (result == SUCCESS) {
						/* Only execute if the binary read was successful */
						result = command->handler(worker, tokens, &membuf);
					}
					if ((command->cmd_type == SEND_BINDATA_COMMAND) && (result == SUCCESS)) {
						/* We first send the JSON header, then the binary data */
						struct json_dict_entry_t json_dict[] = {
							JSON_DICTENTRY_STR("msg_type", "bindata"),
							JSON_DICTENTRY_INT("bindata_length", membuf.length),
							{ 0 },
						};
						json_print_dict(worker->f, json_dict);

						/* Then send the binary data */
						if (fwrite(membuf.data, membuf.length, 1, worker->f) != 1) {
							logmsg(LLVL_ERROR, "(%d) Short write when trying to send %d bytes to client.", worker->client_id, membuf.length);
							json_respond_simple(worker->f, "error", "Short write when trying to send %d bytes to you.", membuf.length);
							result = FATAL_ERROR;
						}
					}
					membuf_free(&membuf);
				}
			} else {
				char usage[256];
				char *buf = usage;
				*buf = 0;
				int bufsize = sizeof(usage);
				for (int i = 0; i < command->arg_count; i++) {
					int charcnt;
					charcnt = snprintf(buf, bufsize, " [%s]", command->arguments[i].name);
					buf += charcnt;
					bufsize -= charcnt;
				}
				logmsg(LLVL_WARN, "(%d) Client issued wrong argument count for command: %s, supplied %d, required %d. Usage: %s%s", worker->client_id, command_name, tokens->token_cnt - 1, command->arg_count, command_name, usage);
				json_respond_simple(worker->f, "error", "The %s command requires %d arguments, you provided %d. Usage: %s%s", command_name, command->arg_count, tokens->token_cnt - 1, command_name, usage);
				result = FAILED;
			}
		}
	}
	tok_free(tokens);
	return result;
}

static void* client_handler(void *vclient_thread_data) {
	struct client_thread_data_t *worker = (struct client_thread_data_t*)vclient_thread_data;
	while (true) {
		char line[256];
		if (!fgets(line, sizeof(line), worker->f)) {
			break;
		}

		enum execution_state_t result = parse_execute_command(worker, line);
		fflush(worker->f);
		if (result == FATAL_ERROR) {
			logmsg(LLVL_ERROR, "(%d) Execution returned fatal error, severing connection to client.", worker->client_id);
			break;
		} else if (result == FAILED) {
			logmsg(LLVL_WARN, "(%d) Error executing client command.", worker->client_id);
		}
	}

	fclose(worker->f);
	logmsg(LLVL_DEBUG, "(%d) Client disconnected.", worker->client_id);
	struct atomic_ctr_t *thread_cnt = &worker->server_state->thread_count;
	free(worker);
	atomic_dec(thread_cnt);
	return NULL;
}

bool start_server(struct server_state_t *server_state) {
	int sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return false;
	}

	if (!ignore_signal(SIGPIPE)) {
		logmsg(LLVL_FATAL, "Could not ignore SIGPIPE: %s", strerror(errno));
		return false;
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, pgm_opts->unix_socket, sizeof(addr.sun_path) - 1);
	if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("bind");
		close(sd);
		return false;
	}

	if (listen(sd, 10) == -1) {
		perror("listen");
		close(sd);
		return false;
	}

	logmsg(LLVL_INFO, "Server started, waiting for clients.");
	unsigned int client_id = 0;
	while (true) {
		struct sockaddr_un peer_addr;
		socklen_t addrlen = sizeof(peer_addr);
		logmsg(LLVL_TRACE, "Accepting connection.");
		int fd = accept(sd, (struct sockaddr*)&peer_addr, &addrlen);
		if (fd == -1) {
			logmsg(LLVL_FATAL, "Could not accept() client connection: %s", strerror(errno));
			close(sd);
			return false;
		}
		logmsg(LLVL_DEBUG, "(%u) Client connected.", client_id);


		struct client_thread_data_t *thread_data = calloc(1, sizeof(struct client_thread_data_t));
		if (!thread_data) {
			logmsg(LLVL_FATAL, "(%u) Could not allocate client thread data memory: %s", client_id, strerror(errno));
			close(fd);
			close(sd);
			return false;
		}
		thread_data->client_id = client_id++;
		thread_data->server_state = server_state;

		thread_data->f = fdopen(fd, "r+");
		if (!thread_data->f) {
			free(thread_data);
			logmsg(LLVL_FATAL, "(%u) Could not fdopen() client file descriptor: %s", client_id, strerror(errno));
			close(fd);
			close(sd);
			return false;
		}

		if (!start_detached_thread(client_handler, thread_data)) {
			logmsg(LLVL_FATAL, "(%u) Could not start client handler thread: %s", client_id, strerror(errno));
			close(fd);
			close(sd);
			return false;
		}
		atomic_inc(&server_state->thread_count);
		if (pgm_opts->quit_after_single_connection) {
			logmsg(LLVL_INFO, "(%u) Shutting down.", client_id);
			break;
		}
	}

	/* Wait for all threads to finish up */
	logmsg(LLVL_INFO, "Waiting for remaining %d threads to finish.", server_state->thread_count.ctr);
	atomic_wait(&server_state->thread_count, 0);
	return true;
}
