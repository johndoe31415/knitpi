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
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <signal.h>

#include "knitserver.h"
#include "peripherals.h"
#include "gpio_thread.h"
#include "isleep.h"
#include "tools.h"
#include "debouncer.h"
#include "sled.h"
#include "needles.h"
#include "pnm_reader.h"
#include "png_writer.h"
#include "png_reader.h"
#include "argparse.h"
#include "json.h"
#include "logging.h"
#include "atomic.h"

struct pgmopts_t {
	bool quit_after_single_connection;
	bool force;
	bool no_hardware;
	enum loglvl_t loglevel;
	const char *unix_socket;
};
static struct pgmopts_t pgm_opts = {
	.loglevel = LLVL_ERROR,
};

struct server_state_t {
	enum server_mode_t server_mode;
	bool carriage_position_valid;
	bool belt_phase;
	bool direction_left_to_right;
	int32_t carriage_position;
	int32_t pattern_row;
	struct pattern_t *pattern;
};
static struct server_state_t server_state;
static struct atomic_ctr_t thread_count = ATOMIC_CTR_INITIALIZER(0);
static void sled_actuation_callback(int position, bool belt_phase, bool direction_left_to_right) {
	server_state.carriage_position_valid = true;
	server_state.carriage_position = position;
	server_state.belt_phase = belt_phase;
	server_state.direction_left_to_right = direction_left_to_right;

#if 0

	if (last_direction != left_to_right) {
		/* Direction reversed. */
		if (stitch_count > 30) {
			current_row++;
			stitch_count = 0;
			printf("Next row: %d\n", current_row);
			if (current_row < pnm_file->height) {
				pnmfile_dump_row(pnm_file, current_row);
			} else {
				printf("======================================================================================================\n");
			}
		}
	}

	uint8_t spi_data[] = { 0, 0 };
	if ((current_row >= 0) && (current_row < pnm_file->height)) {
		stitch_count++;
		const uint8_t *row_data = pnmfile_row(pnm_file, current_row);


		for (int knit_needle_id = 0; knit_needle_id < 200; knit_needle_id++) {
			if (row_data[knit_needle_id]) {
				if (sled_before_needle_id(position, knit_needle_id, belt_phase, left_to_right)) {
					actuate_solenoids_for_needle(spi_data, belt_phase, knit_needle_id);
				}
			}
		}
	} else if ((position == 0) && (current_row == -1)) {
		current_row = 0;
		printf("Starting pattern.\n");
		pnmfile_dump_row(pnm_file, current_row);
	}

	int active_solenoid_cnt = 0;
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 8; j++) {
			if ((spi_data[i] >> j) & 1) {
				active_solenoid_cnt++;
			}
		}
	}
	printf("Serving row %d for %d (%d stitches, %d active solenoids)\n", current_row, position, stitch_count, active_solenoid_cnt);
	spi_send(SPI_74HC595, spi_data, sizeof(spi_data));
	last_direction = left_to_right;
#endif
}

static const char *server_mode_to_str(enum server_mode_t mode) {
	switch (mode) {
		case MODE_OFFLINE:	return "MODE_OFFLINE";
		case MODE_ONLINE:	return "MODE_ONLINE";
	}
	return "?";
}

static bool knitserver_pgmopts(enum argparse_option_t option, const char *value) {
	switch (option) {
		case ARG_NO_HARDWARE:
			pgm_opts.no_hardware = true;
			break;

		case ARG_FORCE:
			pgm_opts.force = true;
			break;

		case ARG_VERBOSE:
			pgm_opts.loglevel++;
			break;

		case ARG_UNIX_SOCKET:
			pgm_opts.unix_socket = value;
			break;

		case ARG_QUIT:
			pgm_opts.quit_after_single_connection = true;
			break;
	}
	return true;
}

static void json_respond_simple(FILE *f, const char *msg_type, const char *msg) {
	fprintf(f, "{");
	json_printf_str(f, "message", msg);
	fprintf(f, "\"msg_type\": \"%s\"}\n", msg_type);
}

static void discard_data(FILE *f, unsigned int discard_bytes) {
	while (discard_bytes) {
		uint8_t buffer[16 * 1024];
		unsigned int chunk_size = (discard_bytes > sizeof(buffer)) ? sizeof(buffer) : discard_bytes;
		size_t discarded = fread(buffer, 1, chunk_size, f);
		if (discarded <= 0) {
			break;
		}
		discard_bytes -= chunk_size;
	}
}

static void* client_handler(void *vf) {
	FILE *f = (FILE*)vf;

	while (true) {
		char line[256];
		if (!fgets(line, sizeof(line), f)) {
			break;
		}
		int len = strlen(line);
		if (len && (line[len - 1] == '\n')) {
			line[--len] = 0;
		}
		if (len && (line[len - 1] == '\r')) {
			line[--len] = 0;
		}

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
	logmsg(LLVL_DEBUG, "Client disconnected.");
	atomic_dec(&thread_count);
	return NULL;
}

static bool start_server(void) {
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
	strncpy(addr.sun_path, pgm_opts.unix_socket, sizeof(addr.sun_path) - 1);
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
		logmsg(LLVL_DEBUG, "Client connected.");

		FILE *f = fdopen(fd, "r+");
		if (!f) {
			logmsg(LLVL_FATAL, "Could not fdopen() client file descriptor: %s", strerror(errno));
			close(fd);
			close(sd);
			return false;
		}

		if (!start_detached_thread(client_handler, f)) {
			logmsg(LLVL_FATAL, "Could not start client handler thread: %s", strerror(errno));
			close(fd);
			close(sd);
			return false;
		}
		atomic_inc(&thread_count);
		if (pgm_opts.quit_after_single_connection) {
			logmsg(LLVL_INFO, "Shutting down.");
			break;
		}
	}

	/* Wait for all threads to finish up */
	atomic_wait(&thread_count, 0);
	return true;
}

int main(int argc, char **argv) {
	argparse_parse_or_die(argc, argv, knitserver_pgmopts);
	set_loglevel(pgm_opts.loglevel);
	if (!pgm_opts.no_hardware) {
		if (!all_peripherals_init()) {
			logmsg(LLVL_FATAL, "Failed to initialize hardware peripherals.");
			exit(EXIT_FAILURE);
		}
		sled_set_callback(sled_actuation_callback);
		start_debouncer_thread(sled_input);
		start_gpio_thread(debouncer_input, true);
		spi_clear(SPI_74HC595, 2);
	}

	if (pgm_opts.force) {
		unlink(pgm_opts.unix_socket);
	}

	if (!start_server()) {
		logmsg(LLVL_FATAL, "Failed to start server.");
		exit(EXIT_FAILURE);
	}
	pattern_free(server_state.pattern);
	return 0;
}
