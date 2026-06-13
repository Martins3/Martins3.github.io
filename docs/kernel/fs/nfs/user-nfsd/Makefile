ifeq ($(origin CC),default)
CC := $(if $(wildcard /usr/bin/gcc),/usr/bin/gcc,cc)
endif
PKG_CONFIG ?= pkg-config
RPCGEN ?= rpcgen

CFLAGS ?= -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-cast-function-type
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags libtirpc 2>/dev/null)
LDLIBS += $(shell $(PKG_CONFIG) --libs libtirpc 2>/dev/null)

SERVER_SRCS := user_nfsd.c nfs3_svc.c nfs3_xdr.c mount3_svc.c mount3_xdr.c
TEST_SRCS := test_path.c user_nfsd_core.c

.PHONY: all clean test regenerate

all: user-nfsd.out

user-nfsd.out: $(SERVER_SRCS:.c=.o) user_nfsd_core.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_path.out: $(TEST_SRCS:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

test: test_path.out
	./test_path.out

regenerate: nfs3.x mount3.x
	$(RPCGEN) -MN -h -o nfs3.h nfs3.x
	$(RPCGEN) -MN -c -o nfs3_xdr.c nfs3.x
	$(RPCGEN) -MN -m -o nfs3_svc.c nfs3.x
	$(RPCGEN) -MN -h -o mount3.h mount3.x
	$(RPCGEN) -MN -c -o mount3_xdr.c mount3.x
	$(RPCGEN) -MN -m -o mount3_svc.c mount3.x

clean:
	rm -f *.o *.out
