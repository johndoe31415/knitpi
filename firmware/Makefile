.PHONY: test

CFLAGS := -O3 -std=c11 -D_POSIX_C_SOURCE=199309L -D_XOPEN_SOURCE=500
CFLAGS += -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -Werror=format -Wshadow -Wswitch
#CFLAGS += -Wimplicit-fallthrough
LDFLAGS := -pthread -lgpiod
OBJS :=  test_gpio.o emit_pattern.o knitserver.o
OBJS += peripherals.o peripherals_gpio.o peripherals_spi.o gpio_thread.o tools.o debouncer.o isleep.o sled.o needles.o pnm_reader.o argparse.o
BINARIES := test_gpio emit_pattern knitserver

all: $(BINARIES)

test_gpio: test_gpio.o peripherals.o peripherals_gpio.o peripherals_spi.o gpio_thread.o tools.o debouncer.o isleep.o sled.o needles.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

emit_pattern: emit_pattern.o peripherals.o peripherals_gpio.o peripherals_spi.o gpio_thread.o tools.o debouncer.o isleep.o sled.o needles.o pnm_reader.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

knitserver: knitserver.o peripherals.o peripherals_gpio.o peripherals_spi.o gpio_thread.o tools.o debouncer.o isleep.o sled.o needles.o pnm_reader.o argparse.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(BINARIES)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
