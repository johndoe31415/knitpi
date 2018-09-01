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

//void gpio_irq_callback_t

#ifndef __DEBOUNCER_H__
#define __DEBOUNCER_H__

#include "peripherals_gpio.h"

#define MAX_DEBOUNCE_GPIOS		16

struct debouncer_state_t {
	bool debounced_state;
	bool pending_change;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
void debouncer_input(enum gpio_t gpio, const struct timespec *ts, bool value);
bool start_debouncer_thread(gpio_irq_callback_t debouncer_output_callback);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
