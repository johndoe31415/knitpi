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
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>
#include "tools.h"

bool start_detached_thread(thread_function_t thread_fnc, void *argument) {
	pthread_attr_t attrs;
	if (pthread_attr_init(&attrs)) {
		perror("pthread_attr_init");
		return false;
	}
	if (pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED)) {
		perror("pthread_attr_setdetachstate");
		return false;
	}

	pthread_t thread;
	if (pthread_create(&thread, &attrs, thread_fnc, argument)) {
		perror("pthread_create");
		return false;
	}
	return true;
}

void get_abs_timespec_offset(struct timespec *timespec, int32_t offset_milliseconds) {
	struct timeval now;
	if (gettimeofday(&now, NULL) != 0) {
		perror("gettimeofday");
		return;
	}

	int32_t offset_full_seconds = offset_milliseconds / 1000;
	int32_t offset_full_microseconds = 1000 * (offset_milliseconds % 1000);
	now.tv_sec += offset_full_seconds;
	now.tv_usec += offset_full_microseconds;
	if (now.tv_usec < 0) {
		now.tv_sec -= 1;
		now.tv_usec += 1000000;
	} else if (now.tv_usec >= 1000000) {
		now.tv_sec += 1;
		now.tv_usec -= 1000000;
	}
	
	timespec->tv_sec = now.tv_sec;
	timespec->tv_nsec = now.tv_usec * 1000;
}

int64_t timespec_diff(struct timespec *a, struct timespec *b) {
	int64_t nanoseconds_difference = (a->tv_sec - b->tv_sec) * 1000000000;
	nanoseconds_difference += (a->tv_nsec - b->tv_nsec);
	return nanoseconds_difference;
}
