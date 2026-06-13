# Makefile for perfbook chapter 9: Deferred Processing
# All binaries end with .out to be ignored by .gitignore

CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
LDFLAGS = -pthread

BINS = route_seq.out route_refcnt.out route_seqlock.out route_hazptr.out route_rcu.out

all: $(BINS)

route_seq.out: route_seq.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

route_refcnt.out: route_refcnt.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

route_seqlock.out: route_seqlock.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

route_hazptr.out: route_hazptr.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

route_rcu.out: route_rcu.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: all
	@echo "Testing sequential version..."
	./route_seq.out
	@echo "Testing reference-counted version (expected to crash due to bug)..."
	-./route_refcnt.out || true
	@echo "Testing sequence-locked version (may crash due to bug)..."
	-./route_seqlock.out || true
	@echo "Testing hazard-pointer version..."
	./route_hazptr.out
	@echo "Testing RCU version..."
	./route_rcu.out

clean:
	rm -f $(BINS)

.PHONY: all test clean
