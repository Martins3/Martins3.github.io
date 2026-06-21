.PHONY: clean
.PHONY: server client

CFLAGS  := -Wall -g
LD      := gcc
LDLIBS  := ${LDLIBS} -lrdmacm -libverbs -lpthread

APPS    := server client

all: ${APPS}

client: client.o
	${LD} -o $@.out $^ ${LDLIBS}

server: server.o
	${LD} -o $@.out $^ ${LDLIBS}

clean:
	rm -f ${APPS}
