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
#include <stdbool.h>
#include <string.h>
#include "peripherals_gpio.h"
#include "debouncer.h"
#include "tools.h"
#include "isleep.h"

static bool run_debouncer_thread;
static struct debouncer_state_t debounce_state[GPIO_COUNT] = {
	[GPIO_BROTHER_V1] = {
		.debounce_time_ms = 0,
	},
	[GPIO_BROTHER_V2] = {
		.debounce_time_ms = 0,
	},
	[GPIO_BROTHER_BP] = {
		.debounce_time_ms = 5,
	},
	[GPIO_BROTHER_LEFT_HALL] = {
		.debounce_time_ms = 25,
	},
	[GPIO_BROTHER_RIGHT_HALL] = {
		.debounce_time_ms = 25,
	},
};
static pthread_mutex_t debounce_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct isleep_t sleeper = ISLEEP_INITIALIZER;
static gpio_irq_callback_t global_debouncer_output_callback;

void debouncer_input(enum gpio_t gpio, const struct timespec *ts, bool value) {
	struct debouncer_state_t *debounce = &debounce_state[gpio];
	pthread_mutex_lock(&debounce_mutex);

	if (!debounce->initialized) {
		debounce->initialized = true;
		debounce->debounced_state = value;
	} else {
		if (debounce->debounced_state == value) {
			/* Same value as already debounced, reset timer */
			debounce->pending_change = false;
		} else {
			/* Different value than debounced */
			if (!debounce->pending_change) {
				/* We don't have that change recorded yet. */
				debounce->pending_change = true;
				memcpy(&debounce->change_time, ts, sizeof(struct timespec));
				add_timespec_offset(&debounce->change_time, debounce->debounce_time_ms);
				isleep_interrupt(&sleeper);
			}
		}
	}
	pthread_mutex_unlock(&debounce_mutex);
}

static void* debouncer_thread(void *vcallback) {
	struct {
		int gpio_id;
		bool new_state;
	} notifiers[GPIO_COUNT];

	while (run_debouncer_thread) {
		pthread_mutex_lock(&debounce_mutex);
		struct timespec now;
		get_timespec_now(&now);

		struct timespec sleep_until = now;
		add_timespec_offset(&sleep_until, 1000);

		int notification_count = 0;
		for (int i = 0; i < GPIO_COUNT; i++) {
			if (debounce_state[i].initialized && debounce_state[i].pending_change) {
				/* Is this expired? */
				if (timespec_lt(&debounce_state[i].change_time, &now)) {
					/* Yes, perform the change! */
					debounce_state[i].debounced_state = !debounce_state[i].debounced_state;
					debounce_state[i].pending_change = false;
					notifiers[notification_count].gpio_id = i;
					notifiers[notification_count].new_state = debounce_state[i].debounced_state;
					notification_count++;
				} else {
					/* No, not yet -- adapt sleep timer */
					timespec_min(&sleep_until, &sleep_until, &debounce_state[i].change_time);
				}
			}
		}
		pthread_mutex_unlock(&debounce_mutex);

		/* Finally notify all changed handlers */
		for (int i = 0; i < notification_count; i++) {
			global_debouncer_output_callback(notifiers[i].gpio_id, &now, notifiers[i].new_state);
		}
		isleep_abs(&sleeper, &sleep_until);
	}
}

bool start_debouncer_thread(gpio_irq_callback_t debouncer_output_callback) {
	run_debouncer_thread = true;
	global_debouncer_output_callback = debouncer_output_callback;
	if (!start_detached_thread(debouncer_thread, debouncer_output_callback)) {
		fprintf(stderr, "Failed to start debouncer thread.\n");
		return false;
	}
	return true;
}
