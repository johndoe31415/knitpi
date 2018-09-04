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

#include <stdbool.h>
#include "pgmopts.h"
#include "argparse.h"

static struct pgmopts_t pgm_opts_rw = {
	.loglevel = LLVL_ERROR,
};
const struct pgmopts_t *pgm_opts = &pgm_opts_rw;

static bool knitserver_pgmopts(enum argparse_option_t option, const char *value) {
	switch (option) {
		case ARG_NO_HARDWARE:
			pgm_opts_rw.no_hardware = true;
			break;

		case ARG_FORCE:
			pgm_opts_rw.force = true;
			break;

		case ARG_VERBOSE:
			pgm_opts_rw.loglevel++;
			break;

		case ARG_UNIX_SOCKET:
			pgm_opts_rw.unix_socket = value;
			break;

		case ARG_QUIT:
			pgm_opts_rw.quit_after_single_connection = true;
			break;
	}
	return true;
}

void parse_pgmopts(int argc, char **argv) {
	argparse_parse_or_die(argc, argv, knitserver_pgmopts);
}
