# Makefile for perfbook Chapter 7: Locking demos
# Binary suffix: .out (auto-ignored by git)

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pthread -D_GNU_SOURCE
LDFLAGS = -pthread

TARGETS = deadlock_demo.out livelock_demo.out xchglock_demo.out rwlock_demo.out existence_guarantee.out

.PHONY: all clean test

all: $(TARGETS)

deadlock_demo.out: deadlock_demo.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

livelock_demo.out: livelock_demo.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

xchglock_demo.out: xchglock_demo.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

rwlock_demo.out: rwlock_demo.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

existence_guarantee.out: existence_guarantee.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test: all
	@echo "===== Running deadlock_demo ====="
	./deadlock_demo.out
	@echo ""
	@echo "===== Running livelock_demo ====="
	./livelock_demo.out
	@echo ""
	@echo "===== Running xchglock_demo ====="
	./xchglock_demo.out
	@echo ""
	@echo "===== Running rwlock_demo ====="
	./rwlock_demo.out
	@echo ""
	@echo "===== Running existence_guarantee ====="
	./existence_guarantee.out

clean:
	rm -f $(TARGETS)
