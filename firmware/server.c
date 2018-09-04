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

#define MAX_CMD_ARG_COUNT		8

enum execution_state_t {
	SUCCESS,
	FAILED,				/* Recoverable: Keep connection going */
	FATAL_ERROR,		/* Non-recoverable, disconnect. */
};

struct client_thread_data_t {
	FILE *f;
	unsigned int client_id;
	struct server_state_t *server_state;
};

typedef bool (*argument_parser_fnc)(union token_t *token);
typedef enum execution_state_t (*handler_fnc)(struct client_thread_data_t *worker, struct tokens_t* tokens);

struct argument_t {
	const char *name;
	argument_parser_fnc parser;
};

struct command_t {
	const char *cmdname;
	unsigned int arg_count;
	handler_fnc handler;
	struct argument_t arguments[MAX_CMD_ARG_COUNT];
};

static bool argument_parse_int(union token_t *token) {

	return true;
}

static enum execution_state_t handler_status(struct client_thread_data_t *worker, struct tokens_t* tokens);
static enum execution_state_t handler_setfoo(struct client_thread_data_t *worker, struct tokens_t* tokens);

static struct command_t known_commands[] = {
	{
		.cmdname = "status",
		.handler = handler_status,
	},
	{
		.cmdname = "setfoo",
		.arg_count = 3,
		.arguments = {
			{ .name = "arg1" },
			{ .name = "arg2" },
			{ .name = "intarg3", .parser = argument_parse_int },
		},
		.handler = handler_setfoo,
	},
};
#define KNOWN_COMMAND_COUNT		(sizeof(known_commands) / sizeof(struct command_t))

static const char *server_mode_to_str(enum server_mode_t mode) {
	switch (mode) {
		case MODE_OFFLINE:	return "MODE_OFFLINE";
		case MODE_ONLINE:	return "MODE_ONLINE";
	}
	return "Unknown";
}

static enum execution_state_t handler_status(struct client_thread_data_t *worker, struct tokens_t* tokens) {
	struct json_dict_entry_t json_dict[] = {
		JSON_DICTENTRY_STR("msg_type", "status"),
		JSON_DICTENTRY_STR("server_mode", server_mode_to_str(worker->server_state->server_mode)),
		JSON_DICTENTRY_BOOL("carriage_position_valid", worker->server_state->carriage_position_valid),
		JSON_DICTENTRY_BOOL("belt_phase", worker->server_state->belt_phase),
		JSON_DICTENTRY_BOOL("direction_left_to_right", worker->server_state->direction_left_to_right),
		JSON_DICTENTRY_INT("carriage_position", worker->server_state->carriage_position),
		JSON_DICTENTRY_INT("skipped_needles_cnt", sled_get_skipped_needles_cnt()),
		JSON_DICTENTRY_INT("pattern_row", worker->server_state->pattern_row),
		{ 0 },
	};
	json_print_dict(worker->f, json_dict);
	return SUCCESS;
}

static enum execution_state_t handler_setfoo(struct client_thread_data_t *worker, struct tokens_t* tokens) {
	return SUCCESS;
}

static void json_respond_simple(FILE *f, const char *msg_type, const char *message, ...) {
	char message_buffer[256];
	va_list ap;
	va_start(ap, message);
	vsnprintf(message_buffer, sizeof(message_buffer), message, ap);
	va_end(ap);

	struct json_dict_entry_t json_dict[] = {
		JSON_DICTENTRY_STR("msg_type", msg_type),
		JSON_DICTENTRY_STR("message", message_buffer),
		{ 0 },
	};
	json_print_dict(f, json_dict);
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
							logmsg(LLVL_WARN, "(%d) Could not parse client's argument %s for command %s: %s", worker->client_id, command->arguments[i].name, command_name, tokens->token[i + 1].string);
							json_respond_simple(worker->f, "error", "Could not parse argument %s of command %s.", command->arguments[i].name, command_name);
							result = FAILED;
							break;
						}
					}
				}
				if (result == SUCCESS) {
					/* All arguments could be successfully parsed. Execute! */
					result = command->handler(worker, tokens);
				}
			} else {
				logmsg(LLVL_WARN, "(%d) Client issued wrong argument count for command: %s, supplied %d, required %d", worker->client_id, command_name, tokens->token_cnt - 1, command->arg_count);
				json_respond_simple(worker->f, "error", "The %s command requires %d arguments, you provided %d.", command_name, command->arg_count, tokens->token_cnt - 1);
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

#if 0
	FILE *f = (FILE*)vf;


		char *saveptr;
		char *token = strtok_r(line, " ", &saveptr);
		if (token == NULL) {
			logmsg(LLVL_ERROR, "Could not tokenize client command.");
			json_respond_simple(f, "error", "Could not tokenize command.");
		} else if (!strcmp(token, "status")) {
			logmsg(LLVL_DEBUG, "<- %s", token);
			fprintf(f, "{");
			json_print_str(f, "server_mode", server_mode_to_str(server_state.server_mode));
			json_print_bool(f, "carriage_position_valid", server_state.carriage_position_valid);
			json_print_bool(f, "belt_phase", server_state.belt_phase);
			json_print_bool(f, "direction_left_to_right", server_state.direction_left_to_right);
			json_print_int(f, "carriage_position", server_state.carriage_position);
			json_print_int(f, "skipped_needles_cnt", sled_get_skipped_needles_cnt());
			json_print_int(f, "pattern_row", server_state.pattern_row);
			fprintf(f, "\"msg_type\": \"status\"}\n");
		} else if (!strcmp(token, "getpattern")) {
			logmsg(LLVL_DEBUG, "<- %s", token);

			struct membuf_t png_file = MEMBUF_INITIALIZER;
			if (server_state.pattern) {
				bool success = png_write_pattern_mem(server_state.pattern, &png_file, NULL);
				if (!success) {
					png_file.length = 0;
				}
			}

			fprintf(f, "{");
			json_print_int(f, "length_bytes", png_file.length);
			fprintf(f, "\"msg_type\": \"pattern\"}\n");
			if (png_file.length) {
				if (fwrite(png_file.data, png_file.length, 1, f) != 1) {
					logmsg(LLVL_ERROR, "Short write of binary data when sending pattern.");
				}
			}
			membuf_free(&png_file);
		} else if (!strcmp(token, "setpattern")) {
			logmsg(LLVL_DEBUG, "<- %s", token);
			int xoffset = 0, yoffset = 0, binlength = 0;
			token = strtok_r(NULL, " ", &saveptr);
			if (token) {
				xoffset = atoi(token);
			}
			token = strtok_r(NULL, " ", &saveptr);
			if (token) {
				yoffset = atoi(token);
			}
			token = strtok_r(NULL, " ", &saveptr);
			if (token) {
				binlength = atoi(token);
				if (binlength <= MAX_PNG_RECV_SIZE) {
					struct membuf_t png_data = MEMBUF_INITIALIZER;
					if (!membuf_resize(&png_data, binlength)) {
						logmsg(LLVL_FATAL, "Could not resize membuf to %d bytes: %s", binlength, strerror(errno));

					} else {
						if (fread(png_data.data, binlength, 1, f) != 1) {
							logmsg(LLVL_ERROR, "Short read of binary data when receiving pattern of %d bytes.", binlength);
							json_respond_simple(f, "error", "Short read of binary data when receiving pattern.");
							membuf_free(&png_data);
							break;
						} else {
							logmsg(LLVL_DEBUG, "Setting pattern at %d, %d PNG length %d bytes.", xoffset, yoffset, binlength);
							struct pattern_t* new_pattern = png_read_pattern(&png_data);
							if (new_pattern) {
								pattern_free(server_state.pattern);
								server_state.pattern = new_pattern;
								json_respond_simple(f, "success", "Input pattern read.");
							} else {
								json_respond_simple(f, "error", "Failed to decode PNG file.");
							}
						}
					}
					membuf_free(&png_data);
				} else {
					logmsg(LLVL_WARN, "Discarding %d bytes of input, too large for PNG buffer.", binlength);
					discard_data(f, binlength);
					json_respond_simple(f, "error", "Input file too large");
				}
			} else {
				json_respond_simple(f, "error", "Not enough parameters supplied for command, expected: [xoffset] [yoffset] [bindata_length].");
			}
		} else {
			logmsg(LLVL_WARN, "Client sent unknown command: %s", token);
			fprintf(f, "{");
			json_printf_str(f, "error_msg", "Request (length %d bytes) not understood.", len);
			fprintf(f, "\"msg_type\": \"error\"}\n");
		}
	}
	fclose(f);
#endif
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
