LIB     = $(shell pkg-config --libs libtirpc)
HEADER  = $(shell pkg-config --cflags libtirpc)

all:
	rpcgen test.x
	gcc server.c test_svc.c -o test_svc.out $(LIB) $(HEADER)
	gcc client.c test_clnt.c -o test_clnt.out $(LIB) $(HEADER)

