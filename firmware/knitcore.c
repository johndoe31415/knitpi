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

#include <stdint.h>
#include <stdbool.h>

#include "knitcore.h"
#include "peripherals.h"
#include "needles.h"
#include "pgmopts.h"

void set_knitting_mode(struct server_state_t *server_state, bool knitting_mode) {
	if (server_state->knitting_mode == knitting_mode) {
		return;
	}

	server_state->knitting_mode = knitting_mode;
	logmsg(LLVL_TRACE, "Knitting mode: %s", knitting_mode ? "enabled" : "disabled");
	if (!pgm_opts->no_hardware) {
		gpio_set_to(GPIO_LED_RED, knitting_mode);
		gpio_set_to(GPIO_74HC595_OE, knitting_mode);
	}
	isleep_interrupt(&server_state->event_notification);
}

static bool is_direction_left_to_right(const struct server_state_t *server_state) {
	return server_state->even_rows_left_to_right == ((server_state->pattern_row % 2) == 0);
}

void sled_update(struct server_state_t *server_state) {
	if (server_state->pattern == NULL) {
		set_knitting_mode(server_state, false);
		return;
	}

	if (!server_state->carriage_position_valid) {
		set_knitting_mode(server_state, false);
		return;
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
				set_knitting_mode(server_state, false);
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
	if (!server_state->knitting_mode) {
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
