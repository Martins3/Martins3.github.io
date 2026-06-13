# Makefile for Chapter 16: Ease of Use demos
# Binary suffix is .out to match .gitignore rules

CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -std=c11

TARGETS = lock_order_demo.out api_safety_levels.out

.PHONY: all clean test run

all: $(TARGETS)

lock_order_demo.out: lock_order_demo.c
	$(CC) $(CFLAGS) -o $@ $<

api_safety_levels.out: api_safety_levels.c
	$(CC) $(CFLAGS) -o $@ $<

run: all
	@echo "Running api_safety_levels.out ..."
	@echo "test input line" | ./api_safety_levels.out
	@echo ""
	@echo "Running lock_order_demo.out ..."
	./lock_order_demo.out

test: run

clean:
	rm -f $(TARGETS)
