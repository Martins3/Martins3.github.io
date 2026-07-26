CC ?= cc
CFLAGS ?= -O2 -g
WARNINGS := -Wall -Wextra -Werror -std=c11

DEMOS := pipe-basic.out named-fifo.out splice-copy.out vmsplice-basic.out tee-duplicate.out

.PHONY: all clean run

all: $(DEMOS)

%.out: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

run: all
	./pipe-basic.out
	./named-fifo.out
	./splice-copy.out
	./vmsplice-basic.out
	./tee-duplicate.out

clean:
	rm -f *.out
