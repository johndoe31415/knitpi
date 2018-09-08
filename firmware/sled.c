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
#include <time.h>
#include <stdbool.h>
#include "peripherals_gpio.h"
#include "sled.h"
#include "logging.h"

static int sled_position = 0;
static uint8_t last_rotary = 0xff;
static bool pos_valid = false;
static unsigned int skipped_needles_cnt = 0;
static struct server_state_t *server_state;
static sled_callback_t sled_callback = NULL;
static int last_reported_position = 0xffff;
static bool belt_phase = false;

static const int fixed_position_left = 0;
static const int fixed_position_right = 794;

unsigned int sled_get_skipped_needles_cnt(void) {
	return skipped_needles_cnt;
}

static int rotary_get_position(void) {
	return sled_position / 4;
}

static void rotary_encoder_movement(void) {
	uint8_t pos = (gpio_get_last_value(GPIO_BROTHER_V1) ? 1 : 0) | (gpio_get_last_value(GPIO_BROTHER_V2) ? 2 : 0);
	if (pos == 3) {
		pos = 2;
	} else if (pos == 2) {
		pos = 3;
	}

	if (pos == ((last_rotary + 1) % 4)) {
		/* Movement to left */
		sled_position--;
	} else if (pos == ((last_rotary + 3) % 4)) {
		/* Movement to right */
		sled_position++;
	} else {
		skipped_needles_cnt++;
	}
	last_rotary = pos;
}

void sled_set_callback(struct server_state_t *new_server_state, sled_callback_t callback) {
	server_state = new_server_state;
	sled_callback = callback;
}

void sled_input(enum gpio_t gpio, const struct timespec *ts, bool value) {
	switch (gpio) {
		case GPIO_BROTHER_LEFT_HALL:
			if (!value) {
				belt_phase = gpio_get_last_value(GPIO_BROTHER_BP);
				logmsg(LLVL_DEBUG, "Left hall sensor triggered, previous rotary position %d, deviation %+d (%+d needles), belt_phase %d, new rotary position %d.", sled_position, sled_position - fixed_position_left, (sled_position - fixed_position_left + 2) / 4, belt_phase, fixed_position_left);
				sled_position = fixed_position_left;
				skipped_needles_cnt = 0;
				pos_valid = true;
			}
			break;

		case GPIO_BROTHER_RIGHT_HALL:
			if (!value) {
				belt_phase = !gpio_get_last_value(GPIO_BROTHER_BP);
				logmsg(LLVL_DEBUG, "Right hall sensor triggered, previous rotary position %d, deviation %+d (%+d needles), belt_phase %d, new rotary position %d.", sled_position, sled_position - fixed_position_right, (sled_position - fixed_position_right + 2) / 4, belt_phase, fixed_position_right);
				sled_position = fixed_position_right;
				skipped_needles_cnt = 0;
				pos_valid = true;
			}
			break;

		case GPIO_BROTHER_V1:
		case GPIO_BROTHER_V2:
			rotary_encoder_movement();
			break;

		default: break;
	}

	if (sled_callback) {
		int sled_pos = rotary_get_position();
		if ((sled_pos != last_reported_position) && pos_valid) {
			sled_callback(server_state, sled_pos, belt_phase);
			last_reported_position = sled_pos;
		}
	}
}
