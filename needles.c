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
