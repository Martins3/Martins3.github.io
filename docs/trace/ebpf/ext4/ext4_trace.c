// ext4_trace.c
#define _GNU_SOURCE
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "ext4.h"
#include "ext4_trace.skel.h" // 自动生成

// ========== 配置 ==========
#define DEBUG 0 // 1: 输出到 stdout; 0: 静默
#define SHM_FILE "/dev/shm/ext4_trace_buffer"
#define BUFFER_SIZE (10 * 1024 * 1024) // 10MB
// ==========================

static int shm_fd = -1;
static off_t current_offset = 0;

static void safe_write_line(const char *line) {
  size_t len = strlen(line);
  if (current_offset + len + 1 > BUFFER_SIZE) {
    fprintf(stderr, "❌ Buffer full! Exiting to avoid data corruption.\n");
    exit(1);
  }

  if (pwrite(shm_fd, line, len, current_offset) != (ssize_t)len ||
      pwrite(shm_fd, "\n", 1, current_offset + len) != 1) {
    perror("pwrite");
    exit(1);
  }

  current_offset += len + 1;

#if DEBUG
  printf("%s\n", line);
#endif
}

static void sig_handler(int sig) {
#if DEBUG
  printf("\n[DEBUG] Received signal %d. Data is in %s (wrote %ld bytes).\n",
         sig, SHM_FILE, current_offset);
#else
  printf("Data written to %s (%ld bytes). Exiting.\n", SHM_FILE,
         current_offset);
#endif
  close(shm_fd);
  exit(0);
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format,
                           va_list args) {
#if DEBUG
  return vfprintf(stderr, format, args);
#else
  return 0;
#endif
}

int main(int argc, char **argv) {
  struct ext4_trace_bpf *skel;
  int err;

  // 设置 libbpf 日志回调
  libbpf_set_print(libbpf_print_fn);

  shm_fd = open(SHM_FILE, O_CREAT | O_RDWR, 0600);
  if (shm_fd < 0) {
    perror("open shm");
    exit(1);
  }

  if (ftruncate(shm_fd, BUFFER_SIZE) < 0) {
    perror("ftruncate");
    close(shm_fd);
    exit(1);
  }

  // 清空内存，而且把内存全部都占用上
  void *ptr = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr == MAP_FAILED) {
    perror("mmap");
    close(shm_fd);
    exit(1);
  }
  memset(ptr, 0, BUFFER_SIZE);
  munmap(ptr, BUFFER_SIZE);

#if DEBUG
  printf("[DEBUG] Pre-allocated %d bytes at %s\n", BUFFER_SIZE, SHM_FILE);
#endif

  // 打开、加载、附加 BPF 程序
  skel = ext4_trace_bpf__open();
  if (!skel) {
    fprintf(stderr, "Failed to open BPF skeleton\n");
    close(shm_fd);
    exit(1);
  }

  err = ext4_trace_bpf__load(skel);
  if (err) {
    fprintf(stderr, "Failed to load BPF skeleton: %d\n", err);
    goto cleanup;
  }

  err = ext4_trace_bpf__attach(skel);
  if (err) {
    fprintf(stderr, "Failed to attach BPF skeleton: %d\n", err);
    goto cleanup;
  }

#if DEBUG
  printf("Tracing ext4_filemap_fault() by filename... Hit Ctrl-C to end.\n");
#endif

  // 写入表头
  safe_write_line("Timestamp,Filename,Count");

  // 注册信号处理
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  // 主循环：每秒读取哈希表并写入
  while (1) {
    sleep(1);

    time_t t = time(NULL);
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_info);

    // 遍历哈希表
    struct fname_key key = {};
    struct fname_key next_key;
    __u64 total = 0;

    // 收集所有条目
    struct entry {
      char name[32];
      __u64 count;
    } entries[1024];
    int entry_count = 0;

    // skel->maps.file_count.map_fd
    // bpf_map__fd(skel->maps.file_count)
    int map_fd = bpf_map__fd(skel->maps.file_count);

    while (bpf_map_get_next_key(map_fd, &key, &next_key) == 0) {
      __u64 count = 0;

      if (bpf_map_lookup_elem(map_fd, &next_key, &count) == 0) {
        if (entry_count < 1024) {
          strncpy(entries[entry_count].name, next_key.name, 31);
          entries[entry_count].name[31] = '\0';
          entries[entry_count].count = count;
          total += count;
          entry_count++;
        }
        // 删除该键（清空计数）
        bpf_map_delete_elem(map_fd, &next_key);
      }

      key = next_key;
    }

    // 按 count 降序排序（简单冒泡）
    for (int i = 0; i < entry_count - 1; i++) {
      for (int j = 0; j < entry_count - i - 1; j++) {
        if (entries[j].count < entries[j + 1].count) {
          struct entry tmp = entries[j];
          entries[j] = entries[j + 1];
          entries[j + 1] = tmp;
        }
      }
    }

    // 写入排序后结果
    for (int i = 0; i < entry_count; i++) {
      char line[256];
      snprintf(line, sizeof(line), "%s,%s,%llu", timestamp, entries[i].name,
               entries[i].count);
      safe_write_line(line);
    }

    // 写入总计
    char total_line[256];
    snprintf(total_line, sizeof(total_line), "%s,<TOTAL>,%llu", timestamp,
             total);
    safe_write_line(total_line);
  }

cleanup:
  ext4_trace_bpf__destroy(skel);
  if (shm_fd >= 0)
    close(shm_fd);
  return -err;
}
