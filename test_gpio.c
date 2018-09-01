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
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>
#include "peripherals.h"
#include "gpio_thread.h"
#include "isleep.h"
#include "tools.h"
#include "debouncer.h"
#include "sled.h"
#include "needles.h"

struct test_mode_t {
	const char *mode_name;
	const char *description;
	int (*run_test)(int argc, char **argv);
};

static int run_test_wiggle(int argc, char **argv);
static int run_test_spi(int argc, char **argv);
static int run_test_init_oe(int argc, char **argv);
static int run_test_gpio_irqs(int argc, char **argv);
static int run_test_single_output(int argc, char **argv);
static int run_test_interruptible_sleep(int argc, char **argv);
static int run_test_abstime(int argc, char **argv);
static int run_test_debounce(int argc, char **argv);
static int run_test_gpio_irqs_debounced(int argc, char **argv);
static int run_test_sled(int argc, char **argv);
static int run_test_sled_actuate(int argc, char **argv);
static int run_test_needle_name(int argc, char **argv);

static struct timespec last_gpio_event[GPIO_COUNT];

static struct test_mode_t test_modes[] = {
	{
		.mode_name = "wiggle",
		.description = "Wiggles GPIO OE back and forth.",
		.run_test = run_test_wiggle,
	},
	{
		.mode_name = "spi",
		.description = "Transmits data on SPI.",
		.run_test = run_test_spi,
	},
	{
		.mode_name = "init-oe",
		.description = "Assert that initially !OE is disabled.",
		.run_test = run_test_init_oe,
	},
	{
		.mode_name = "gpio-irqs",
		.description = "Test GPIO IRQs.",
		.run_test = run_test_gpio_irqs,
	},
	{
		.mode_name = "gpio-irqs-debounced",
		.description = "Test GPIO IRQs with debouncing.",
		.run_test = run_test_gpio_irqs_debounced,
	},
	{
		.mode_name = "gpio-output-single",
		.description = "Test single output.",
		.run_test = run_test_single_output,
	},
	{
		.mode_name = "isleep",
		.description = "Test interruptible sleep primitive.",
		.run_test = run_test_interruptible_sleep,
	},
	{
		.mode_name = "abstime",
		.description = "Test abstime() primitive.",
		.run_test = run_test_abstime,
	},
	{
		.mode_name = "debounce",
		.description = "Test debouncing",
		.run_test = run_test_debounce,
	},
	{
		.mode_name = "sled",
		.description = "Test sled positioning",
		.run_test = run_test_sled,
	},
	{
		.mode_name = "sled-act",
		.description = "Test sled positioning/actuation",
		.run_test = run_test_sled_actuate,
	},
	{
		.mode_name = "needle-name",
		.description = "Test needle names",
		.run_test = run_test_needle_name,
	},
};

static int run_test_wiggle(int argc, char **argv) {
	printf("Wiggle test.\n");
	gpio_init();
	while (true) {
		gpio_active(GPIO_74HC595_OE);
		usleep(500 * 1000);
		gpio_inactive(GPIO_74HC595_OE);
		usleep(500 * 1000);
	}
	return 0;
}

static int run_test_spi(int argc, char **argv) {
	printf("SPI test.\n");
	all_peripherals_init();
	while (true) {
		uint8_t data[2];
		memset(data, 0, sizeof(data));
		spi_send(SPI_74HC595, data, sizeof(data));
		gpio_active(GPIO_74HC595_OE);
		usleep(500 * 1000);

		memset(data, 0xff, sizeof(data));
		spi_send(SPI_74HC595, data, sizeof(data));
		gpio_active(GPIO_74HC595_OE);
		usleep(1000 * 1000);
	}
	return 0;
}

static int run_test_init_oe(int argc, char **argv) {
	printf("SPI test. OE is INACTIVE, but all bits are set.\n");
	all_peripherals_init();

	uint8_t data[2] = { 0xff, 0xff };
	spi_send(SPI_74HC595, data, sizeof(data));
	while (true) {
		sleep(1);
	}
	return 0;
}

static void gpio_callback_raw(enum gpio_t gpio, const struct timespec *ts, bool value) {
	int64_t tdiff = timespec_diff(ts, &last_gpio_event[gpio]);
	memcpy(&last_gpio_event[gpio], ts, sizeof(struct timespec));

	const struct gpio_init_data_t *gpio_data = gpio_get_init_data(gpio);
	fprintf(stderr, "%s: %lu.%09lu %s (%.1f ms)\n", gpio_data->name, ts->tv_sec, ts->tv_nsec, value ? "Active" : "Inactive", tdiff / 1e6);
}

static void gpio_callback_debounced(enum gpio_t gpio, const struct timespec *ts, bool value) {
	int64_t tdiff = timespec_diff(ts, &last_gpio_event[gpio]);
	memcpy(&last_gpio_event[gpio], ts, sizeof(struct timespec));

	const struct gpio_init_data_t *gpio_data = gpio_get_init_data(gpio);
	fprintf(stderr, "Debounced %s: %lu.%09lu %s (%.1f ms)\n", gpio_data->name, ts->tv_sec, ts->tv_nsec, value ? "Active" : "Inactive", tdiff / 1e6);
}

static int run_test_gpio_irqs(int argc, char **argv) {
	printf("Testing GPIO IRQs.\n");
	all_peripherals_init();
	start_gpio_thread(gpio_callback_raw, true);
	while (true) {
		sleep(1);
	}
	return 0;
}

static int run_test_gpio_irqs_debounced(int argc, char **argv) {
	printf("Testing GPIO IRQs with debouncing.\n");
	all_peripherals_init();
	start_debouncer_thread(gpio_callback_debounced);
	start_gpio_thread(debouncer_input, true);
	while (true) {
		sleep(1);
	}
	return 0;
}

static int run_test_single_output(int argc, char **argv) {
	all_peripherals_init();
	uint8_t zero_pattern[] = { 0, 0 };
	spi_send(SPI_74HC595, zero_pattern, sizeof(zero_pattern));
	gpio_active(GPIO_74HC595_OE);

	int active = 0;
	while (true) {
		uint8_t pattern[2];
		memset(pattern, 0, sizeof(pattern));
		pattern[active / 8] |= (1 << (active % 8));
		printf("Active: %d (pattern %02x %02x)", active, pattern[0], pattern[1]);
		fflush(stdout);
		spi_send(SPI_74HC595, pattern, sizeof(pattern));

		char buf[16];
		if (!fgets(buf, sizeof(buf), stdin)) {
			perror("fgets");
			break;
		}
		active = (active + 1) % 16;
	}
	return 0;
}

static void *annoying_sleep_interruptor(void *vsleeper) {
	struct isleep_t *sleeper = (struct isleep_t*)vsleeper;
	while (true) {
		sleep(3);
		isleep_interrupt(sleeper);
	}
	return NULL;
}

static int run_test_interruptible_sleep(int argc, char **argv) {
	struct isleep_t sleeper = ISLEEP_INITIALIZER;
	start_detached_thread(annoying_sleep_interruptor, &sleeper);
	while (true) {
		struct timespec before;
		get_abs_timespec_offset(&before, 0);
		bool interrupted = isleep(&sleeper, 1234);
		struct timespec after;
		get_abs_timespec_offset(&after, 0);
		int64_t nanodiff = timespec_diff(&after, &before);
		fprintf(stderr, "Slept %s: %" PRIu64 " ms\n", interrupted ? "INTERRUPTED" : "normally", nanodiff / 1000000);
	}
}

static int run_test_abstime(int argc, char **argv) {
	while (true) {
		struct timespec past, now, future;
		get_abs_timespec_offset(&past, -5555);
		get_abs_timespec_offset(&now, 0);
		get_abs_timespec_offset(&future, 5555);

		if (past.tv_nsec >= 1000000000) {
			fprintf(stderr, "past timespec error: %lu %lu\n", past.tv_sec, past.tv_nsec);
			return 1;
		}
		if (now.tv_nsec >= 1000000000) {
			fprintf(stderr, "now timespec error: %lu %lu\n", now.tv_sec, now.tv_nsec);
			return 1;
		}
		if (future.tv_nsec >= 1000000000) {
			fprintf(stderr, "future timespec error: %lu %lu\n", future.tv_sec, future.tv_nsec);
			return 1;
		}

		double fpast = past.tv_sec + (past.tv_nsec / 1e9);
		double fnow = now.tv_sec + (now.tv_nsec / 1e9);
		double ffuture = future.tv_sec + (future.tv_nsec / 1e9);
		fprintf(stderr, "%.3f %.3f\n", fnow - fpast, ffuture - fnow);

		usleep(100 * 1000);
	}
}

static int run_test_debounce(int argc, char **argv) {
	start_debouncer_thread(gpio_callback_debounced);

	struct timespec now;
	get_timespec_now(&now);
	debouncer_input(2, &now, false);

	usleep(10 * 1000);

	get_timespec_now(&now);
	debouncer_input(2, &now, false);

	usleep(10 * 1000);

	get_timespec_now(&now);
	debouncer_input(2, &now, true);

	usleep(10 * 1000);

	get_timespec_now(&now);
	debouncer_input(2, &now, false);

	usleep(10 * 1000);

	get_timespec_now(&now);
	fprintf(stderr, "Final change: %lu.%09lu\n", now.tv_sec, now.tv_nsec);
	debouncer_input(2, &now, true);

	usleep(100 * 1000);
}

static void sled_callback(int position, bool left_to_right) {
	fprintf(stderr, "Sled at: %3d %s\n", position, left_to_right ? "->" : "<-");
}

static int run_test_sled(int argc, char **argv) {
	printf("Testing sled positioning.\n");
	all_peripherals_init();
	sled_set_callback(sled_callback);
	start_debouncer_thread(sled_input);
	start_gpio_thread(debouncer_input, true);
	while (true) {
		sleep(1);
	}
}

static int knit_needle_id = 104;

static bool sled_around_needle_id(unsigned int position, unsigned int needle_id) {
//	return (position / 8) == (needle_id / 8);
	return true;
}

static void actuate_solenoids_for_needle(uint8_t *spi_data, unsigned int needle_id) {
	int bit = needle_id % 16;
	//if (phase) {
		bit = (bit + 8) % 16;
	//}
	spi_data[bit / 8] |= (1 << (bit % 8));
}

static void sled_actuation_callback(int position, bool left_to_right) {
	uint8_t spi_data[] = { 0, 0 };

	if (sled_around_needle_id(position, knit_needle_id)) {
		actuate_solenoids_for_needle(spi_data, knit_needle_id);
	}

	if (spi_data[0] || spi_data[1]) {
		fprintf(stderr, "KNIT %02x %02x at sled pos %d\n", spi_data[0], spi_data[1], position);
	}
	spi_send(SPI_74HC595, spi_data, sizeof(spi_data));
}

static int run_test_sled_actuate(int argc, char **argv) {
	printf("Testing sled positioning and actuation.\n");
	all_peripherals_init();
	sled_set_callback(sled_actuation_callback);
	start_debouncer_thread(sled_input);
	start_gpio_thread(debouncer_input, true);
	spi_clear(SPI_74HC595, 2);
	gpio_active(GPIO_74HC595_OE);
	while (true) {
		char name[32];
		needle_pos_to_text(name, knit_needle_id);
		printf("Knit needle ID: %s (%d)\n", name, knit_needle_id);
		char buf[16];
		if (!fgets(buf, sizeof(buf), stdin)) {
			perror("fgets");
			break;
		}
		knit_needle_id++;
	}
}

static int run_test_needle_name(int argc, char **argv) {
	for (int i = 0; i < 200; i++) {
		char name[32];
		needle_pos_to_text(name, i);
		fprintf(stderr, "%3d: %s\n", i, name);
	}
	for (int i = 100; i >= 1; i--) {
		int needle_pos = needle_text_to_pos('y', i);
		fprintf(stderr, "Y %-3d: %3d\n", i, needle_pos);
	}
	for (int i = 1; i <= 100; i++) {
		int needle_pos = needle_text_to_pos('g', i);
		fprintf(stderr, "G %-3d: %3d\n", i, needle_pos);
	}
	return 0;
}

static void show_syntax(const char *errmsg) {
	if (errmsg) {
		fprintf(stderr, "error: %s\n", errmsg);
		fprintf(stderr, "\n");
	}
	for (int i = 0; i < sizeof(test_modes) / sizeof(struct test_mode_t); i++) {
		fprintf(stderr, "    %-10s %s\n", test_modes[i].mode_name, test_modes[i].description);
	}
}

int main(int argc, char **argv) {
	if (argc < 2) {
		show_syntax("Test mode not given.");
		return 1;
	}

	for (int i = 0; i < sizeof(test_modes) / sizeof(struct test_mode_t); i++) {
		if (!strcmp(test_modes[i].mode_name, argv[1])) {
			return test_modes[i].run_test(argc, argv);
		}
	}

	show_syntax("Unknown test mode given.");
	return 1;
}
