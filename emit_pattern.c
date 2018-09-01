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
#include "peripherals.h"
#include "gpio_thread.h"
#include "isleep.h"
#include "tools.h"
#include "debouncer.h"
#include "sled.h"
#include "needles.h"
#include "pnm_reader.h"

static struct pnmfile_t* pnm_file;
static int current_row = -1;
static bool last_direction;
static int stitch_count;

static void sled_actuation_callback(int position, bool belt_phase, bool left_to_right) {
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
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "syntax: %s [pnm filename]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	pnm_file = pnmfile_read(argv[1]);
	if (!pnm_file) {
		fprintf(stderr, "Failed to read input file, bailing out.\n");
		exit(EXIT_FAILURE);
	}
	printf("Emitting pattern.\n");
	pnmfile_dump(pnm_file);
	if (!all_peripherals_init()) {
		fprintf(stderr, "Failed to initialize peripherals, bailing out.\n");
		exit(EXIT_FAILURE);
	}
	sled_set_callback(sled_actuation_callback);
	start_debouncer_thread(sled_input);
	start_gpio_thread(debouncer_input, true);
	spi_clear(SPI_74HC595, 2);
	gpio_active(GPIO_74HC595_OE);
	while (true) {
		char cmd[256];
		if (!fgets(cmd, sizeof(cmd), stdin)) {
			perror("fgets");
			exit(EXIT_FAILURE);
		}

		int len = strlen(cmd);
		if (len && (cmd[len - 1] == '\n')) {
			cmd[--len] = 0;
		}
		if (len && (cmd[len - 1] == '\r')) {
			cmd[--len] = 0;
		}

		if (cmd[0] == 'r') {
			int row_no = atoi(cmd + 1);
			current_row = row_no;
			fprintf(stderr, "Set new row: %d\n", current_row);
		} else {
			fprintf(stderr, "Unknown command.\n");
		}
	}
}
