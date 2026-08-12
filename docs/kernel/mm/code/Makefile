FILTER_OUT = $(foreach v,$(2),$(if $(findstring $(1),$(v)),,$(v)))
ALL_SRCS := $(wildcard ./*.c)
SRCS := $(call FILTER_OUT,lib, $(ALL_SRCS))
OUTS := $(SRCS:.c=.out)

$(info $(OUTS))

CFLAGS = -Wall -g
# -laio

all: $(OUTS) mmap_sem_get.out

%.out:%.o
	$(CC) $(CFLAGS) -o $@ $<

rmap.out:rmap.o lib.c
	$(CC) $(CFLAGS) -o $@ $^

madvise-locked-page.out : madvise-locked-page.c
	$(CC) $(CFLAGS) -laio -o $@ $^

mmap_sem_get.out: mmap_sem_get.cc
	g++ -o $@ $^

numa.out: numa.o
	$(CC) -lnuma -o $@ $^

clean:
	rm -f $(OUTS)
