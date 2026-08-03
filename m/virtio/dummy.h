#ifndef DUMMY_F67FXGM2_H
#define DUMMY_F67FXGM2_H

#include <linux/blk-mq.h>
#include <linux/types.h>

#define VIRTIO_ID_DUMMY 42
#define DUMMY_SECTOR_SHIFT 9
#define DUMMY_BLOCK_SIZE (1U << DUMMY_SECTOR_SHIFT)
#define DUMMY_QUEUE_DEPTH 128
#define DUMMY_MAX_SEGS 64

struct virtio_dummy_config {
	__le64 capacity_sectors;
	__le32 blk_size;
	__le32 max_segments;
} __packed;

enum virtio_dummy_req_type {
	VIRTIO_DUMMY_T_IN = 0,
	VIRTIO_DUMMY_T_OUT = 1,
	VIRTIO_DUMMY_T_FLUSH = 4,
	VIRTIO_DUMMY_T_GET_ID = 8,
	VIRTIO_DUMMY_T_WRITE_ZEROES = 13,
};

struct virtio_dummy_req_hdr {
	__le32 type;
	__le32 reserved;
	__le64 sector;
	__le32 data_len;
	__le32 reserved2;
} __packed;

enum virtio_dummy_status {
	VIRTIO_DUMMY_S_OK = 0,
	VIRTIO_DUMMY_S_IOERR = 1,
	VIRTIO_DUMMY_S_UNSUPP = 2,
};

enum Testcase {
	NOTHING,
	INTERRUPT_STACK,
	BLK_TRACE,
	IRQ_CURRENT,
	SLEEP_IN_SOFTIQR,
	SLEEP_IN_HARDIQR,
	MDELAY_IN_HARDIQR
};

extern int testcase;
void debug_dump_request(struct request *req);
void debug_softirq(void);
void debug_hardirq(void);
void debug_dump_gendisk(struct gendisk * disk);


#endif /* end of include guard: DUMMY_F67FXGM2_H */
