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

#ifndef __ISLEEP_H__
#define __ISLEEP_H__

#include <pthread.h>
#include <stdbool.h>

struct isleep_t {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

#define ISLEEP_INITIALIZER		{ \
	.mutex = PTHREAD_MUTEX_INITIALIZER,	\
	.cond = PTHREAD_COND_INITIALIZER,	\
}

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
void isleep_interrupt(struct isleep_t *isleep);
bool isleep_abs(struct isleep_t *isleep, const struct timespec *abstime);
bool isleep(struct isleep_t *isleep, unsigned int milliseconds);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
