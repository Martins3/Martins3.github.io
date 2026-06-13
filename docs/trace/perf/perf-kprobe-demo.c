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
#include <sys/syscall.h>
#include <unistd.h>

/*
 * perf_kprobe 演示程序
 *
 * 通过 perf_event_open 系统调用直接创建 kprobe，无需使用 perf 命令行工具。
 * 这展示了 kprobe 如何被封装为 perf_event 的 PMU 类型。
 */

static volatile sig_atomic_t done;

static void sig_handler(int sig __attribute__((unused))) { done = 1; }

/* 封装 perf_event_open 系统调用 */
static int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                           int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

int main(int argc, char *argv[]) {
  int fd;
  size_t page_size = sysconf(_SC_PAGESIZE);

  /* metadata page + 数据页 */
  int data_pages = 8;
  size_t mmap_len = page_size * (1 + data_pages);

  char *mmap_buf;
  struct perf_event_mmap_page *meta;

  /* 默认探测 do_nanosleep，可通过命令行参数指定 */
  const char *kprobe_func = (argc > 1) ? argv[1] : "do_nanosleep";
  int is_kretprobe = 0;

  /* 检查是否是 kretprobe (函数名以 "ret:" 开头) */
  if (strncmp(kprobe_func, "ret:", 4) == 0) {
    kprobe_func += 4;
    is_kretprobe = 1;
  }

  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  /* 读取 kprobe PMU 的 type ID */
  int kprobe_type = -1;
  FILE *f = fopen("/sys/bus/event_source/devices/kprobe/type", "r");
  if (!f) {
    perror("无法打开 kprobe type 文件");
    fprintf(stderr, "提示: 确保内核支持 kprobe 事件\n");
    fprintf(stderr, "      路径: /sys/bus/event_source/devices/kprobe/type\n");
    return 1;
  }
  if (fscanf(f, "%d", &kprobe_type) != 1) {
    fprintf(stderr, "无法读取 kprobe type\n");
    fclose(f);
    return 1;
  }
  fclose(f);

  printf("kprobe PMU type = %d\n", kprobe_type);
  printf("探测函数: %s%s\n", is_kretprobe ? "ret:" : "", kprobe_func);
  printf("(使用 Ctrl+C 停止)\n\n");

  /* 配置 perf_event_attr */
  struct perf_event_attr attr;
  memset(&attr, 0, sizeof(attr));

  attr.type = kprobe_type;
  attr.size = sizeof(attr);

  /* config 的 bit 0 用于标记是否是 kretprobe (PERF_PROBE_CONFIG_IS_RETPROBE = 1) */
  attr.config = is_kretprobe ? 1 : 0;

  /*
   * 设置要探测的函数名和偏移
   * kprobe_func 和 config1 是 union 成员
   * probe_offset 和 config2 是 union 成员
   */
  attr.kprobe_func = (uint64_t)(unsigned long)kprobe_func;
  attr.probe_offset = 0;

  /* 设置采样类型 */
  attr.sample_type = PERF_SAMPLE_TIME | PERF_SAMPLE_IP;
  attr.sample_period = 1;
  attr.disabled = 1;

  /* 调用 perf_event_open 创建 kprobe */
  fd = perf_event_open(&attr, -1, 0, -1, 0);
  if (fd < 0) {
    perror("perf_event_open failed");
    fprintf(stderr, "提示: 可能需要 root 权限 (CAP_PERFMON 或 CAP_SYS_ADMIN)\n");
    fprintf(stderr, "      或者检查函数名 '%s' 是否存在: grep %s /proc/kallsyms\n",
            kprobe_func, kprobe_func);
    return 1;
  }

  /* mmap ring buffer */
  mmap_buf = mmap(NULL, mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mmap_buf == MAP_FAILED) {
    perror("mmap failed");
    close(fd);
    return 1;
  }

  meta = (struct perf_event_mmap_page *)mmap_buf;

  /* 启用 kprobe */
  if (ioctl(fd, PERF_EVENT_IOC_ENABLE, 0) < 0) {
    perror("ioctl(PERF_EVENT_IOC_ENABLE) failed");
    munmap(mmap_buf, mmap_len);
    close(fd);
    return 1;
  }

  printf("kprobe 已启用，等待事件...\n\n");

  /* 事件循环 */
  uint64_t event_count = 0;

  while (!done) {
    uint64_t head, tail;
    char *data;

    /* 内存屏障 */
    head = meta->data_head;
    __sync_synchronize();
    tail = meta->data_tail;
    data = mmap_buf + page_size;

    /* 处理所有新事件 */
    while (tail < head) {
      struct perf_event_header *ehdr;
      size_t offset = tail % (data_pages * page_size);

      ehdr = (struct perf_event_header *)(data + offset);

      if (ehdr->type == PERF_RECORD_SAMPLE) {
        char *ptr = (char *)ehdr + sizeof(*ehdr);

        uint64_t time = 0, ip = 0;

        /* 解析时间戳 */
        time = *(uint64_t *)ptr;
        ptr += sizeof(uint64_t);

        /* 解析指令指针 */
        ip = *(uint64_t *)ptr;
        ptr += sizeof(uint64_t);

        printf("[%6lu] time=%lu ip=0x%lx %s\n",
               ++event_count, time, ip, is_kretprobe ? "<-- return" : "--> entry");
      }

      tail += ehdr->size;
    }

    meta->data_tail = tail;
    usleep(10000);  /* 10ms */
  }

  /* 清理 */
  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
  munmap(mmap_buf, mmap_len);
  close(fd);

  printf("\n总计捕获 %lu 个事件\n", event_count);
  printf("kprobe 已停止.\n");
  return 0;
}
