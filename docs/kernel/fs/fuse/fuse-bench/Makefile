CC ?= cc
CFLAGS ?= -O2 -g
CPPFLAGS += -D_GNU_SOURCE -I./src
WARNINGS := -Wall -Wextra -Werror -std=c11

COMMON_OBJS := src/common.o
DAEMON_OBJS := src/fuse-bench.o $(COMMON_OBJS)
BENCH_OBJS := src/bench.o $(COMMON_OBJS)
TEST_OBJS := tests/test_common.o $(COMMON_OBJS)

.PHONY: all test clean

all: fuse-bench.out bench.out

fuse-bench.out: $(DAEMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

bench.out: $(BENCH_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_common.out: $(TEST_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -c -o $@ $<

test: test_common.out
	./test_common.out

clean:
	rm -f src/*.o tests/*.o ./*.out
