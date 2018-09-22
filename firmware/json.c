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
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include "json.h"

void json_print_dict(FILE *f, const struct json_dict_entry_t *entries) {
	assert(entries);
	fprintf(f, "{ ");
	bool first = true;
	while (entries->key) {
		if (!first) {
			fprintf(f, ", ");
		} else {
			first = false;
		}
		fprintf(f, "\"%s\": ", entries->key);
		switch (entries->value_type) {
			case JSON_INT:
				fprintf(f, "%d", entries->value.integer);
				break;

			case JSON_STRING:
				fprintf(f, "\"%s\"", entries->value.string);
				break;

			case JSON_BOOL:
				fprintf(f, "%s", entries->value.boolean ? "true" : "false");
				break;

		}
		entries++;
	}
	fprintf(f, " }\n");
}

void __attribute__ ((format (printf, 3, 4))) json_respond_simple(FILE *f, const char *msg_type, const char *message, ...) {
	char message_buffer[256];
	va_list ap;
	va_start(ap, message);
	vsnprintf(message_buffer, sizeof(message_buffer), message, ap);
	va_end(ap);

	struct json_dict_entry_t json_dict[] = {
		JSON_DICTENTRY_STR("msg_type", msg_type),
		JSON_DICTENTRY_STR("message", message_buffer),
		{ 0 },
	};
	json_print_dict(f, json_dict);
}

void json_trace(const struct json_dict_entry_t *entries) {
	static FILE *tracefile;
	if (!tracefile) {
		tracefile = fopen("trace.json", "a");
	}
	if (tracefile) {
		json_print_dict(tracefile, entries);
		fflush(tracefile);
	}
}
