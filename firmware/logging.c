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
#include <stdarg.h>
#include <time.h>
#include "logging.h"

static enum loglvl_t configured_loglvl = LLVL_FATAL;

static const char *loglvl_text[] = {
	[LLVL_FATAL] = "FATAL",
	[LLVL_ERROR] = "ERROR",
	[LLVL_WARN] = "WARN",
	[LLVL_INFO] = "INFO",
	[LLVL_DEBUG] = "DEBUG",
	[LLVL_TRACE] = "TRACE",
};

static void get_datetime(char datetime[32]) {
	time_t timet = time(NULL);
	struct tm tm;
	localtime_r(&timet, &tm),
	strftime(datetime, 32, "%Y-%m-%d %H:%M:%S", &tm);
}

void set_loglevel(enum loglvl_t new_loglevel) {
	configured_loglvl = new_loglevel;
}

void __attribute__ ((format (printf, 2, 3))) logmsg(enum loglvl_t msg_loglvl, const char *msg, ...) {
	if (msg_loglvl > configured_loglvl) {
		return;
	}
	char datetime[16];
	get_datetime(datetime);
	fprintf(stderr, "%s [%s]: ", datetime, loglvl_text[msg_loglvl]);

	va_list ap;
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}
