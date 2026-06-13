SRCS = $(wildcard ./*.c)
OUTS = $(SRCS:.c=.out)

CFLAGS = -Wall -g

all: $(OUTS)

%.out:%.o
	$(CC) $(CFLAGS) -o $@ $<

signalfd-iouring.out: signalfd-iouring.o
	$(CC) $(CFLAGS) -luring -o $@ $<

clean:
	rm -f $(OUTS)
