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
#include <string.h>
#include <signal.h>
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

void add_timespec_offset(struct timespec *timespec, int32_t offset_milliseconds) {
	int32_t offset_full_seconds = offset_milliseconds / 1000;
	int32_t offset_full_nanoseconds = 1000000 * (offset_milliseconds % 1000);
	timespec->tv_sec += offset_full_seconds;
	timespec->tv_nsec += offset_full_nanoseconds;
	if (timespec->tv_nsec < 0) {
		timespec->tv_sec -= 1;
		timespec->tv_nsec += 1000000000;
	} else if (timespec->tv_nsec >= 1000000000) {
		timespec->tv_sec += 1;
		timespec->tv_nsec -= 1000000000;
	}
}

void get_timespec_now(struct timespec *timespec) {
	struct timeval now;
	if (gettimeofday(&now, NULL) != 0) {
		perror("gettimeofday");
		return;
	}
	timespec->tv_sec = now.tv_sec;
	timespec->tv_nsec = now.tv_usec * 1000;
}

void get_abs_timespec_offset(struct timespec *timespec, int32_t offset_milliseconds) {
	get_timespec_now(timespec);
	add_timespec_offset(timespec, offset_milliseconds);
}

int64_t timespec_diff(const struct timespec *a, const struct timespec *b) {
	int64_t nanoseconds_difference = (a->tv_sec - b->tv_sec) * (int64_t)1000000000;
	nanoseconds_difference += (a->tv_nsec - b->tv_nsec);
	return nanoseconds_difference;
}

bool timespec_lt(const struct timespec *a, const struct timespec *b) {
	if (a->tv_sec < b->tv_sec) {
		return true;
	} else if (a->tv_sec > b->tv_sec) {
		return false;
	} else {
		/* Seconds equal, compare nanoseconds */
		return (a->tv_nsec < b->tv_nsec);
	}
}

void timespec_min(struct timespec *result, const struct timespec *a, const struct timespec *b) {
	if (timespec_lt(a, b)) {
		if (result != a) {
			memcpy(result, a, sizeof(struct timespec));
		}
	} else {
		if (result != b) {
			memcpy(result, b, sizeof(struct timespec));
		}
	}
}

bool ignore_signal(int signum) {
	struct sigaction action = {
		.sa_handler = SIG_IGN,
	};
	return (sigaction(signum, &action, NULL) == 0);
}

bool file_discard_data(FILE *f, unsigned int discard_bytes) {
	while (discard_bytes) {
		uint8_t buffer[16 * 1024];
		unsigned int chunk_size = (discard_bytes > sizeof(buffer)) ? sizeof(buffer) : discard_bytes;
		size_t discarded = fread(buffer, 1, chunk_size, f);
		if (discarded <= 0) {
			return false;
		}
		discard_bytes -= chunk_size;
	}
	return true;
}

int trim_crlf(char *string) {
	int len = strlen(string);
	if (len && (string[len - 1] == '\n')) {
		string[--len] = 0;
	}
	if (len && (string[len - 1] == '\r')) {
		string[--len] = 0;
	}
	return len;
}
