CC = gcc
CFLAGS = -O2 -Wall -Wextra -pthread

TARGETS = hash_fixed.out hash_rcu.out hash_resize.out

all: $(TARGETS)

hash_fixed.out: hash_fixed.c
	$(CC) $(CFLAGS) -o $@ $<

hash_rcu.out: hash_rcu.c
	$(CC) $(CFLAGS) -o $@ $<

hash_resize.out: hash_resize.c
	$(CC) $(CFLAGS) -o $@ $<

test: all
	@echo "=== Testing hash_fixed.out ==="
	./hash_fixed.out
	@echo "=== Testing hash_rcu.out ==="
	./hash_rcu.out
	@echo "=== Testing hash_resize.out ==="
	./hash_resize.out

.PHONY: clean test

clean:
	rm -f $(TARGETS)
