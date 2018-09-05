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
#include "knitserver.h"
#include "membuf.h"
#include "atomic.h"
#include "logging.h"
#include "server.h"
#include "tools.h"
#include "pgmopts.h"
#include "tokenizer.h"
#include "png_reader.h"
#include "png_writer.h"

#define MAX_CMD_ARG_COUNT		8

enum execution_state_t {
	SUCCESS,
	FAILED,				/* Recoverable: Keep connection going */
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
static enum execution_state_t handler_setpattern(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_getpattern(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_setrow(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_setknitmode(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);
static enum execution_state_t handler_setrepeatmode(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf);

static struct command_t known_commands[] = {
	{
		.cmdname = "status",
		.handler = handler_status,
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
		.cmdname = "setrow",
		.handler = handler_setrow,
		.arg_count = 1,
		.arguments = {
			{ .name = "rowid/int", .parser = argument_parse_int },
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
			{ .name = "[oneshot|repeat]/str" },
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
	}
	return "unknown";
}

static enum execution_state_t handler_status(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	struct json_dict_entry_t json_dict[] = {
		JSON_DICTENTRY_STR("msg_type", "status"),
		JSON_DICTENTRY_STR("knitting_mode", knitting_mode_to_str(worker->server_state->knitting_mode)),
		JSON_DICTENTRY_STR("repeat_mode", repeat_mode_to_str(worker->server_state->repeat_mode)),
		JSON_DICTENTRY_BOOL("carriage_position_valid", worker->server_state->carriage_position_valid),
		JSON_DICTENTRY_BOOL("belt_phase", worker->server_state->belt_phase),
		JSON_DICTENTRY_BOOL("direction_left_to_right", worker->server_state->direction_left_to_right),
		JSON_DICTENTRY_INT("carriage_position", worker->server_state->carriage_position),
		JSON_DICTENTRY_INT("skipped_needles_cnt", sled_get_skipped_needles_cnt()),
		JSON_DICTENTRY_INT("pattern_row", worker->server_state->pattern_row),
		JSON_DICTENTRY_INT("pattern_width", worker->server_state->pattern ? worker->server_state->pattern->width : 0),
		JSON_DICTENTRY_INT("pattern_height", worker->server_state->pattern ? worker->server_state->pattern->height : 0),
		{ 0 },
	};
	json_print_dict(worker->f, json_dict);
	return SUCCESS;
}

static enum execution_state_t handler_setpattern(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	/* TODO implement offsets */
//	int offsetx = tokens->token[1].integer;
//	int offsety = tokens->token[2].integer;
	bool merge = tokens->token[3].boolean;
	struct pattern_t *pattern = png_read_pattern(membuf);
	if (!pattern) {
		return FAILED;
	} else {
		if (!merge) {
			pattern_free(worker->server_state->pattern);
			worker->server_state->pattern = pattern;
		} else {
			// TODO implement
		}
	}
	json_respond_simple(worker->f, "ok", "New pattern set.");
	return SUCCESS;
}

static enum execution_state_t handler_getpattern(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	bool rawdata = tokens->token[1].boolean;
	if (!worker->server_state->pattern) {
		json_respond_simple(worker->f, "error", "No pattern is set.");
		return FAILED;
	}
	if (!png_write_pattern_mem(worker->server_state->pattern, membuf, rawdata ? NULL : NULL)) {	// TODO implement raw pattern
		json_respond_simple(worker->f, "error", "Unable to convert pattern to PNG.");
		return FAILED;
	}
	return SUCCESS;
}

static enum execution_state_t handler_setrow(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	if ((worker->server_state->pattern) && (tokens->token[1].integer >= 0) && (tokens->token[1].integer < worker->server_state->pattern->height)) {
		worker->server_state->pattern_row = tokens->token[1].integer;
		json_respond_simple(worker->f, "ok", "New row set.");
		return SUCCESS;
	} else {
		json_respond_simple(worker->f, "error", "No pattern set or given index %d out of bounds for pattern.", tokens->token[1].integer);
		return FAILED;
	}
}

static enum execution_state_t handler_setknitmode(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	if (!strcasecmp(tokens->token[1].string, "on")) {
		worker->server_state->knitting_mode = MODE_ON;
	} else if (!strcasecmp(tokens->token[1].string, "off")) {
		worker->server_state->knitting_mode = MODE_OFF;
	} else {
		json_respond_simple(worker->f, "error", "Invalid choice: %s", tokens->token[1].string);
		return FAILED;
	}
	json_respond_simple(worker->f, "ok", "New knitting mode: %s", knitting_mode_to_str(worker->server_state->knitting_mode));
	return SUCCESS;
}

static enum execution_state_t handler_setrepeatmode(struct client_thread_data_t *worker, struct tokens_t* tokens, struct membuf_t *membuf) {
	if (!strcasecmp(tokens->token[1].string, "oneshot")) {
		worker->server_state->repeat_mode = RPTMODE_ONESHOT;
	} else if (!strcasecmp(tokens->token[1].string, "repeat")) {
		worker->server_state->repeat_mode = RPTMODE_REPEAT;
	} else {
		json_respond_simple(worker->f, "error", "Invalid choice: %s", tokens->token[1].string);
		return FAILED;
	}
	json_respond_simple(worker->f, "ok", "New repeat mode: %s", repeat_mode_to_str(worker->server_state->repeat_mode));
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
