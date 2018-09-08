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

#include <unistd.h>
#include <stdlib.h>

#include "peripherals.h"
#include "gpio_thread.h"
#include "debouncer.h"
#include "sled.h"
#include "server.h"
#include "pgmopts.h"

int main(int argc, char **argv) {
	parse_pgmopts(argc, argv);
	set_loglevel(pgm_opts->loglevel);

	struct server_state_t server_state = SERVER_STATE_INITIALIZER;
	if (!pgm_opts->no_hardware) {
		if (!all_peripherals_init()) {
			logmsg(LLVL_FATAL, "Failed to initialize hardware peripherals.");
			exit(EXIT_FAILURE);
		}
		sled_set_callback(&server_state, sled_actuation_callback);
		start_debouncer_thread(sled_input);
		start_gpio_thread(debouncer_input, true);
		spi_clear(SPI_74HC595, 2);
	}

	if (pgm_opts->force) {
		unlink(pgm_opts->unix_socket);
	}

	if (!start_server(&server_state)) {
		logmsg(LLVL_FATAL, "Failed to start server.");
		exit(EXIT_FAILURE);
	}
	pattern_free(server_state.pattern);
	return 0;
}
