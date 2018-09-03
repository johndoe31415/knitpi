.PHONY: test

CFLAGS := -O3 -std=c11 -D_POSIX_C_SOURCE=199309L -D_XOPEN_SOURCE=500
CFLAGS += -Wall -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -Werror=format -Wshadow -Wswitch
#CFLAGS += -Wimplicit-fallthrough
LDFLAGS := -pthread -lgpiod -lpng16

CFLAGS += -ggdb3 -fsanitize=address -fsanitize=undefined -fsanitize=leak -fno-omit-frame-pointer -D_FORTITY_SOURCE=2

OBJS :=  test_fncs.o emit_pattern.o knitserver.o
OBJS += peripherals.o peripherals_gpio.o peripherals_spi.o gpio_thread.o tools.o debouncer.o isleep.o sled.o needles.o pnm_reader.o argparse.o json.o pattern.o png_writer.o logging.o png_reader.o membuf.o atomic.o
BINARIES := test_fncs emit_pattern knitserver

all: $(BINARIES)

test_fncs: test_fncs.o peripherals.o peripherals_gpio.o peripherals_spi.o gpio_thread.o tools.o debouncer.o isleep.o sled.o needles.o pnm_reader.o pattern.o png_writer.o membuf.o logging.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

emit_pattern: emit_pattern.o peripherals.o peripherals_gpio.o peripherals_spi.o gpio_thread.o tools.o debouncer.o isleep.o sled.o needles.o pnm_reader.o pattern.o logging.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

knitserver: knitserver.o peripherals.o peripherals_gpio.o peripherals_spi.o gpio_thread.o tools.o debouncer.o isleep.o sled.o needles.o pnm_reader.o argparse.o json.o pattern.o png_writer.o logging.o png_reader.o membuf.o atomic.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(BINARIES)

test: all
	./knitserver --no-hardware -f -vvvvv socket

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
