CC = gcc
CFLAGS = -Wall -O2 -pthread -D_GNU_SOURCE

TARGETS = process_ownership.out partial_ownership.out function_shipping.out designated_thread.out privatization.out

all: $(TARGETS)

%.out: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
