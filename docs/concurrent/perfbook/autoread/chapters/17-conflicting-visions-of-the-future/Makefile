# Makefile for Chapter 17 demos
# SPDX-License-Identifier: GPL-2.0

CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -pthread -mrtm
LDFLAGS := -pthread

TARGETS := seqlock_demo.out tsx_demo.out priority_boost_demo.out

.PHONY: all clean test

all: $(TARGETS)

seqlock_demo.out: seqlock_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

tsx_demo.out: tsx_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

priority_boost_demo.out: priority_boost_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: all
	@echo "=== Testing seqlock_demo ==="
	./seqlock_demo.out
	@echo ""
	@echo "=== Testing tsx_demo ==="
	./tsx_demo.out
	@echo ""
	@echo "=== Testing priority_boost_demo ==="
	./priority_boost_demo.out

clean:
	rm -f $(TARGETS)
