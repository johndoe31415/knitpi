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
#include <string.h>
#include <errno.h>
#include <gpiod.h>
#include "peripherals_gpio.h"
	
#define GPIO_COUNT	(sizeof(gpio_init_data) / sizeof(struct gpio_init_data_t))
#define GPIO_CHIP_FILENAME	 "/dev/gpiochip0"

static const struct gpio_init_data_t gpio_init_data[] = {
	[GPIO_74HC595_OE] = {
		.name = "74HC595_OE",
		.gpio_no = 12,
		.active_low = true,
		.is_output = true,
	},
	[GPIO_BROTHER_LEFT_HALL] = {
		.name = "BROTHER_LEFT_HALL",
		.gpio_no = 19,
		.active_low = false,
		.is_output = false,
	},
	[GPIO_BROTHER_RIGHT_HALL] = {
		.name = "BROTHER_RIGHT_HALL",
		.gpio_no = 26,
		.active_low = false,
		.is_output = false,
	},
	[GPIO_BROTHER_BP] = {
		.name = "BROTHER_BP",
		.gpio_no = 21,
		.active_low = false,
		.is_output = false,
	},
	[GPIO_BROTHER_V1] = {
		.name = "BROTHER_V1",
		.gpio_no = 20,
		.active_low = false,
		.is_output = false,
	},
	[GPIO_BROTHER_V2] = {
		.name = "BROTHER_V2",
		.gpio_no = 16,
		.active_low = false,
		.is_output = false,
	},
};

static struct gpiod_chip *gpio_chip;
struct gpio_runtime_data_t {
	struct gpiod_line *gpio_line;
	bool last_value;
};
static struct gpio_runtime_data_t gpio_runtime_data[GPIO_COUNT];

const struct gpio_init_data_t* gpio_get_init_data(enum gpio_t gpio) {
	return &gpio_init_data[gpio];
}

bool gpio_get_last_value(enum gpio_t gpio) {
	return gpio_runtime_data[gpio].last_value;
}

static enum gpio_t get_gpio_for_offset(unsigned int gpio_offset) {
	for (int i = 0; i < GPIO_COUNT; i++) {
		if (gpio_init_data[i].gpio_no == gpio_offset) {
			return i;
		}
	}
	return GPIO_INVALID;
}

void gpio_init(void) {
	gpio_chip = gpiod_chip_open(GPIO_CHIP_FILENAME);
	if (!gpio_chip) {
		perror("gpiod_chip_open(" GPIO_CHIP_FILENAME ")");
		return;
	}

	for (int i = 0; i < GPIO_COUNT; i++) {
		const struct gpio_init_data_t *init_data = &gpio_init_data[i];
		struct gpio_runtime_data_t *runtime_data = &gpio_runtime_data[i];
		runtime_data->gpio_line = gpiod_chip_get_line(gpio_chip, init_data->gpio_no);
		if (!runtime_data->gpio_line) {
			fprintf(stderr, "Failed to get GPIO line %d: %s\n", init_data->gpio_no, strerror(errno));
			return;
		}

		struct gpiod_line_request_config request_config = {
			.consumer = init_data->name,
			.request_type = (init_data->is_output ? GPIOD_LINE_REQUEST_DIRECTION_OUTPUT : GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES),
			.flags = (init_data->active_low ? GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW : 0),
		};
		if (gpiod_line_request(runtime_data->gpio_line, &request_config, 0)) {
			fprintf(stderr, "Failed to set GPIO line %d configuration: %s\n", init_data->gpio_no, strerror(errno));
			return;
		}
	}
}

void gpio_active(enum gpio_t gpio) {
	struct gpio_runtime_data_t *runtime_data = &gpio_runtime_data[gpio];
	gpiod_line_set_value(runtime_data->gpio_line, 1);
}

void gpio_inactive(enum gpio_t gpio) {
	struct gpio_runtime_data_t *runtime_data = &gpio_runtime_data[gpio];
	gpiod_line_set_value(runtime_data->gpio_line, 0);
}

void gpio_pulse(enum gpio_t gpio, uint16_t microseconds) {
	gpio_active(gpio);
	if (microseconds) {
		struct timespec sleeptime = {
			.tv_sec = 0,
			.tv_nsec = microseconds * 1000,
		};
		while (nanosleep(&sleeptime, &sleeptime) != 0);
	}
	gpio_inactive(gpio);
}

void gpio_set_to(enum gpio_t gpio, bool value) {
	if (value) {
		gpio_active(gpio);
	} else {
		gpio_inactive(gpio);
	}
}

bool gpio_wait_for_input_change(gpio_irq_callback_t callback, unsigned int timeout_millis) {
	struct gpiod_line_bulk lines = GPIOD_LINE_BULK_INITIALIZER;
	for (int i = 0; i < GPIO_COUNT; i++) {
		const struct gpio_init_data_t *init_data = &gpio_init_data[i];
		if (!init_data->is_output) {
			// Is input, wait for IRQ
			struct gpiod_line *line = gpio_runtime_data[i].gpio_line;
			gpiod_line_bulk_add(&lines, line);
		}
	}

	struct timespec timeout = {
		.tv_sec = (timeout_millis / 1000),
		.tv_nsec = (timeout_millis % 1000) * 1000000,
	};

	int wait_result = gpiod_line_event_wait_bulk(&lines, &timeout, &lines);
	if (wait_result == -1) {
		perror("gpiod_line_event_wait_bulk");
		return false;
	} else if (wait_result == 0) {
		return true;
	}

	/* There were results, first get them all */
	int gpio_values[lines.num_lines];
	gpiod_line_get_value_bulk(&lines, gpio_values);

	/* Move them /all/ into the "last_values" member before calling callbacks */
	enum gpio_t gpio_ids[lines.num_lines];
	for (int i = 0; i < lines.num_lines; i++) {
		enum gpio_t gpio_id = get_gpio_for_offset(gpiod_line_offset(lines.lines[i]));
		gpio_ids[i] = gpio_id;
		gpio_runtime_data[gpio_id].last_value = gpio_values[i];
	}

	/* Then call the callbacks */
	for (int i = 0; i < lines.num_lines; i++) {
		callback(gpio_ids[i], gpio_values[i]);
	}
	return true;
}
