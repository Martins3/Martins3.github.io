SRCS = $(wildcard ./*.c)
OUTS = $(SRCS:.c=.out)

CFLAGS = -Wall -g

GLIB_LIB     = $(shell pkg-config --libs gthread-2.0)
GLIB_INCLUDE = $(shell pkg-config --cflags glib-2.0)

all: $(OUTS)

%.out:%.c
	$(CC) $(CFLAGS) -I . $(GLIB_INCLUDE) -o $@ $< $(GLIB_LIB)

clean:
	rm -f $(OUTS)
