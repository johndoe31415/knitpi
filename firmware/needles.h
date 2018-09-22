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

#ifndef __NEEDLES_H__
#define __NEEDLES_H__

#include <stdbool.h>
#include <stdint.h>

struct knitmachine_params_t {
	unsigned int solenoid_count;
	int belt_phase_offset;
	unsigned int needle_count;
	unsigned int active_window_offset;
	unsigned int active_window_size;
};

struct needle_window_t {
	int min_needle;
	int max_needle;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
void needle_pos_to_text(char text[static 32], unsigned int needle_pos);
int needle_text_to_pos(char letter, unsigned int number);
bool sled_before_needle_id(int sled_position, int needle_id, bool belt_phase, bool left_to_right);
void actuate_solenoids_for_needle(uint8_t *spi_data, bool belt_phase, unsigned int needle_id);
struct needle_window_t get_needle_window_for_carriage_position(int carriage_position, bool left_to_right);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
