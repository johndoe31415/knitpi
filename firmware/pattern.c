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

struct pattern_t* pattern_new(unsigned int width, unsigned int height) {
	struct pattern_t *pattern = calloc(1, sizeof(struct pattern_t));
	if (!pattern) {
		perror("calloc pattern");
		return NULL;
	}

	pattern->width = width;
	pattern->height = height;
	pattern->pixel_data = malloc(pattern->width * pattern->height);
	if (!pattern->pixel_data) {
		fprintf(stderr, "Failed to allocate %d bytes for %d x %d pixel pattern: %s\n", pattern->width * pattern->height, pattern->width, pattern->height, strerror(errno));
		pattern_free(pattern);
		return NULL;
	}

	return pattern;
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
	free(pattern->pixel_data);
	free(pattern);
}
