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

static const struct knitmachine_params_t knitmachine_params = {
	.solenoid_count = 16,
	.belt_phase_offset = 8,
	.needle_count = 200,
	.active_window_offset = 12,
	.active_window_size = 12,
};

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

int min_active_needle_for_carriage_position(int carriage_position, bool left_to_right) {
	if (left_to_right) {
		return carriage_position - knitmachine_params.active_window_offset - knitmachine_params.active_window_size + 1;
	} else {
		return carriage_position + knitmachine_params.active_window_offset + 1;
	}
}

int max_active_needle_for_carriage_position(int carriage_position, bool left_to_right) {
	if (left_to_right) {
		return carriage_position - knitmachine_params.active_window_offset;
	} else {
		return carriage_position + knitmachine_params.active_window_offset + knitmachine_params.active_window_size;
	}
}

#ifdef __DEBUG_NEEDLES__
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "%s [carriage pos] [left-to-right]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	int carriage_pos = atoi(argv[1]);
	bool left_to_right = atoi(argv[2]);

	char carriage_pos_str[32];
	needle_pos_to_text(carriage_pos_str, carriage_pos);
	printf("Carriage position: %s (%d), movement %s\n", carriage_pos_str, carriage_pos, left_to_right ? "->" : "<-");

	int min_active_needle = min_active_needle_for_carriage_position(carriage_pos, left_to_right);
	int max_active_needle = max_active_needle_for_carriage_position(carriage_pos, left_to_right);

	char min_needle_str[32], max_needle_str[32];
	needle_pos_to_text(min_needle_str, min_active_needle);
	needle_pos_to_text(max_needle_str, max_active_needle);
	printf("Active needles: %s (%d) to %s (%d).\n", min_needle_str, min_active_needle, max_needle_str, max_active_needle);

	printf("\n");
	printf("For actuating single needle %s (%d), actuating windows follow:\n", carriage_pos_str, carriage_pos);
	printf("Left-to right:");
	for (int i = 0; i < knitmachine_params.needle_count; i++) {
		int min_active = min_active_needle_for_carriage_position(i, true);
		int max_active = max_active_needle_for_carriage_position(i, true);
		if ((carriage_pos >= min_active) && (carriage_pos <= max_active)) {
			printf(" %3d", i);
		}
	}
	printf("\n");
	printf("Right-to-left:");
	for (int i = 0; i < knitmachine_params.needle_count; i++) {
		int min_active = min_active_needle_for_carriage_position(i, false);
		int max_active = max_active_needle_for_carriage_position(i, false);
		if ((carriage_pos >= min_active) && (carriage_pos <= max_active)) {
			printf(" %3d", i);
		}
	}
	printf("\n");


	return 0;
}
#endif
