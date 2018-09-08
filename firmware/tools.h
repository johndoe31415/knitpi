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

#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef void* (*thread_function_t)(void *arg);

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
bool start_detached_thread(thread_function_t thread_fnc, void *argument);
void add_timespec_offset(struct timespec *timespec, int32_t offset_milliseconds);
void get_timespec_now(struct timespec *timespec);
void get_abs_timespec_offset(struct timespec *timespec, int32_t offset_milliseconds);
int64_t timespec_diff(const struct timespec *a, const struct timespec *b);
bool timespec_lt(const struct timespec *a, const struct timespec *b);
void timespec_min(struct timespec *result, const struct timespec *a, const struct timespec *b);
bool ignore_signal(int signum);
bool file_discard_data(FILE *f, unsigned int discard_bytes);
int trim_crlf(char *string);
bool safe_atoi(const char *string, int *result);
bool safe_atod(const char *string, double *result);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
