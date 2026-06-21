LIBFUSE=/home/martins3/core/libfuse/build/install

LIBFUSE_FLAGS=-I$(LIBFUSE)/include/fuse3  -lpthread
# -L/$(LIBFUSE)/lib -lfuse3
# LIBFUSE_FLAGS=`pkg-config fuse3 --cflags --libs`
all:
	gcc -Wall hello.c $(LIBFUSE)/lib/libfuse3.a $(LIBFUSE_FLAGS)  -o hello.out
