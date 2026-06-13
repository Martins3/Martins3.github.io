# Makefile for Chapter 13: Putting It All Together
# All binaries end with .out to be ignored by .gitignore

CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -std=c11
LDFLAGS = -pthread

TARGETS = sref_demo.out kref_demo.out seqlock_demo.out \
          correlated_fields.out count_end_rcu.out \
          atomic_move.out retrigger_gp.out microopt_demo.out

all: $(TARGETS)

sref_demo.out: sref_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

kref_demo.out: kref_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

seqlock_demo.out: seqlock_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

correlated_fields.out: correlated_fields.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

count_end_rcu.out: count_end_rcu.c urcu_simple.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

atomic_move.out: atomic_move.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

retrigger_gp.out: retrigger_gp.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

microopt_demo.out: microopt_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: all
	@echo "[TEST] sref_demo"
	./sref_demo.out
	@echo "[TEST] kref_demo"
	./kref_demo.out
	@echo "[TEST] seqlock_demo"
	./seqlock_demo.out
	@echo "[TEST] correlated_fields"
	./correlated_fields.out
	@echo "[TEST] count_end_rcu"
	./count_end_rcu.out
	@echo "[TEST] atomic_move"
	./atomic_move.out
	@echo "[TEST] retrigger_gp"
	./retrigger_gp.out
	@echo "[TEST] microopt_demo"
	./microopt_demo.out
	@echo "[OK] All tests passed"

clean:
	rm -f $(TARGETS)

.PHONY: all test clean
