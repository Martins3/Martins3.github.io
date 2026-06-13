# Makefile for perfbook chapter 5 Counting demos

CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -pthread -D_GNU_SOURCE
LDFLAGS := -pthread

TARGETS := count_nonatomic.out count_atomic.out count_perthread.out \
           count_eventual.out count_limit.out

.PHONY: all clean test

all: $(TARGETS)

count_nonatomic.out: count_nonatomic.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

count_atomic.out: count_atomic.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

count_perthread.out: count_perthread.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

count_eventual.out: count_eventual.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

count_limit.out: count_limit.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: all
	@echo "=== Test 1: Non-atomic counter (expect lost counts) ==="
	./count_nonatomic.out
	@echo ""
	@echo "=== Test 2: Atomic counter (expect correct, slower) ==="
	./count_atomic.out
	@echo ""
	@echo "=== Test 3: Per-thread counter (expect correct, fast) ==="
	./count_perthread.out
	@echo ""
	@echo "=== Test 4: Eventually consistent counter ==="
	./count_eventual.out
	@echo ""
	@echo "=== Test 5: Approximate limit counter ==="
	./count_limit.out

clean:
	rm -f $(TARGETS)
