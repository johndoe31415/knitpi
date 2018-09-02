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
#include "pnm_reader.h"
#include "pattern.h"

struct pattern_t* pnmfile_read(const char *filename) {
	FILE *f = fopen(filename, "r");
	if (!f) {
		perror(filename);
		return NULL;
	}

	char line[256];
	int width = 0;
	int height = 0;
	for (int i = 0; i < 3; i++) {
		if (fgets(line, sizeof(line), f) == NULL) {
			perror("Error reading line from file");
			return NULL;
		}
		if (line[0] == '#') {
			// Ignore comment line
			i--;
		   continue;
		}

		int l = strlen(line);
		if (l && (line[l - 1] == '\n')) {
			line[--l] = 0;
		}
		if (l && (line[l - 1] == '\r')) {
			line[--l] = 0;
		}

		if (i == 0) {
			if (strcmp(line, "P6")) {
				fprintf(stderr, "Unsupported PNM file, header not recognized.\n");
				return NULL;
			}
		} else if (i == 1) {
			char *token = strtok(line, " ");
			if (!token) {
				fprintf(stderr, "Unsupported PNM file, width could not be read.\n");
				return NULL;
			}
			width = atoi(token);
			token = strtok(NULL, " ");
			if (!token) {
				fprintf(stderr, "Unsupported PNM file, height could not be read.\n");
				return NULL;
			}
			height = atoi(token);
			if ((width < 1) || (height < 1)) {
				fprintf(stderr, "Unsupported PNM file size: %d x %d pixels.\n", width, height);
				return NULL;
			}
		} else if (i == 2) {
			int depth = atoi(line);
			if (depth != 255) {
				fprintf(stderr, "Unsupported PNM color depth: %d colors.\n", depth);
				return NULL;
			}
		}
	}

	struct pattern_t *pattern = pattern_new(width, height);
	if (!pattern) {
		fprintf(stderr, "Failed to allocate pattern data while reading PNM image.\n");
		return NULL;
	}

	for (int y = 0; y < pattern->height; y++) {
		uint8_t rgb_row_data[pattern->width * 3];
		if (fread(rgb_row_data, sizeof(rgb_row_data), 1, f) != 1) {
			fprintf(stderr, "Failed to read row data from PNM file, y = %d.\n", y);
			pattern_free(pattern);
		}

		uint8_t *pattern_row_data = pattern_row_rw(pattern, y);
		for (int x = 0; x < pattern->width; x++) {
			uint8_t r = rgb_row_data[(3 * x) + 0];
			uint8_t g = rgb_row_data[(3 * x) + 1];
			uint8_t b = rgb_row_data[(3 * x) + 2];
			uint8_t gray = r & g & b;
			pattern_row_data[x] = ~gray;
		}
	}

	return pattern;
}

