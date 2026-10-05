CC = gcc
CFLAGS = -std=c11 -O0 -g -Wall -Wextra -Werror

.PHONY: all check
all: session.out job-control.out

session.out: session.c
	$(CC) $(CFLAGS) $< -o $@

job-control.out: job-control.c
	$(CC) $(CFLAGS) $< -o $@ -lutil

check: all
	./session.out
	./job-control.out
	./snapshot.py
	ruff check snapshot.py
	ruff format --check snapshot.py
