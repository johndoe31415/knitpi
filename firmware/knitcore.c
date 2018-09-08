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

static void deactivate_solenoids(void) {
	if (pgm_opts->no_hardware) {
		return;
	}
	gpio_inactive(GPIO_74HC595_OE);
}

static void activate_solenoids(void) {
	if (pgm_opts->no_hardware) {
		return;
	}
	gpio_active(GPIO_74HC595_OE);
}

static bool is_direction_left_to_right(const struct server_state_t *server_state) {
	return server_state->even_rows_left_to_right == ((server_state->pattern_row % 2) == 0);
}

void sled_update(struct server_state_t *server_state) {
	if (server_state->pattern == NULL) {
		deactivate_solenoids();
		server_state->knitting_mode = MODE_OFF;
		return;
	}

	if (!server_state->carriage_position_valid) {
		deactivate_solenoids();
		server_state->knitting_mode = MODE_OFF;
		return;
	}

	if (server_state->knitting_mode == MODE_ON) {
		activate_solenoids();
	} else {
		deactivate_solenoids();
	}



	uint8_t spi_data[] = { 0, 0 };
	if ((server_state->pattern_row >= 0) && (server_state->pattern_row < server_state->pattern->height)) {
		bool direction_left_to_right = is_direction_left_to_right(server_state);
		for (int knit_needle_id = 0; knit_needle_id < 200; knit_needle_id++) {
			uint8_t color = pattern_get_color(server_state->pattern, knit_needle_id - server_state->pattern_offset, server_state->pattern_row);
			if (color != 0) {

				if (sled_before_needle_id(server_state->carriage_position, knit_needle_id, server_state->belt_phase, direction_left_to_right)) {
					char needle_name[32];
					needle_pos_to_text(needle_name, knit_needle_id);
					logmsg(LLVL_TRACE, "At %d actuating %s", server_state->carriage_position, needle_name);
					actuate_solenoids_for_needle(spi_data, server_state->belt_phase, knit_needle_id);
				}
			}
		}
	}
	if (!pgm_opts->no_hardware) {
		spi_send(SPI_74HC595, spi_data, sizeof(spi_data));
	}
}

static void next_row(struct server_state_t *server_state) {
	if (server_state->repeat_mode != RPTMODE_MANUAL) {
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
	} else {
		/* Do not advance row, but inverse direction in manual mode */
		server_state->even_rows_left_to_right = !server_state->even_rows_left_to_right;
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
		int rightmost_needle = server_state->pattern->max_x + server_state->pattern_offset + 32;
		if (server_state->carriage_position >= rightmost_needle) {
			next_row(server_state);
		}
	} else {
		int leftmost_needle = server_state->pattern->min_x + server_state->pattern_offset - 32;
		if (server_state->carriage_position <= leftmost_needle) {
			next_row(server_state);
		}
	}
}

void sled_actuation_callback(struct server_state_t *server_state, int position, bool belt_phase) {
	server_state->carriage_position = position;
	server_state->belt_phase = belt_phase;
	if (server_state->carriage_position_valid) {
		check_for_next_row(server_state);
	}
	server_state->carriage_position_valid = true;

	sled_update(server_state);
	isleep_interrupt(&server_state->event_notification);
}
