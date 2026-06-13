# Makefile for Chapter 6: Partitioning and Synchronization Design

CC = gcc
CFLAGS = -O2 -Wall -Wextra -pthread
TARGETS = hash_table.out allocator_cache.out dining_philosophers.out

.PHONY: all clean test

all: $(TARGETS)

hash_table.out: hash_table.c
	$(CC) $(CFLAGS) -o $@ $<

allocator_cache.out: allocator_cache.c
	$(CC) $(CFLAGS) -o $@ $<

dining_philosophers.out: dining_philosophers.c
	$(CC) $(CFLAGS) -o $@ $<

test: all
	@echo "=== Testing hash_table (sequential) ==="
	./hash_table.out 1 1 100000
	@echo ""
	@echo "=== Testing hash_table (code locking) ==="
	./hash_table.out 2 4 100000
	@echo ""
	@echo "=== Testing hash_table (data locking) ==="
	./hash_table.out 3 4 100000
	@echo ""
	@echo "=== Testing hash_table (rw locking) ==="
	./hash_table.out 4 4 100000
	@echo ""
	@echo "=== Testing allocator_cache (runlength=1) ==="
	./allocator_cache.out 2 1
	@echo ""
	@echo "=== Testing allocator_cache (runlength=6) ==="
	./allocator_cache.out 2 6
	@echo ""
	@echo "=== Testing dining_philosophers ==="
	./dining_philosophers.out

clean:
	rm -f $(TARGETS)
