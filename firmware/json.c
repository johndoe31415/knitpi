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
#include "json.h"

void json_print_str(FILE *f, const char *key, const char *value) {
	fprintf(f, "\"%s\": \"%s\", ", key, value);
}

void json_print_bool(FILE *f, const char *key, bool value) {
	fprintf(f, "\"%s\": %s, ", key, value ? "true" : "false");
}

void json_printf_str(FILE *f, const char *key, const char *msg, ...) {
	va_list ap;
	char buffer[1024];
	va_start(ap, msg);
	vsnprintf(buffer, sizeof(buffer), msg, ap);
	va_end(ap);
	json_print_str(f, key, buffer);
}

void json_print_int(FILE *f, const char *key, int32_t value) {
	fprintf(f, "\"%s\": %d, ", key, value);
}
