SRCS = $(wildcard ./*.c)
OUTS = $(SRCS:.c=.out)

CFLAGS = -Wall -g -pthread

all: $(OUTS)

%.out:%.o
	@$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(OUTS)
