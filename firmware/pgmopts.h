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

#ifndef __PGMOPTS_H__
#define __PGMOPTS_H__

#include <stdbool.h>
#include "logging.h"

struct pgmopts_t {
	bool quit_after_single_connection;
	bool force;
	bool no_hardware;
	enum loglvl_t loglevel;
	const char *unix_socket;
	int max_bindata_recv_bytes;
};

extern const struct pgmopts_t *pgm_opts;

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
void parse_pgmopts(int argc, char **argv);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
