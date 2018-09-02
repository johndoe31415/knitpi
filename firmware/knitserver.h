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

#define MAX_SUPPORTED_PAYLOAD_SIZE		(3 * 1024 * 1024)

enum socket_cmd_t {
	CMD_GET_STATUS = 0,
	RSP_GET_STATUS = 1,
	CMD_SET_MODE = 2,
	RSP_SET_MODE = 3,
	CMD_SET_PATTERN = 4,
	RSP_SET_PATTERN = 5,
};

enum server_mode_t {
	MODE_OFFLINE = 0,
	MODE_ONLINE = 1,
};

struct msg_t {
	uint32_t cmdcode;
	uint32_t payload_size;
	uint8_t payload[];
};

struct rsp_get_info_msg_t {
	struct msg_t header;
	uint16_t server_mode;
	bool carriage_position_valid;
	bool belt_phase;
	bool direction_left_to_right;
	int32_t carriage_position;
	uint32_t skipped_needle_cnt;
	int32_t pattern_row;
	uint16_t pattern_width, pattern_height;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
