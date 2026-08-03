#ifndef VIRTIO_DUMMY_H
#define VIRTIO_DUMMY_H

#include "hw/virtio/virtio.h"

#define TYPE_VIRTIO_DUMMY "virtio-dummy"
#define VIRTIO_ID_DUMMY 42
#define VIRTIO_DUMMY_BLOCK_SIZE 512
#define VIRTIO_DUMMY_DEFAULT_SIZE (64ULL * 1024 * 1024)
#define VIRTIO_DUMMY_MAX_SEGS 64

enum virtio_dummy_req_type {
  VIRTIO_DUMMY_T_IN = 0,
  VIRTIO_DUMMY_T_OUT = 1,
  VIRTIO_DUMMY_T_FLUSH = 4,
  VIRTIO_DUMMY_T_GET_ID = 8,
  VIRTIO_DUMMY_T_WRITE_ZEROES = 13,
};

enum virtio_dummy_status {
  VIRTIO_DUMMY_S_OK = 0,
  VIRTIO_DUMMY_S_IOERR = 1,
  VIRTIO_DUMMY_S_UNSUPP = 2,
};

struct virtio_dummy_config {
  uint64_t capacity_sectors;
  uint32_t blk_size;
  uint32_t max_segments;
} QEMU_PACKED;

struct virtio_dummy_req_hdr {
  uint32_t type;
  uint32_t reserved;
  uint64_t sector;
  uint32_t data_len;
  uint32_t reserved2;
} QEMU_PACKED;

struct VirtIODummy {
  VirtIODevice parent_obj;
  VirtQueue *vq;
  struct virtio_dummy_config config;
  uint8_t *storage;
  uint64_t storage_size;
  uint64_t size;
};
typedef struct VirtIODummy VirtIODummy;

#endif /* end of include guard: VIRTIO_DUMMY_H */
