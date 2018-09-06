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

#include "knitcore.h"
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

void sled_update(struct server_state_t *server_state) {
	if (pgm_opts->no_hardware) {
		return;
	}

	if (server_state->pattern == NULL) {
		gpio_inactive(GPIO_74HC595_OE);
		server_state->knitting_mode = MODE_OFF;
		return;
	}

	if (!server_state->carriage_position_valid) {
		gpio_inactive(GPIO_74HC595_OE);
		server_state->knitting_mode = MODE_OFF;
		return;
	}

	if (server_state->knitting_mode == MODE_ON) {
		gpio_active(GPIO_74HC595_OE);
	} else {
		gpio_inactive(GPIO_74HC595_OE);
	}



	uint8_t spi_data[] = { 0, 0 };
	if ((server_state->pattern_row >= 0) && (server_state->pattern_row < server_state->pattern->height)) {
		for (int knit_needle_id = 0; knit_needle_id < 200; knit_needle_id++) {
			uint8_t color = pattern_get_color(server_state->pattern, knit_needle_id - server_state->pattern_offset, server_state->pattern_row);
			if (color != 0) {
				if (sled_before_needle_id(server_state->carriage_position, knit_needle_id, server_state->belt_phase, server_state->direction_left_to_right)) {
					actuate_solenoids_for_needle(spi_data, server_state->belt_phase, knit_needle_id);
				}
			}
		}
	}
	spi_send(SPI_74HC595, spi_data, sizeof(spi_data));
}

static void next_row(struct server_state_t *server_state) {
	if (server_state->pattern_row + 1 < server_state->pattern->height) {
		server_state->pattern_row++;
	} else {
		server_state->pattern_row = 0;
		if (server_state->repeat_mode == RPTMODE_ONESHOT) {
			server_state->knitting_mode = MODE_OFF;
		} else {
			if ((server_state->pattern->height % 2) == 1) {
				server_state->even_rows_left_to_right = !server_state->even_rows_left_to_right;
			}
		}
	}
}

static void check_for_next_row(struct server_state_t *server_state) {
	if (!server_state->pattern) {
		return;
	}
	if (server_state->knitting_mode != MODE_ON) {
		return;
	}

	bool is_even_row = (server_state->pattern_row % 2) == 0;
	if (server_state->even_rows_left_to_right == is_even_row) {
		/* Waiting for position right of pattern */
		int rightmost_needle = server_state->pattern->max_x + server_state->pattern_offset + 5;
		if (server_state->carriage_position >= rightmost_needle) {
			next_row(server_state);
		}
	} else {
		int leftmost_needle = server_state->pattern->min_x + server_state->pattern_offset - 5;
		if (server_state->carriage_position <= leftmost_needle) {
			next_row(server_state);
		}
	}
}

void sled_actuation_callback(struct server_state_t *server_state, int position, bool belt_phase, bool direction_left_to_right) {
	server_state->carriage_position = position;
	server_state->belt_phase = belt_phase;
	if (server_state->carriage_position_valid) {
		check_for_next_row(server_state);
	}
	server_state->direction_left_to_right = direction_left_to_right;
	server_state->carriage_position_valid = true;

	sled_update(server_state);
	isleep_interrupt(&server_state->event_notification);


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
