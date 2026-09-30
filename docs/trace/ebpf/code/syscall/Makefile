# SPDX-License-Identifier: GPL-2.0-only

CC ?= cc
CFLAGS ?= -O2 -g -Wall -Wextra -Wpedantic
CPPFLAGS ?=
LDFLAGS ?=

PROGRAM := pcpu-populate-demo.out

.PHONY: all clean run

all: $(PROGRAM)

$(PROGRAM): pcpu-populate-demo.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@

clean:
	rm -f $(PROGRAM)

run: $(PROGRAM)
	sudo ./$(PROGRAM) $(ARGS)
