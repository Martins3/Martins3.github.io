# Makefile for perfbook Chapter 18 demo programs
# All binaries end with .out to be ignored by .gitignore

CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -pthread -std=c11
LDFLAGS := -pthread

TARGETS := counter_comparison.out lockdep_demo.out right_tool_demo.out

.PHONY: all test clean

all: $(TARGETS)

counter_comparison.out: counter_comparison.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

lockdep_demo.out: lockdep_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

right_tool_demo.out: right_tool_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: all
	@echo "========================================"
	@echo "Running counter_comparison.out"
	@echo "========================================"
	./counter_comparison.out
	@echo ""
	@echo "========================================"
	@echo "Running lockdep_demo.out"
	@echo "========================================"
	./lockdep_demo.out
	@echo ""
	@echo "========================================"
	@echo "Running right_tool_demo.out"
	@echo "========================================"
	./right_tool_demo.out

clean:
	rm -f $(TARGETS)
