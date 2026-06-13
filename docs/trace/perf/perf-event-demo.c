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

static void sig_handler(int sig __attribute__((unused))) { done = 1; }

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

static int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                           int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

int main(void) {
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

  f = fopen("/sys/kernel/debug/tracing/events/sched/sched_switch/id", "r");
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

  printf("sched:sched_switch id = %d\n", sched_switch_id);

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

  fd = perf_event_open(&attr, -1, 0, -1, 0);
  if (fd < 0) {
    perror("perf_event_open");
    return 1;
  }

  /* ---------- mmap ring buffer ---------- */

  mmap_buf = mmap(NULL, mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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

        if (raw_size >= sizeof(*ent)) {
          ent = (struct sched_switch_entry *)ptr;

          printf("[%lu] %u %-16s (%5d) ==> %-16s (%5d)\n", time,
                 ent->common_pid, ent->prev_comm, ent->prev_pid, ent->next_comm,
                 ent->next_pid);
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
