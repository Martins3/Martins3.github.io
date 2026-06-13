# RDMA Demo Makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = -libverbs -lpthread

# Try to find rdma-core
RDMA_INCLUDE ?= /nix/store/55srcgw80lncngmvs0p0m98695wc1hkb-rdma-core-60.0-dev/include
RDMA_LIB ?= /nix/store/ln64vgzhlx1k4h5gqlxkmwprfz7xwlqs-rdma-core-60.0/lib

CFLAGS += -I$(RDMA_INCLUDE)
LDFLAGS += -L$(RDMA_LIB) -Wl,-rpath,$(RDMA_LIB)

TARGETS = rdma_pingpong_poll.out rdma_pingpong_event.out rdma_write_read.out

.PHONY: all clean run-server run-client

all: $(TARGETS)

rdma_pingpong_poll.out: rdma_pingpong.c
	$(CC) $(CFLAGS) -DUSE_POLLING=1 -o $@ $< $(LDFLAGS)

rdma_pingpong_event.out: rdma_pingpong.c
	$(CC) $(CFLAGS) -DUSE_POLLING=0 -o $@ $< $(LDFLAGS)

rdma_write_read.out: rdma_write_read.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

rdma_atomic.out: rdma_atomic.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f *.out

run-server:
	./rdma_pingpong_poll.out -s -d rocep0s6

run-client:
	./rdma_pingpong_poll.out -d rocep0s6 10.0.3.2
