CC := gcc
CFLAGS := -std=gnu11 -Wall -Wextra -Wpedantic -g
LDLIBS := -pthread

OUTS := userfault.out

.PHONY: all clean test

all: $(OUTS)

userfault.out: userfault.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

test: userfault.out
	./userfault.out all

clean:
	rm -f $(OUTS)
