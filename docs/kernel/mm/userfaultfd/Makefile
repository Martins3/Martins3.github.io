CC := gcc
CFLAGS := -Wall -g
LDLIBS := -lpthread -lm

SRCS := $(wildcard *.c)
OUTS := $(SRCS:.c=.out)

.PHONY: all clean

all: $(OUTS)

%.out: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(OUTS)
