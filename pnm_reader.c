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

struct pnmfile_t* pnmfile_read(const char *filename) {
	struct pnmfile_t *pnmfile = calloc(1, sizeof(struct pnmfile_t));
	FILE *f = fopen(filename, "r");
	if (!f) {
		perror(filename);
		pnmfile_free(pnmfile);
		return NULL;
	}

	char line[256];
	for (int i = 0; i < 3; i++) {
		if (fgets(line, sizeof(line), f) == NULL) {
			perror("Error reading line from file");
			pnmfile_free(pnmfile);
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
				pnmfile_free(pnmfile);
				return NULL;
			}
		} else if (i == 1) {
			char *token = strtok(line, " ");
			if (!token) {
				fprintf(stderr, "Unsupported PNM file, width could not be read.\n");
				pnmfile_free(pnmfile);
				return NULL;
			}
			pnmfile->width = atoi(token);
			token = strtok(NULL, " ");
			if (!token) {
				fprintf(stderr, "Unsupported PNM file, height could not be read.\n");
				pnmfile_free(pnmfile);
				return NULL;
			}
			pnmfile->height = atoi(token);
			if ((pnmfile->width < 1) || (pnmfile->height < 1)) {
				fprintf(stderr, "Unsupported PNM file size: %d x %d pixels.\n", pnmfile->width, pnmfile->height);
				pnmfile_free(pnmfile);
				return NULL;
			}
		} else if (i == 2) {
			int depth = atoi(line);
			if (depth != 255) {
				fprintf(stderr, "Unsupported PNM color depth: %d colors.\n", depth);
				pnmfile_free(pnmfile);
				return NULL;
			}
		}
	}

	pnmfile->image_data = malloc(pnmfile->width * pnmfile->height);
	if (!pnmfile->image_data) {
		fprintf(stderr, "Failed to allocate %d bytes for %d x %d pixel PNM file %s: %s\n", pnmfile->width * pnmfile->height, pnmfile->width, pnmfile->height, filename, strerror(errno));
		pnmfile_free(pnmfile);
		return NULL;
	}

	for (int y = 0; y < pnmfile->height; y++) {
		uint8_t row_data[pnmfile->width * 3];
		if (fread(row_data, sizeof(row_data), 1, f) != 1) {
			fprintf(stderr, "Failed to read row data from PNM file, y = %d.\n", y);
			pnmfile_free(pnmfile);
		}
		for (int x = 0; x < pnmfile->width; x++) {
			uint8_t r = row_data[(3 * x) + 0];
			uint8_t g = row_data[(3 * x) + 1];
			uint8_t b = row_data[(3 * x) + 2];
			uint8_t gray = r & g & b;
			pnmfile->image_data[(pnmfile->width * y) + x] = ~gray;
		}
	}

	return pnmfile;
}

const uint8_t* pnmfile_row(const struct pnmfile_t *pnmfile, unsigned int y) {
	return pnmfile->image_data + (pnmfile->width * y);
}

void pnmfile_dump(const struct pnmfile_t *pnmfile) {
	for (int y = 0; y < pnmfile->height; y++) {
		const uint8_t *row = pnmfile_row(pnmfile, y);
		for (int x = 0; x < pnmfile->width; x++) {
			printf("%s", row[x] ? "*" : " ");
		}
		printf("\n");
	}
}

void pnmfile_free(struct pnmfile_t *pnmfile) {
	free(pnmfile->image_data);
	free(pnmfile);
}
