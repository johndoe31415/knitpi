.PHONY: test connect

CFLAGS := -O3 -std=c11 -D_POSIX_C_SOURCE=199309L -D_XOPEN_SOURCE=500
CFLAGS += -Wall -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -Werror=format -Wshadow -Wswitch
#CFLAGS += -Wimplicit-fallthrough
LDFLAGS := -pthread -lgpiod -lpng16

CFLAGS += -ggdb3 -fsanitize=address -fsanitize=undefined -fsanitize=leak -fno-omit-frame-pointer -D_FORTITY_SOURCE=2

SPECIFIC_OBJS :=  test_fncs.o emit_pattern.o knitserver.o
OBJS := \
	argparse.o \
	atomic.o \
	debouncer.o \
	gpio_thread.o \
	isleep.o \
	json.o \
	logging.o \
	membuf.o \
	needles.o \
	pattern.o \
	peripherals_gpio.o \
	peripherals.o \
	peripherals_spi.o \
	pgmopts.o \
	png_reader.o \
	png_writer.o \
	pnm_reader.o \
	server.o \
	sled.o \
	tokenizer.o \
	tools.o

BINARIES := test_fncs emit_pattern knitserver

all: $(BINARIES)

test_fncs: test_fncs.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

emit_pattern: emit_pattern.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

knitserver: knitserver.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(SPECIFIC_OBJS)
	rm -f $(BINARIES)

test: all
	./knitserver --no-hardware -f -vvvvv socket

connect:
	socat - unix-connect:socket

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
