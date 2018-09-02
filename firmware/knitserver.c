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
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>

#include "knitserver.h"
#include "peripherals.h"
#include "gpio_thread.h"
#include "isleep.h"
#include "tools.h"
#include "debouncer.h"
#include "sled.h"
#include "needles.h"
#include "pnm_reader.h"
#include "argparse.h"

struct pgmopts_t {
	bool no_hardware;
	const char *unix_socket;
};
static struct pgmopts_t pgm_opts;


//static struct pnmfile_t* pnm_file;
//static int current_row = -1;
//static bool last_direction;
//static int stitch_count;

struct server_state_t {
	enum server_mode_t server_mode;
	bool carriage_position_valid;
	bool belt_phase;
	bool direction_left_to_right;
	int32_t carriage_position;
	uint32_t skipped_needle_cnt;
	int32_t pattern_row;
};
static struct server_state_t server_state;


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


static bool knitserver_pgmopts(enum argparse_option_t option, const char *value) {
	switch (option) {
		case ARG_NO_HARDWARE:
			pgm_opts.no_hardware = true;
			break;

		case ARG_UNIX_SOCKET:
			pgm_opts.unix_socket = value;
			break;
	}
	return true;
}

static struct msg_t* read_msg(FILE *f) {
	struct msg_t header;
	if (fread(&header, sizeof(header), 1, f) != 1) {
		perror("could not read header from peer");
		return NULL;
	}
	if (header.payload_size > MAX_SUPPORTED_PAYLOAD_SIZE) {
		fprintf(stderr, "Peer message indicated payload size %u bytes, maximum allowable limit is %u bytes.\n", header.payload_size, MAX_SUPPORTED_PAYLOAD_SIZE);
		return NULL;
	}

	unsigned int msg_size = sizeof(header) + header.payload_size;
	struct msg_t *response = malloc(msg_size);
	if (!response) {
		perror("malloc");
		return NULL;
	}
	response->cmdcode = header.cmdcode;
	response->payload_size = header.payload_size;
	if (response->payload_size && (fread(response->payload, response->payload_size, 1, f) != 1)) {
		perror("could not read data from peer");
		free(response);
		return NULL;
	}
	return response;
}

static bool respond_msg(FILE *f, const void *response_data, unsigned int response_length) {
	return fwrite(response_data, response_length, 1, f) == 1;
}

static bool handle_message(FILE *f, struct msg_t *msg) {
	if ((msg->cmdcode == CMD_GET_STATUS) && (msg->payload_size == 0)) {
		struct rsp_get_info_msg_t response = {
			.header = {
				.cmdcode = RSP_GET_STATUS,
				.payload_size = sizeof(response) - sizeof(response.header),
			},
			.server_mode = server_state.server_mode,
			.carriage_position_valid = server_state.carriage_position_valid,
			.belt_phase = server_state.belt_phase,
			.direction_left_to_right = server_state.direction_left_to_right,
			.carriage_position = server_state.carriage_position,
			.skipped_needle_cnt = server_state.skipped_needle_cnt,
		};
		return respond_msg(f, &response, sizeof(response));
	}
	return false;
}

static void* client_handler(void *vf) {
	FILE *f = (FILE*)vf;

	while (true) {
		struct msg_t *msg = read_msg(f);
		if (msg == NULL) {
			fprintf(stderr, "Could not read message from peer, closing connection.\n");
			break;
		}
		bool success = handle_message(f, msg);
		free(msg);
		if (!success) {
			fprintf(stderr, "Unexpected message received from peer, cmdcode 0x%x, closing connection.\n", msg->cmdcode);
			break;
		}
	}
	fclose(f);
	return NULL;
}

static bool start_server(void) {
	int sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
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

	while (true) {
		struct sockaddr_un peer_addr;
		socklen_t addrlen = sizeof(peer_addr);
		int fd = accept(sd, (struct sockaddr*)&peer_addr, &addrlen);
		if (fd == -1) {
			perror("accept");
			close(sd);
			return false;
		}

		FILE *f = fdopen(fd, "r+");
		if (!f) {
			perror("fdopen");
			close(fd);
			close(sd);
			return false;
		}

		if (!start_detached_thread(client_handler, f)) {
			fprintf(stderr, "Could not start client handler thread.\n");
			close(fd);
			close(sd);
			return false;
		}
	}
	return true;
}

int main(int argc, char **argv) {
	argparse_parse_or_die(argc, argv, knitserver_pgmopts);
	if (!pgm_opts.no_hardware) {
		if (!all_peripherals_init()) {
			fprintf(stderr, "Failed to initialize peripherals, bailing out.\n");
			exit(EXIT_FAILURE);
		}
		sled_set_callback(sled_actuation_callback);
		start_debouncer_thread(sled_input);
		start_gpio_thread(debouncer_input, true);
		spi_clear(SPI_74HC595, 2);
	}

	if (!start_server()) {
		fprintf(stderr, "Failed to start server.\n");
		exit(EXIT_FAILURE);
	}
	return 0;
}
