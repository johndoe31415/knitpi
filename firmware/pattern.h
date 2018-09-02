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

#ifndef __PATTERN_H__
#define __PATTERN_H__

struct pattern_t {
	unsigned int width, height;
	uint8_t *pixel_data;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
struct pattern_t* pattern_new(unsigned int width, unsigned int height);
uint8_t* pattern_row_rw(const struct pattern_t *pattern, unsigned int y);
const uint8_t* pattern_row(const struct pattern_t *pattern, unsigned int y);
void pattern_dump_row(const struct pattern_t *pattern, unsigned int y);
void pattern_dump(const struct pattern_t *pattern);
void pattern_free(struct pattern_t *pattern);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
