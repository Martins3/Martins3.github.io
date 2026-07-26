SRCS = $(wildcard ./*.S)
OUTS = $(SRCS:.S=.out)

$(info $(SRCS))

CFLAGS = -Wall -g -O0
CFLAGS += -march=armv8.3-a


all: $(OUTS)

lib.o:lib.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.out:%.S lib.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OUTS)

dis:
	gdb -quiet 4.out -ex "disass add" -ex "q"
	gdb -quiet 4.out -ex "disass non_leaf_add" -ex "q"
