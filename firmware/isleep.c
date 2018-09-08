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
#include "isleep.h"
#include "tools.h"

void isleep_interrupt(struct isleep_t *isleep) {
	pthread_cond_broadcast(&isleep->cond);
}

bool isleep_abs(struct isleep_t *isleep, const struct timespec *abstime) {
	pthread_mutex_lock(&isleep->mutex);
	int wait_result = pthread_cond_timedwait(&isleep->cond, &isleep->mutex, abstime);
	pthread_mutex_unlock(&isleep->mutex);
	return (wait_result == 0);
}

bool isleep(struct isleep_t *isleep, unsigned int milliseconds) {
	struct timespec abstime;
	get_abs_timespec_offset(&abstime, milliseconds);
	return isleep_abs(isleep, &abstime);
}
