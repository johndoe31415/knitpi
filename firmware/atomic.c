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
#include "atomic.h"

void atomic_inc_value(struct atomic_ctr_t *atomic, int value) {
	pthread_mutex_lock(&atomic->mutex);
	atomic->ctr += value;
	pthread_mutex_unlock(&atomic->mutex);
	pthread_cond_broadcast(&atomic->cond);
}

void atomic_inc(struct atomic_ctr_t *atomic) {
	atomic_inc_value(atomic, 1);
}

void atomic_dec(struct atomic_ctr_t *atomic) {
	atomic_inc_value(atomic, -1);
}

void atomic_wait(struct atomic_ctr_t *atomic, int target) {
	pthread_mutex_lock(&atomic->mutex);
	while (atomic->ctr != target) {
		pthread_cond_wait(&atomic->cond, &atomic->mutex);
	}
	pthread_mutex_unlock(&atomic->mutex);
}
