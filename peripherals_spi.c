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

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "peripherals_spi.h"

#define SPI_COUNT	(sizeof(spi_init_data) / sizeof(struct spi_init_data_t))

static const struct spi_init_data_t spi_init_data[] = {
	[SPI_74HC595] = {
		.device = "/dev/spidev0.0",
		.speed_khz = 500,
	},
};

struct spi_runtime_data_t {
	int fd;
};
static struct spi_runtime_data_t spi_runtime_data[SPI_COUNT];

void spi_init(void) {
	for (int i = 0; i < SPI_COUNT; i++) {
		const struct spi_init_data_t *init_data = &spi_init_data[i];
		struct spi_runtime_data_t *runtime_data = &spi_runtime_data[i];
		runtime_data->fd = open(init_data->device, O_RDWR);
		if (runtime_data->fd == -1) {
			perror(init_data->device);
			return;
		}

		{
			uint8_t spi_mode = SPI_MODE_0;
			if (ioctl(runtime_data->fd, SPI_IOC_WR_MODE, &spi_mode) == -1) {
				perror("ioctl(SPI_IOC_WR_MODE)");
				return;
			}
			if (ioctl(runtime_data->fd, SPI_IOC_RD_MODE, &spi_mode) == -1) {
				perror("ioctl(SPI_IOC_RD_MODE)");
				return;
			}
		}
		{
			uint8_t bits_per_word = 8;
			if (ioctl(runtime_data->fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) == -1) {
				perror("ioctl(SPI_IOC_WR_BITS_PER_WORD)");
				return;
			}
			if (ioctl(runtime_data->fd, SPI_IOC_RD_BITS_PER_WORD, &bits_per_word) == -1) {
				perror("ioctl(SPI_IOC_RD_BITS_PER_WORD)");
				return;
			}
		}
		{
			uint32_t speed_hz = init_data->speed_khz * 1000;
			if (ioctl(runtime_data->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) == -1) {
				perror("ioctl(SPI_IOC_WR_MAX_SPEED_HZ)");
				return;
			}
			if (ioctl(runtime_data->fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed_hz) == -1) {
				perror("ioctl(SPI_IOC_RD_MAX_SPEED_HZ)");
				return;
			}
		}
	}
}

void spi_send(enum spi_bus_t spi_bus, const uint8_t *spi_tx_data, unsigned int tx_length) {
	const struct spi_init_data_t *init_data = &spi_init_data[spi_bus];
	struct spi_runtime_data_t *runtime_data = &spi_runtime_data[spi_bus];
	uint8_t dummy[tx_length];
	struct spi_ioc_transfer transfer = {
		.tx_buf = (unsigned long)spi_tx_data,
		.rx_buf = (unsigned long)dummy,
		.len = tx_length,
		.delay_usecs = 1,		/* Deassert !CS 1µs after last bit */
		.speed_hz = init_data->speed_khz * 1000,
		.bits_per_word = 8,
	};
	if (ioctl(runtime_data->fd, SPI_IOC_MESSAGE(1), &transfer) < 1) {
		perror("Could not send SPI transmission");
	}
}
