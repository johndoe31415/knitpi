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

#ifndef __SLED_H__
#define __SLED_H__

#include <stdbool.h>
#include <time.h>
#include "peripherals_gpio.h"
#include "knitcore.h"

typedef void (*sled_callback_t)(struct server_state_t *server_state, int position, bool belt_phase);

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
unsigned int sled_get_skipped_needles_cnt(void);
void sled_set_callback(struct server_state_t *server_state, sled_callback_t callback);
void sled_input(enum gpio_t gpio, const struct timespec *ts, bool value);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
