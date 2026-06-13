# Makefile for Chapter 2 Introduction demos
# SPDX-License-Identifier: GPL-2.0

CC := gcc
CFLAGS := -O2 -Wall -Wextra -pthread
LDFLAGS := -pthread

TARGETS := work_partition.out access_control.out resource_partition.out seq_optimize.out

.PHONY: all clean run

all: $(TARGETS)

work_partition.out: work_partition.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

access_control.out: access_control.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

resource_partition.out: resource_partition.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

seq_optimize.out: seq_optimize.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

run: all
	@echo "=== work_partition ==="
	@./work_partition.out
	@echo ""
	@echo "=== access_control ==="
	@./access_control.out
	@echo ""
	@echo "=== resource_partition ==="
	@./resource_partition.out
	@echo ""
	@echo "=== seq_optimize ==="
	@./seq_optimize.out

clean:
	rm -f $(TARGETS)
