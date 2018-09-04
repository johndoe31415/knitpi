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
#include "json.h"
#include "logging.h"
#include "atomic.h"
#include "server.h"
#include "pgmopts.h"


static struct server_state_t server_state = {
	.thread_count = ATOMIC_CTR_INITIALIZER(0),
};

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

int main(int argc, char **argv) {
	parse_pgmopts(argc, argv);
	set_loglevel(pgm_opts->loglevel);
	if (!pgm_opts->no_hardware) {
		if (!all_peripherals_init()) {
			logmsg(LLVL_FATAL, "Failed to initialize hardware peripherals.");
			exit(EXIT_FAILURE);
		}
		sled_set_callback(sled_actuation_callback);
		start_debouncer_thread(sled_input);
		start_gpio_thread(debouncer_input, true);
		spi_clear(SPI_74HC595, 2);
	}

	if (pgm_opts->force) {
		unlink(pgm_opts->unix_socket);
	}

	if (!start_server(&server_state)) {
		logmsg(LLVL_FATAL, "Failed to start server.");
		exit(EXIT_FAILURE);
	}
	pattern_free(server_state.pattern);
	return 0;
}
