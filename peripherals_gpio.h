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

#ifndef __PERIPHERALS_GPIO_H__
#define __PERIPHERALS_GPIO_H__

#include <stdint.h>
#include <stdbool.h>

enum gpio_t {
	GPIO_74HC595_OE,
	GPIO_BROTHER_V1,
	GPIO_BROTHER_V2,
	GPIO_BROTHER_BR,
	GPIO_BROTHER_LEFT_HALL,
	GPIO_BROTHER_RIGHT_HALL,
};

struct gpio_action_t {
	enum gpio_t gpio;
	bool value;
};

struct gpio_init_data_t {
	const char *name;
	int gpio_no;
	bool active_low;
	bool is_output;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
void gpio_init(void);
void gpio_active(enum gpio_t gpio);
void gpio_inactive(enum gpio_t gpio);
void gpio_pulse(enum gpio_t gpio, uint16_t microseconds);
void gpio_set_to(enum gpio_t gpio, bool value);
unsigned int gpio_wait_for_input_change(struct gpio_action_t *actions, unsigned int max_actions, unsigned int timeout_millis);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
