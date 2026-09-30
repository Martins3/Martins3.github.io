SRCS = $(wildcard ./*.c)
OUTS = $(SRCS:.c=.out)

PKG_CONFIG ?= pkg-config
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags liburing)
CFLAGS += -Wall -g
LDLIBS += $(shell $(PKG_CONFIG) --libs liburing)

all: $(OUTS)

%.out: %.o
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

register-buffer-pin.out: register-buffer-pin.o
	$(CC) -o $@ $<

clean:
	rm -f $(OUTS)
