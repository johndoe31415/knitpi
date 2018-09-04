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
	if (!pattern->pixel_data) {
		fprintf(stderr, "Failed to allocate %d bytes for %d x %d pixel pattern: %s\n", pattern->width * pattern->height, pattern->width, pattern->height, strerror(errno));
		pattern_free(pattern);
		return NULL;
	}

	return pattern;
}

static void pattern_set_index(struct pattern_t *pattern, unsigned int x, unsigned int y, uint8_t color_index) {
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
	pattern_set_index(pattern, x, y, color_index);
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
