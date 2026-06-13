# Makefile for perfbook Chapter 11 (Validation) code examples
# Binary suffix .out for automatic .gitignore

CC = gcc
CFLAGS = -O2 -Wall -Wextra -pthread -std=c11 -D_GNU_SOURCE
LDFLAGS = -pthread -lm

TARGETS = count_racey.out assertion_demo.out getrusage_demo.out \
	  binomial_test.out datablows.out heisenbug_demo.out

.PHONY: all clean test

all: $(TARGETS)

count_racey.out: count_racey.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

assertion_demo.out: assertion_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

getrusage_demo.out: getrusage_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

binomial_test.out: binomial_test.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

datablows.out: datablows.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

heisenbug_demo.out: heisenbug_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: all
	@echo "=== count_racey.out (no delay) ==="
	./count_racey.out 4 1000000 0
	@echo ""
	@echo "=== count_racey.out (with delay) ==="
	./count_racey.out 4 10000 1
	@echo ""
	@echo "=== assertion_demo.out ==="
	./assertion_demo.out
	@echo ""
	@echo "=== getrusage_demo.out ==="
	./getrusage_demo.out 50000000
	@echo ""
	@echo "=== binomial_test.out ==="
	./binomial_test.out 0.1 0.99
	@echo ""
	@echo "=== datablows.out ==="
	echo "1 10.0 10.1 10.2 50.0 100.0" | ./datablows.out
	@echo ""
	@echo "=== heisenbug_demo.out (no delay) ==="
	./heisenbug_demo.out 4 100000 0
	@echo ""
	@echo "=== heisenbug_demo.out (with delay) ==="
	./heisenbug_demo.out 4 1000 1000

clean:
	rm -f $(TARGETS)
