# Makefile for perfbook Chapter 14 code samples
# Binary suffix .out for automatic gitignore

CC := gcc
CFLAGS := -O2 -Wall -Wextra -pthread -D_GNU_SOURCE

PROGS := nbs_stack.out nbs_queue.out rt_timer.out

.PHONY: all clean test

all: $(PROGS)

nbs_stack.out: nbs_stack.c
	$(CC) $(CFLAGS) -o $@ $<

nbs_queue.out: nbs_queue.c
	$(CC) $(CFLAGS) -o $@ $<

rt_timer.out: rt_timer.c
	$(CC) $(CFLAGS) -o $@ $<

test: all
	@echo "=== Testing NBS Stack ==="
	./nbs_stack.out
	@echo ""
	@echo "=== Testing Half-NBS Queue ==="
	./nbs_queue.out
	@echo ""
	@echo "=== Testing RT Timer (may need root for best results) ==="
	./rt_timer.out || true

clean:
	rm -f $(PROGS)
