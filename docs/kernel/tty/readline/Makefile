CC = cc
CPPFLAGS += $(shell pkg-config --cflags readline)
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
LDLIBS += $(shell pkg-config --libs readline)

.PHONY: all run
all: demo.out

demo.out: demo.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

run: demo.out
	./demo.out
