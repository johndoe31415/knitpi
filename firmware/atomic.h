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

#include <pthread.h>

#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#define ATOMIC_CTR_INITIALIZER(value)	{	\
	.ctr = (value),							\
	.mutex = PTHREAD_MUTEX_INITIALIZER,		\
	.cond = PTHREAD_COND_INITIALIZER,		\
}

struct atomic_ctr_t {
	int ctr;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
void atomic_inc_value(struct atomic_ctr_t *atomic, int value);
void atomic_inc(struct atomic_ctr_t *atomic);
void atomic_dec(struct atomic_ctr_t *atomic);
void atomic_wait(struct atomic_ctr_t *atomic, int target);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
