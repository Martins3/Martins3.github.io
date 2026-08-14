// readahead.c
#define _GNU_SOURCE
#include "readahead.skel.h"
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static struct readahead_bpf *skel;

int end = 0;
static void sig_handler(int sig) {
  printf("\nReceived signal %d, exiting...\n", sig);
  end = 1;
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format,
                           va_list args) {
  return vfprintf(stderr, format, args);
}

// ✅ 打印精确值直方图（按 nr_to_read 排序）
static void print_exact_histogram(int map_fd) {
  __u64 key = 0, next_key;
  __u64 total = 0;

  // 收集所有条目
  struct entry {
    __u64 nr_to_read;
    __u64 count;
  } entries[1024];
  int entry_count = 0;

  while (bpf_map_get_next_key(map_fd, &key, &next_key) == 0) {
    __u64 count;
    if (bpf_map_lookup_elem(map_fd, &next_key, &count) == 0) {
      printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
      if (entry_count < 1024) {
        entries[entry_count].nr_to_read = next_key;
        entries[entry_count].count = count;
        total += count;
        entry_count++;
      }
    }
    bpf_map_delete_elem(map_fd, &next_key);
    key = next_key;
  }

  // 按 nr_to_read 升序排序（冒泡）
  for (int i = 0; i < entry_count - 1; i++) {
    for (int j = 0; j < entry_count - i - 1; j++) {
      if (entries[j].nr_to_read > entries[j + 1].nr_to_read) {
        struct entry tmp = entries[j];
        entries[j] = entries[j + 1];
        entries[j + 1] = tmp;
      }
    }
  }

  // 打印表头
  printf("\n%-12s %s\n", "nr_to_read", "count");
  printf("%-12s %s\n", "-----------", "-----");

  // 打印排序后结果
  for (int i = 0; i < entry_count; i++) {
    printf("%-12llu %llu\n", entries[i].nr_to_read, entries[i].count);
  }

  printf("Total events: %llu\n", total);
}

int main(int argc, char **argv) {
  int err;

  libbpf_set_print(libbpf_print_fn);

  skel = readahead_bpf__open();
  if (!skel) {
    fprintf(stderr, "Failed to open BPF skeleton\n");
    return 1;
  }

  err = readahead_bpf__load(skel);
  if (err) {
    fprintf(stderr, "Failed to load BPF skeleton: %d\n", err);
    goto cleanup;
  }

  err = readahead_bpf__attach(skel);
  if (err) {
    fprintf(stderr, "Failed to attach BPF skeleton: %d\n", err);
    goto cleanup;
  }

  printf(
      "Tracing page_cache_ra_unbounded() nr_to_read... Hit Ctrl-C to end.\n");

  signal(SIGINT, sig_handler);

  while (!end) {
    sleep(1);
    printf(".");
    fflush(stdout);
  }

  printf("\n=== Exact nr_to_read Distribution ===\n");
  int map_fd = bpf_map__fd(skel->maps.nr_to_read_count);
  print_exact_histogram(map_fd);
  printf("=====================================\n");

cleanup:
  readahead_bpf__destroy(skel);
  return -err;
}
