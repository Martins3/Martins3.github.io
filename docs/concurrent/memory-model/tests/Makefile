CC ?= cc
CFLAGS := -O2 -Wall -Wextra -pthread

TESTS := sb mp lb ll wr ss
BINS := $(foreach t,$(TESTS),$(t)-nofence.out $(t)-fence.out)

all: $(BINS)

%-nofence.out: %.c common.h
	$(CC) $(CFLAGS) -o $@ $<

%-fence.out: %.c common.h
	$(CC) $(CFLAGS) -DUSE_FENCE -o $@ $<

clean:
	rm -f $(BINS)

.PHONY: all clean
