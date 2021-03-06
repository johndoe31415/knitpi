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
#include "gpio_thread.h"
#include "tools.h"

static bool run_thread;

static void* gpio_thread_body(void *vhandler) {
	gpio_irq_callback_t handler = (gpio_irq_callback_t)vhandler;
	while (run_thread) {
		gpio_wait_for_input_change(handler, 5000);
	}
	return NULL;
}

void start_gpio_thread(gpio_irq_callback_t irq_handler, bool initial_notify) {
	run_thread = true;
	if (!start_detached_thread(gpio_thread_body, irq_handler)) {
		perror("failed to start GPIO thread");
		return;
	}
	if (initial_notify) {
		gpio_notify_all_inputs(irq_handler);
	}
}

void stop_gpio_thread(void) {
	run_thread = false;
}
