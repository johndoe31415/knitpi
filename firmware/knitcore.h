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

#ifndef __KNITCORE_H__
#define __KNITCORE_H__

#include <stdbool.h>
#include <stdint.h>
#include "pattern.h"
#include "atomic.h"
#include "isleep.h"

enum repeat_mode_t {
	RPTMODE_ONESHOT,
	RPTMODE_REPEAT,
	RPTMODE_MANUAL,
};

struct server_state_t {
	struct isleep_t event_notification;
	bool knitting_mode;
	enum repeat_mode_t repeat_mode;
	bool even_rows_left_to_right;
	bool carriage_position_valid;
	bool belt_phase;
	int32_t carriage_position;
	int32_t pattern_row;
	int32_t pattern_offset;
	struct pattern_t *pattern;
	struct atomic_ctr_t thread_count;
};

#define SERVER_STATE_INITIALIZER		{		\
	.event_notification = ISLEEP_INITIALIZER,	\
	.thread_count = ATOMIC_CTR_INITIALIZER(0),	\
}

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
void set_knitting_mode(struct server_state_t *state, bool knitting_mode);
void sled_update(struct server_state_t *server_state);
void sled_actuation_callback(struct server_state_t *server_state, int position, bool belt_phase);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
