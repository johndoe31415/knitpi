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

static struct knitmachine_params_t knitmachine_params = {
	.solenoid_count = 16,
	.belt_phase_offset = 8,
	.needle_count = 200,
	.active_window_offset = 11,
	.active_window_size = 16,
};

const struct knitmachine_params_t *get_knitmachine_params(void) {
	return &knitmachine_params;
}

void modify_knitmachine_params(int new_window_offset, int new_window_size) {
	knitmachine_params.active_window_offset = new_window_offset;
	knitmachine_params.active_window_size = new_window_size;
}

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

void actuate_solenoids_for_needle(uint8_t *spi_data, bool belt_phase, unsigned int needle_id) {
	int bit = needle_id % 16;
	if (belt_phase) {
		bit = (bit + 8) % 16;
	}
	spi_data[bit / 8] |= (1 << (bit % 8));
}

struct needle_window_t get_needle_window_for_carriage_position(int carriage_position, bool left_to_right) {
	struct needle_window_t result;
	if (left_to_right) {
		result.min_needle = carriage_position - knitmachine_params.active_window_offset - knitmachine_params.active_window_size + 1;
		result.max_needle = carriage_position - knitmachine_params.active_window_offset;
	} else {
		result.min_needle = carriage_position + knitmachine_params.active_window_offset;
		result.max_needle = carriage_position + knitmachine_params.active_window_offset + knitmachine_params.active_window_size - 1;
	}
	return result;
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

	const struct knitmachine_params_t *params = get_knitmachine_params();
	printf("Hardware configuration: offset %d, window length %d\n", params->active_window_offset, params->active_window_size);

	char carriage_pos_str[32];
	needle_pos_to_text(carriage_pos_str, carriage_pos);
	printf("Carriage position: %s (%d), movement %s\n", carriage_pos_str, carriage_pos, left_to_right ? "->" : "<-");

	{
		struct needle_window_t window = get_needle_window_for_carriage_position(carriage_pos, left_to_right);
		char min_needle_str[32], max_needle_str[32];
		needle_pos_to_text(min_needle_str, window.min_needle);
		needle_pos_to_text(max_needle_str, window.max_needle);
		printf("Active needles: %s (%d) to %s (%d).\n", min_needle_str, window.min_needle, max_needle_str, window.max_needle);
	}

	printf("\n");
	printf("For actuating single needle %s (%d), actuating windows are:\n", carriage_pos_str, carriage_pos);
	printf("Left-to right:");
	for (int i = 0; i < knitmachine_params.needle_count; i++) {
		struct needle_window_t window = get_needle_window_for_carriage_position(i, true);
		if ((carriage_pos >= window.min_needle) && (carriage_pos <= window.max_needle)) {
			printf(" %3d", i);
		}
	}
	printf("\n");
	printf("Right-to-left:");
	for (int i = 0; i < knitmachine_params.needle_count; i++) {
		struct needle_window_t window = get_needle_window_for_carriage_position(i, false);
		if ((carriage_pos >= window.min_needle) && (carriage_pos <= window.max_needle)) {
			printf(" %3d", i);
		}
	}
	printf("\n");


	return 0;
}
#endif
