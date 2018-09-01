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

static int sled_position = 0;
static uint8_t last_rotary = 0xff;
static bool pos_valid = false;
static unsigned int skip_needle_count = 0;

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
		skip_needle_count++;
	}
	last_rotary = pos;
}

void sled_input(enum gpio_t gpio, const struct timespec *ts, bool value) {
	bool execute_callback = false;


	switch (gpio) {
		case GPIO_BROTHER_LEFT_HALL:
			if (!value) {
//				printf("LEFT %+d\n", sled_position);
				sled_position = 0;
				pos_valid = true;
				execute_callback = true;
			}
			break;
		
		case GPIO_BROTHER_RIGHT_HALL:
			if (!value) {
//				printf("RIGHT %+d\n", 794 - sled_position);
				pos_valid = true;
				execute_callback = true;
				sled_position = 794;
			}
			break;

		case GPIO_BROTHER_V1:
		case GPIO_BROTHER_V2:
			{
				int sled_before = rotary_get_position();
				rotary_encoder_movement();
				int sled_after = rotary_get_position();
				execute_callback = pos_valid && (sled_before != sled_after);
			}
			break;

		default: break;
	}

	if (execute_callback) {
		int sled_pos = rotary_get_position();
		printf("SLED %d\n", sled_pos);
	}
}
