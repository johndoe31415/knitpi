.PHONY: test

CFLAGS := -O3 -std=c11 -D_POSIX_C_SOURCE=199309L -D_XOPEN_SOURCE=500
CFLAGS += -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -Werror=format -Wimplicit-fallthrough -Wshadow
LDFLAGS := -pthread -lgpiod
OBJS := peripherals.o peripherals_gpio.o peripherals_spi.o test_gpio.o gpio_thread.o
BINARIES := test_gpio

all: $(BINARIES)

test_gpio: peripherals.o peripherals_gpio.o peripherals_spi.o test_gpio.o gpio_thread.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(BINARIES)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
