#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define SHM_NAME "/my_shared_memory"
#define PAGE_SIZE 4096

int main() {
  int fd;
  char *mapped;
  int counter = 0;
  char buffer[PAGE_SIZE];

  // 打印当前进程 PID
  printf("Process PID: %d\n", getpid()); // <-- 新增

  // 创建或打开共享内存对象
  fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    perror("shm_open");
    exit(EXIT_FAILURE);
  }

  // 设置共享内存大小
  if (ftruncate(fd, PAGE_SIZE) == -1) {
    perror("ftruncate");
    exit(EXIT_FAILURE);
  }

  // 映射共享内存
  mapped = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mapped == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  printf("Shared memory mapped at address %p\n", mapped);

  // 每隔1秒写入数据
  while (1) {
    time_t now = time(NULL);
    snprintf(buffer, PAGE_SIZE, "Counter: %d, Time: %s", counter++,
             ctime(&now));
    memcpy(mapped, buffer, strlen(buffer) + 1);
    sleep(1);
  }

  // 清理（通常不会执行到这里）
  munmap(mapped, PAGE_SIZE);
  shm_unlink(SHM_NAME);
  close(fd);

  return 0;
}
