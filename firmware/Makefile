.PHONY: test connect

ARCH := $(shell uname -p)
ifeq ($(ARCH),unknown)
# Running on Raspberry Pi
DEVELOPMENT := 0
else
DEVELOPMENT := 1
endif


CFLAGS := -O3 -std=c11 -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=500
CFLAGS += -Wall -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -Werror=format -Wshadow -Wswitch
#CFLAGS += -Wimplicit-fallthrough
LDFLAGS := -pthread -lgpiod -lpng16

ifeq ($(DEVELOPMENT),1)
CFLAGS += -ggdb3 -fsanitize=address -fsanitize=undefined -fsanitize=leak -fno-omit-frame-pointer -D_FORTITY_SOURCE=2
TEST_FLAGS += --no-hardware
endif

SPECIFIC_OBJS :=  test_fncs.o knitserver.o
OBJS := \
	argparse.o \
	atomic.o \
	debouncer.o \
	gpio_thread.o \
	isleep.o \
	json.o \
	knitcore.o \
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

BINARIES := test_fncs knitserver

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
	./knitserver $(TEST_FLAGS) -f -vvvvv socket

connect:
	socat - unix-connect:socket

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
