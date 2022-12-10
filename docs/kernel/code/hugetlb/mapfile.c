/*

// @todo 不知道为什么这个程序就不能正常运行
#include <assert.h>  // assert
#include <fcntl.h>   // open
#include <limits.h>  // INT_MAX
#include <math.h>    // sqrt
#include <stdbool.h> // bool false true
#include <stdio.h>
#include <stdlib.h> // malloc sort
#include <string.h> // strcmp ..
#include <sys/mman.h>
#include <unistd.h> // sleep

#define HUGE_SZ (2 << 20)

static inline void bad(const char *msg) {
  printf("%s", msg);
  exit(1);
}

int main(int argc, char *argv[]) {
  int c;

  char template[] = "/tmp/hugeXXXXXX";
  int fd = mkstemp(template);
  if (fd < 0) {
    bad("mkstemp failed");
  }
  printf("[huxueshi:%s:%d] %d\n", __FUNCTION__, __LINE__, fd);
  if (unlink(template)) {
    bad("unlink failed");
  }

  int count = 8;
  char *ptr = (char *)mmap(NULL, count * HUGE_SZ, PROT_READ | PROT_WRITE,
                           MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    bad("mmap failed");
  }
  sleep(1000);

  for (int i = 0; i < count; ++i) {
    printf("[huxueshi:%s:%d] \n", __FUNCTION__, __LINE__);
    printf("%c", *(ptr + i * HUGE_SZ));
  }

  return 0;
}
*/

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define FILE_NAME "/mnt/huge/x"
/* #define FILE_NAME "/tmp/x" */
#define LENGTH (256UL * 1024 * 1024)
#define PROTECTION (PROT_READ | PROT_WRITE)

/* Only ia64 requires this */
#ifdef __ia64__
#define ADDR (void *)(0x8000000000000000UL)
#define FLAGS (MAP_SHARED | MAP_FIXED)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS (MAP_SHARED | MAP_NORESERVE | MAP_HUGETLB)
#endif

static void check_bytes(char *addr) {
  printf("First hex is %x\n", *((unsigned int *)addr));
}

static void write_bytes(char *addr) {
  unsigned long i;

  for (i = 0; i < LENGTH; i++)
    *(addr + i) = (char)i;
}

static int read_bytes(char *addr) {
  unsigned long i;

  check_bytes(addr);
  for (i = 0; i < LENGTH; i++)
    if (*(addr + i) != (char)i) {
      printf("Mismatch at %lu\n", i);
      return 1;
    }
  return 0;
}

int main(void) {
  void *addr;
  int fd, ret;

  fd = open(FILE_NAME, O_CREAT | O_RDWR, 0755);
  if (fd < 0) {
    perror("Open failed");
    exit(1);
  }

  int is_shared = 1;
  scanf("%d", &is_shared);

  addr = mmap(ADDR, LENGTH, PROTECTION, FLAGS, fd, 0);
  if (addr == MAP_FAILED) {
    perror("mmap");
    unlink(FILE_NAME);
    exit(1);
  }
  printf("Returned address is %p\n", addr);

  char str[100];
  printf("before page fault");
  scanf("%s", str);
  check_bytes(addr);
  write_bytes(addr);
  ret = read_bytes(addr);

  /* fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, 0, LENGTH); */

  write_bytes(addr);
  ret = read_bytes(addr);

  munmap(addr, LENGTH);
  close(fd);
  unlink(FILE_NAME);

  return ret;
}
