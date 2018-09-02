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
#include "needles.h"

void needle_pos_to_text(char text[static 32], unsigned int needle_pos) {
	if (needle_pos < 100) {
		snprintf(text, 32, "Yellow %d", 100 - needle_pos);
	} else if (needle_pos < 200) {
		snprintf(text, 32, "Green %d", needle_pos - 99);
	} else {
		snprintf(text, 32, "Unknown %d", needle_pos);
	}
}

int needle_text_to_pos(char letter, unsigned int number) {
	if ((letter == 'y') || (letter == 'Y')) {
		return 100 - number;
	} else if ((letter == 'g') || (letter == 'G')) {
		return 99 + number;
	} else {
		return -1;
	}
}

bool sled_before_needle_id(int sled_position, int needle_id, bool belt_phase, bool left_to_right) {
	int offset_left, offset_right;

	/* 8 - 24 is the full cycle. 16 - 20 is the minimum that still works.  We
	 * use 15 - 21 to be safe. TODO: 15-21 might lose stitches, try more safety
	 * margin! */
	const int safety = 4;
	offset_left = 16 - safety;
	offset_right = 20 + safety;

	if (!left_to_right) {
		int tmp = offset_left;
		offset_left = -offset_right;
		offset_right = -tmp;
	}

	int window_left = needle_id + offset_left;
	int window_right = needle_id + offset_right;
	//printf("For needle id %3d BP %d %s: [ %3d - %3d ] window size %d\n", needle_id, belt_phase, left_to_right ? "->" : "<-", window_left, window_right, window_right - window_left);
	return (sled_position >= window_left) && (sled_position < window_right);
}

void actuate_solenoids_for_needle(uint8_t *spi_data, bool belt_phase, unsigned int needle_id) {
	int bit = needle_id % 16;
	if (belt_phase) {
		bit = (bit + 8) % 16;
	}
	spi_data[bit / 8] |= (1 << (bit % 8));
}
