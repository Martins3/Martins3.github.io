#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/syscall.h>
#include <unistd.h>

static volatile sig_atomic_t done;

static void sig_handler(int sig __attribute__((unused)))
{
	done = 1;
}

struct __attribute__((__packed__)) sched_switch_entry {
	unsigned short common_type;
	unsigned char common_flags;
	unsigned char common_preempt_count;
	int common_pid;

	char prev_comm[16];
	int32_t prev_pid;
	int32_t prev_prio;
	int64_t prev_state;
	char next_comm[16];
	int32_t next_pid;
	int32_t next_prio;
};

struct __attribute__((__packed__)) scsi_timeout_entry {
	unsigned short common_type; //       offset:0;       size:2; signed:0;
	unsigned char common_flags; //       offset:2;       size:1; signed:0;
	unsigned char
		common_preempt_count; //       offset:3;       size:1; signed:0;
	int common_pid; //   offset:4;       size:4; signed:1;

	unsigned int host_no; //     offset:8;       size:4; signed:0;
	unsigned int channel; //     offset:12;      size:4; signed:0;
	unsigned int id; //  offset:16;      size:4; signed:0;
	unsigned int lun; // offset:20;      size:4; signed:0;
	int result; //       offset:24;      size:4; signed:1;
	unsigned int opcode; //      offset:28;      size:4; signed:0;
	unsigned int cmd_len; //     offset:32;      size:4; signed:0;
	int driver_tag; //   offset:36;      size:4; signed:1;
	int scheduler_tag; //        offset:40;      size:4; signed:1;
	unsigned int data_sglen; //  offset:44;      size:4; signed:0;
	unsigned int prot_sglen; //  offset:48;      size:4; signed:0;
	unsigned char prot_op; //    offset:52;      size:1; signed:0;
	// __data_loc unsigned char[] cmnd; //  offset:56;      size:4; signed:0;
	uint32_t cmnd_loc;
	uint8_t sense_key; //     offset:60;      size:1; signed:0;
	uint8_t asc; //   offset:61;      size:1; signed:0;
	uint8_t ascq; //  offset:62;      size:1; signed:0;
};

static inline void parse_data_loc(uint32_t loc, uint16_t *off, uint16_t *len)
{
	*len = loc & 0xffff;
	*off = loc >> 16;
}

static int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
			   int group_fd, unsigned long flags)
{
	return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

int main(void)
{
	int fd;
	FILE *f;
	int sched_switch_id;
	size_t page_size = sysconf(_SC_PAGESIZE);

	/* metadata page + 2^n data pages */
	int data_pages = 8;
	size_t mmap_len = page_size * (1 + data_pages);

	char *mmap_buf;
	struct perf_event_mmap_page *meta;

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

// #define SYSFS_PATH "/sys/kernel/debug/tracing/events/sched/sched_switch/id"
#define SYSFS_PATH \
	"/sys/kernel/debug/tracing/events/scsi/scsi_dispatch_cmd_timeout/id"
	f = fopen(SYSFS_PATH, "r");
	if (!f) {
		perror("open sched_switch id");
		fprintf(stderr, "did you mount debugfs?\n");
		return 1;
	}

	if (fscanf(f, "%d", &sched_switch_id) != 1) {
		fprintf(stderr, "failed to read sched_switch id\n");
		fclose(f);
		return 1;
	}
	fclose(f);

	printf("scsi:scsi_dispatch_cmd_timeout id = %d\n", sched_switch_id);

	struct perf_event_attr attr;
	memset(&attr, 0, sizeof(attr));

	attr.type = PERF_TYPE_TRACEPOINT;
	attr.config = sched_switch_id;
	attr.size = sizeof(attr);

	attr.sample_type = PERF_SAMPLE_TIME | PERF_SAMPLE_RAW;
	attr.sample_period = 1;
	attr.disabled = 1;
	attr.wakeup_events = 1;

	/* ---------- perf_event_open ---------- */
	/* pid = -1 (all tasks), cpu = 0 (one cpu) */
	/*
	 * 
       int syscall(SYS_perf_event_open, struct perf_event_attr *attr,
                   pid_t pid, int cpu, int group_fd, unsigned long flags);
	 */

	fd = perf_event_open(&attr, -1, -1, -1, 0);
	if (fd < 0) {
		perror("perf_event_open");
		return 1;
	}

	/* ---------- mmap ring buffer ---------- */

	mmap_buf =
		mmap(NULL, mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mmap_buf == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return 1;
	}

	meta = (struct perf_event_mmap_page *)mmap_buf;

	printf("Tracing sched_switch... Ctrl+C to stop\n\n");

	ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

	/* ---------- event loop ---------- */

	while (!done) {
		uint64_t head, tail;
		char *data;

		/* memory barrier: kernel writes head first */
		head = meta->data_head;
		__sync_synchronize();

		tail = meta->data_tail;
		data = mmap_buf + page_size;

		while (tail < head) {
			struct perf_event_header *ehdr;
			size_t offset = tail & ((data_pages * page_size) - 1);

			ehdr = (struct perf_event_header *)(data + offset);

			if (ehdr->type == PERF_RECORD_SAMPLE) {
				char *ptr = (char *)ehdr + sizeof(*ehdr);

				uint64_t time;
				uint32_t raw_size;
				struct sched_switch_entry *ent;

				time = *(uint64_t *)ptr;
				ptr += sizeof(uint64_t);

				raw_size = *(uint32_t *)ptr;
				ptr += sizeof(uint32_t);

				void *raw = ptr;
				if (raw_size >=
				    sizeof(struct scsi_timeout_entry)) {
					struct scsi_timeout_entry *e = raw;
					printf("[martins3:%s:%d] \n", __func__, __LINE__);

					uint16_t cmnd_off, cmnd_len;
					parse_data_loc(e->cmnd_loc, &cmnd_off,
						       &cmnd_len);

					unsigned char *cmnd =
						(unsigned char *)raw + cmnd_off;

					printf("PID=%d host=%u:%u:%u:%u opcode=0x%x cmd_len=%u ",
					       e->common_pid, e->host_no,
					       e->channel, e->id, e->lun,
					       e->opcode, e->cmd_len);

					printf("cmnd=");
					for (int i = 0; i < cmnd_len; i++)
						printf("%02x ", cmnd[i]);

					printf(" sense=%02x/%02x/%02x\n",
					       e->sense_key, e->asc, e->ascq);
				}
			}

			tail += ehdr->size;
		}

		meta->data_tail = tail;
		usleep(100000);
	}

	ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
	munmap(mmap_buf, mmap_len);
	close(fd);

	puts("\nTrace stopped.");
	return 0;
}
