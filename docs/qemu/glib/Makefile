#!/usr/bin/make
GLIB_LIB     = $(shell pkg-config --libs gthread-2.0)
GLIB_INCLUDE = $(shell pkg-config --cflags glib-2.0)

# define the compiler
CC=gcc

# define the libs, include lm for completeness
LIBS=$(GLIB_LIB) -lm

# define the build flags, turn on all warnings and setup for GDB
DEBUG_FLAGS=-ggdb -Wall
INCLUDE=-I . $(GLIB_INCLUDE)
CFLAGS=$(DEBUG_FLAGS) $(INCLUDE)

# define binaries so we have a list for build rules and cleanup
BINS=default-context

# build test programs
all: $(BINS)

# simple build rule for examples, source is same name + ".c"
%: %.c
	$(CC) $(CFLAGS) -o $@ $<  $(LIBS)

# cleanup:
clean::
	rm -f $(BINS) *.o
