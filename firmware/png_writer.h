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

#ifndef __PNG_WRITER_H__
#define __PNG_WRITER_H__

#include <stdbool.h>
#include <stdint.h>
#include "pattern.h"

struct png_write_options_t {
	unsigned int pixel_width, pixel_height;
	unsigned int grid_width;
	uint32_t grid_color;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
bool png_write_pattern(const struct pattern_t *pattern, const char *filename, const struct png_write_options_t *options);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
