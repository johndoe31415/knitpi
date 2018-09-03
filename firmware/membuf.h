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

#ifndef __MEMBUF_H__
#define __MEMBUF_H__

#include <stdint.h>

#define MEMBUF_INITIALIZER		 { 0 }

struct membuf_t {
	uint8_t *data;
	unsigned int length;
	unsigned int position;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
void membuf_init(struct membuf_t *membuf);
bool membuf_resize(struct membuf_t *membuf, unsigned int length);
bool membuf_append(struct membuf_t *membuf, const uint8_t *data, unsigned int length);
bool membuf_read(struct membuf_t *membuf, uint8_t *data, unsigned int length);
bool membuf_seek(struct membuf_t *membuf, unsigned int offset);
void membuf_rewind(struct membuf_t *membuf);
bool membuf_write_to_file(const struct membuf_t *membuf, const char *filename);
void membuf_free(struct membuf_t *membuf);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
