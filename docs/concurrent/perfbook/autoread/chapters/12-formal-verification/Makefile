# Makefile for Chapter 12: Formal Verification code samples
# All binaries end with .out to be gitignored automatically

CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread

TARGETS = race_increment.out atomic_increment.out spinlock_demo.out \
          dynticks_like.out litmus_sb.out

.PHONY: all clean test

all: $(TARGETS)

race_increment.out: race_increment.c
	$(CC) $(CFLAGS) -o $@ $<

atomic_increment.out: atomic_increment.c
	$(CC) $(CFLAGS) -o $@ $<

spinlock_demo.out: spinlock_demo.c
	$(CC) $(CFLAGS) -o $@ $<

dynticks_like.out: dynticks_like.c
	$(CC) $(CFLAGS) -o $@ $<

litmus_sb.out: litmus_sb.c
	$(CC) $(CFLAGS) -o $@ $<

test: all
	@echo "=== Testing atomic_increment ==="
	./atomic_increment.out
	@echo ""
	@echo "=== Testing spinlock_demo ==="
	./spinlock_demo.out
	@echo ""
	@echo "=== Testing dynticks_like ==="
	./dynticks_like.out
	@echo ""
	@echo "=== Testing litmus_sb ==="
	./litmus_sb.out
	@echo ""
	@echo "=== Testing race_increment (may fail assertions) ==="
	./race_increment.out || true

clean:
	rm -f $(TARGETS)
