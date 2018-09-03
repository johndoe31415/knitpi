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
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "membuf.h"

void membuf_init(struct membuf_t *membuf) {
	memset(membuf, 0, sizeof(struct membuf_t));
}

bool membuf_resize(struct membuf_t *membuf, unsigned int length) {
	uint8_t *realloced = realloc(membuf->data, length);
	if (!realloced) {
		return false;
	}
	membuf->data = realloced;
	membuf->length = length;
	if (membuf->position > membuf->length) {
		membuf->position = membuf->length;
	}
	return true;
}

bool membuf_append(struct membuf_t *membuf, const uint8_t *data, unsigned int length) {
	uint8_t *realloced = realloc(membuf->data, membuf->length + length);
	if (!realloced) {
		return false;
	} else {
		membuf->data = realloced;
		memcpy(membuf->data + membuf->length, data, length);
		membuf->length += length;
	}
	return true;
}

bool membuf_read(struct membuf_t *membuf, uint8_t *data, unsigned int length) {
	if (membuf->position + length > membuf->length) {
		return false;
	}
	memcpy(data, membuf->data + membuf->position, length);
	membuf->position += length;
	return true;
}

bool membuf_seek(struct membuf_t *membuf, unsigned int offset) {
	if (offset > membuf->length) {
		return false;
	}
	membuf->position = offset;
	return true;
}

void membuf_rewind(struct membuf_t *membuf) {
	membuf_seek(membuf, 0);
}

bool membuf_write_to_file(const struct membuf_t *membuf, const char *filename) {
	FILE *f = fopen(filename, "w");
	if (!f) {
		return false;
	}
	if (fwrite(membuf->data, membuf->length, 1, f) != 1) {
		fclose(f);
		return false;
	}

	fclose(f);
	return true;
}

void membuf_free(struct membuf_t *membuf) {
	free(membuf->data);
	membuf_init(membuf);
}
