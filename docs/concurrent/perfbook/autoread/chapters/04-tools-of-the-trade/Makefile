# Makefile for Chapter 4: Tools of the Trade
# All binaries end with .out to be ignored by .gitignore

CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread

TARGETS = forkjoin.out pcreate.out lock.out rwlockscale.out \
          atomic.out compiler_barrier.out data_race_demo.out

.PHONY: all clean test

all: $(TARGETS)

forkjoin.out: forkjoin.c
	$(CC) $(CFLAGS) -o $@ $<

pcreate.out: pcreate.c
	$(CC) $(CFLAGS) -o $@ $<

lock.out: lock.c
	$(CC) $(CFLAGS) -o $@ $<

rwlockscale.out: rwlockscale.c
	$(CC) $(CFLAGS) -o $@ $<

atomic.out: atomic.c
	$(CC) $(CFLAGS) -o $@ $<

compiler_barrier.out: compiler_barrier.c
	$(CC) $(CFLAGS) -o $@ $<

data_race_demo.out: data_race_demo.c
	$(CC) $(CFLAGS) -o $@ $<

test: all
	@echo "=== Testing forkjoin ==="
	./forkjoin.out 2
	@echo ""
	@echo "=== Testing pcreate ==="
	./pcreate.out
	@echo ""
	@echo "=== Testing lock ==="
	./lock.out
	@echo ""
	@echo "=== Testing atomic ==="
	./atomic.out
	@echo ""
	@echo "=== Testing compiler_barrier ==="
	./compiler_barrier.out
	@echo ""
	@echo "=== Testing data_race_demo ==="
	./data_race_demo.out
	@echo ""
	@echo "=== Testing rwlockscale (8 threads, 10 hold loops, 0 think loops) ==="
	./rwlockscale.out 8 10 0

clean:
	rm -f $(TARGETS)
