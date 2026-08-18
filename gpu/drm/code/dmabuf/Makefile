CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g $(shell pkg-config --cflags libdrm 2>/dev/null || echo "")
LIBS = $(shell pkg-config --libs libdrm 2>/dev/null || echo "-ldrm")

TEST_PROGS = virtio_gpu_dma_test.out udmabuf_demo.out udmabuf_drm_import_demo.out
TEST_OBJS = virtio_gpu_dma_test.o udmabuf_demo.o udmabuf_drm_import_demo.o

all: $(TEST_PROGS)

virtio_gpu_dma_test.out: virtio_gpu_dma_test.o
	$(CC) -o $@ $< $(LIBS)

udmabuf_demo.out: udmabuf_demo.o
	$(CC) -o $@ $<

udmabuf_drm_import_demo.out: udmabuf_drm_import_demo.o
	$(CC) -o $@ $< $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TEST_OBJS) $(TEST_PROGS)

.PHONY: all clean install-deps test
