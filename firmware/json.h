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

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
void json_print_str(FILE *f, const char *key, const char *value);
void json_print_bool(FILE *f, const char *key, bool value);
void json_printf_str(FILE *f, const char *key, const char *msg, ...);
void json_print_int(FILE *f, const char *key, int32_t value);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif