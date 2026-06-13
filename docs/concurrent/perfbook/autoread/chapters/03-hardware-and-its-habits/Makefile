# Makefile for Chapter 3: Hardware and its Habits demos
# SPDX-License-Identifier: GPL-2.0

CC = gcc
CFLAGS = -O2 -Wall -Wextra -pthread
LDFLAGS = -pthread

# All demo binaries end with .out as required
TARGETS = cache_miss_demo.out false_sharing_demo.out \
	  atomic_overhead_demo.out branch_prediction_demo.out \
	  memory_barrier_demo.out read_mostly_demo.out

.PHONY: all clean test

all: $(TARGETS)

cache_miss_demo.out: cache_miss_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

false_sharing_demo.out: false_sharing_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

atomic_overhead_demo.out: atomic_overhead_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

branch_prediction_demo.out: branch_prediction_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

memory_barrier_demo.out: memory_barrier_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

read_mostly_demo.out: read_mostly_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: all
	@echo "=== Running cache_miss_demo ==="
	./cache_miss_demo.out
	@echo ""
	@echo "=== Running false_sharing_demo ==="
	./false_sharing_demo.out
	@echo ""
	@echo "=== Running atomic_overhead_demo ==="
	./atomic_overhead_demo.out
	@echo ""
	@echo "=== Running branch_prediction_demo ==="
	./branch_prediction_demo.out
	@echo ""
	@echo "=== Running memory_barrier_demo ==="
	./memory_barrier_demo.out
	@echo ""
	@echo "=== Running read_mostly_demo ==="
	./read_mostly_demo.out

clean:
	rm -f $(TARGETS)
