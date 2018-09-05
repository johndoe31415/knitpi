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

#define MAX_PATTERN_WIDTH	400
#define MAX_PATTERN_HEIGHT	1000

#define UINT8(x)						((x) & 0xff)
#define MK_RGBA(r, g, b, a)				((UINT8(a) << 24) | (UINT8(b) << 16) | (UINT8(g) << 8) | (UINT8(r) << 0))
#define MK_RGB(r, g, b)					MK_RGBA((r), (g), (b), 0xff)

#define PIXEL_GET_RGB(pixel)			((pixel) & 0xffffff)
#define PIXEL_GET_ALPHA(pixel)			UINT8((pixel) >> 24)

#define PIXEL_FULLY_TRANSPARENT			MK_RGBA(0xff, 0xff, 0xff, 0)
#define PIXEL_COLOR_IS_WHITE(pixel)		(PIXEL_GET_RGB(pixel) == 0xffffff)
#define PIXEL_COLOR_IS_BLACK(pixel)		(PIXEL_GET_RGB(pixel) == 0)

struct pattern_t {
	unsigned int width, height;
	uint8_t *pixel_data;
	unsigned int used_colors;
	uint32_t rgb_palette[255];
	unsigned int min_x, max_x;
	unsigned int min_y, max_y;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
struct pattern_t* pattern_new(unsigned int width, unsigned int height);
void pattern_set_rgba(struct pattern_t *pattern, unsigned int x, unsigned int y, uint32_t rgba);
uint8_t* pattern_row_rw(const struct pattern_t *pattern, unsigned int y);
const uint8_t* pattern_row(const struct pattern_t *pattern, unsigned int y);
void pattern_dump_row(const struct pattern_t *pattern, unsigned int y);
void pattern_update_min_max(struct pattern_t *pattern);
struct pattern_t* pattern_merge(const struct pattern_t *old_pattern, const struct pattern_t *new_pattern);
struct pattern_t* pattern_trim(struct pattern_t *pattern);
void pattern_dump(const struct pattern_t *pattern);
void pattern_free(struct pattern_t *pattern);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
