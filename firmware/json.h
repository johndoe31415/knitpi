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

#ifndef __JSON_H__
#define __JSON_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

enum json_type_t {
	JSON_INT,
	JSON_STRING,
	JSON_BOOL,
};

struct json_dict_entry_t {
	const char *key;
	enum json_type_t value_type;
	union {
		int integer;
		const char *string;
		bool boolean;
	} value;
};

#define JSON_DICTENTRY_INT(_key, _value)		{ .key = (_key), .value_type = JSON_INT, .value.integer = (_value) }
#define JSON_DICTENTRY_STR(_key, _value)		{ .key = (_key), .value_type = JSON_STRING, .value.string = (_value) }
#define JSON_DICTENTRY_BOOL(_key, _value)		{ .key = (_key), .value_type = JSON_BOOL, .value.boolean = (_value) }

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
void json_print_dict(FILE *f, const struct json_dict_entry_t *entries);
void json_respond_simple(FILE *f, const char *msg_type, const char *message, ...);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
