.PHONY: clean

CFLAGS  := -Wall -Werror -g
LD      := gcc
LDFLAGS := ${LDFLAGS} -lrdmacm -lpthread -libverbs

APPS    := client server

all: ${APPS}

client: rdma-common.o rdma-client.o
	${LD} -o $@.out $^ ${LDFLAGS}

server: rdma-common.o rdma-server.o
	${LD} -o $@.out $^ ${LDFLAGS}

clean:
	rm -f *.o ${APPS}

