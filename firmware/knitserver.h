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

#ifndef __KNITSERVER_H__
#define __KNITSERVER_H__

#include <stdbool.h>
#include <stdint.h>
#include "pattern.h"
#include "atomic.h"

#define MAX_PNG_RECV_SIZE		(128 * 1024)

enum server_mode_t {
	MODE_OFFLINE,
	MODE_ONLINE,
};

struct server_state_t {
	enum server_mode_t server_mode;
	bool carriage_position_valid;
	bool belt_phase;
	bool direction_left_to_right;
	int32_t carriage_position;
	int32_t pattern_row;
	struct pattern_t *pattern;
	struct atomic_ctr_t thread_count;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
