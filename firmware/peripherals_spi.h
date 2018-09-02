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

#ifndef __PERIPHERALS_SPI_H__
#define __PERIPHERALS_SPI_H__

#include <stdint.h>
#include <stdbool.h>

enum spi_bus_t {
	SPI_74HC595,
};

struct spi_init_data_t {
	const char *device;
	uint32_t speed_khz;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
bool spi_init(void);
bool spi_send(enum spi_bus_t spi_bus, const uint8_t *spi_tx_data, unsigned int tx_length);
bool spi_clear(enum spi_bus_t spi_bus, unsigned int tx_length);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
