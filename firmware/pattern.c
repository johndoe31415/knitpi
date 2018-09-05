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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include "pattern.h"
#include "logging.h"

struct pattern_t* pattern_new(unsigned int width, unsigned int height) {
	if ((width > MAX_PATTERN_WIDTH) || (height > MAX_PATTERN_HEIGHT)) {
		logmsg(LLVL_WARN, "Refusing to create %u x %u size pattern, maximum size is %u x %u.\n", width, height, MAX_PATTERN_WIDTH, MAX_PATTERN_HEIGHT);
		return NULL;
	}

	struct pattern_t *pattern = calloc(1, sizeof(struct pattern_t));
	if (!pattern) {
		perror("calloc pattern");
		return NULL;
	}

	pattern->width = width;
	pattern->height = height;
	pattern->pixel_data = calloc(1, pattern->width * pattern->height);
	pattern->min_x = width + 1;
	pattern->min_y = height + 1;
	if (!pattern->pixel_data) {
		fprintf(stderr, "Failed to allocate %d bytes for %d x %d pixel pattern: %s\n", pattern->width * pattern->height, pattern->width, pattern->height, strerror(errno));
		pattern_free(pattern);
		return NULL;
	}

	return pattern;
}

static uint8_t pattern_get_color(const struct pattern_t *pattern, unsigned int x, unsigned int y) {
	if ((x >= pattern->width) || (y >= pattern->height)) {
		return 0;
	}
	return pattern->pixel_data[(pattern->width * y) + x];
}

static void pattern_set_color(struct pattern_t *pattern, unsigned int x, unsigned int y, uint8_t color_index) {
	pattern->pixel_data[(pattern->width * y) + x] = color_index;
}

void pattern_set_rgba(struct pattern_t *pattern, unsigned int x, unsigned int y, uint32_t rgba) {
	uint8_t color_index;
	if ((PIXEL_COLOR_IS_WHITE(rgba)) || (PIXEL_GET_ALPHA(rgba) == 0)) {
		/* Completely white or completely transparent */
		color_index = 0;
	} else {
		bool found = false;
		uint32_t rgb = PIXEL_GET_RGB(rgba);
		for (int i = 0; i < pattern->used_colors; i++) {
			if (pattern->rgb_palette[i] == rgb) {
				color_index = i + 1;
				found = true;
				break;
			}
		}
		if (!found) {
			/* Color not found in palette, add if possible */
			if (pattern->used_colors < 255) {
				pattern->used_colors++;
				color_index = pattern->used_colors;
				pattern->rgb_palette[pattern->used_colors - 1] = rgb;
			} else {
				logmsg(LLVL_WARN, "Cannot add more than 255 colors to pattern.");
				color_index = 0;
			}
		}
	}
	pattern_set_color(pattern, x, y, color_index);
}


uint8_t* pattern_row_rw(const struct pattern_t *pattern, unsigned int y) {
	return pattern->pixel_data + (pattern->width * y);
}

const uint8_t* pattern_row(const struct pattern_t *pattern, unsigned int y) {
	return pattern_row_rw(pattern, y);
}

void pattern_dump_row(const struct pattern_t *pattern, unsigned int y) {
	const uint8_t *row = pattern_row(pattern, y);
	for (int x = 0; x < pattern->width; x++) {
		printf("%s", row[x] ? " " : "•");
	}
	printf("\n");
}

void pattern_update_min_max(struct pattern_t *pattern) {
	/* Need to increment minimum values by 1 to get correct result even with
	 * 0x0 sized pattern */
	pattern->min_x = pattern->width + 1;
	pattern->min_y = pattern->height + 1;
	pattern->max_x = 0;
	pattern->max_y = 0;
	for (int y = 0; y < pattern->height; y++) {
		for (int x = 0; x < pattern->width; x++) {
			uint8_t color = pattern_get_color(pattern, x, y);
			if (color) {
				pattern->min_x = (x < pattern->min_x) ? x : pattern->min_x;
				pattern->min_y = (y < pattern->min_y) ? y : pattern->min_y;
				pattern->max_x = (x > pattern->max_x) ? x : pattern->max_x;
				pattern->max_y = (y > pattern->max_y) ? y : pattern->max_y;
			}
		}
	}
}

struct pattern_t* pattern_merge(const struct pattern_t *old_pattern, const struct pattern_t *new_pattern) {
	struct pattern_t *merge = pattern_new((old_pattern->width > new_pattern->width) ? old_pattern->width : new_pattern->width, (old_pattern->height > new_pattern->height) ? old_pattern->height : new_pattern->height);
	if (!merge) {
		return NULL;
	}
	for (unsigned int y = 0; y < merge->height; y++) {
		for (unsigned int x = 0; x < merge->width; x++) {
			uint8_t pixel = pattern_get_color(new_pattern, x, y);
			if (!pixel) {
				pixel = pattern_get_color(old_pattern, x, y);
			}
			pattern_set_color(merge, x, y, pixel);
		}
	}

	return merge;
}

struct pattern_t* pattern_trim(struct pattern_t *pattern) {
	pattern_update_min_max(pattern);
	if ((pattern->min_x > pattern->max_x) || (pattern->min_y > pattern->max_y)) {
		/* Either width or height is zero. */
		return pattern_new(0, 0);
	}

	struct pattern_t *trimmed = pattern_new(pattern->max_x - pattern->min_x + 1, pattern->max_y - pattern->min_y + 1);
	if (!trimmed) {
		return NULL;
	}
	for (unsigned int y = 0; y < trimmed->height; y++) {
		for (unsigned int x = 0; x < trimmed->width; x++) {
			uint8_t pixel = pattern_get_color(pattern, x + pattern->min_x, y + pattern->min_y);
			pattern_set_color(trimmed, x, y, pixel);
		}
	}
	trimmed->min_x = 0;
	trimmed->min_y = 0;
	trimmed->max_x = trimmed->width - 1;
	trimmed->max_y = trimmed->height - 1;
	return trimmed;
}

void pattern_dump(const struct pattern_t *pattern) {
	for (int y = 0; y < pattern->height; y++) {
		pattern_dump_row(pattern, y);
	}
}

void pattern_free(struct pattern_t *pattern) {
	if (pattern) {
		free(pattern->pixel_data);
		free(pattern);
	}
}
