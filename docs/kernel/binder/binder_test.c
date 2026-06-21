#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

// Define binder constants if the header isn't available
#ifndef _LINUX_BINDER_H
// Define the types we need
typedef uint64_t __u64;
typedef int32_t __s32;

#define BINDER_WRITE_READ _IOWR('b', 1, struct binder_write_read)
#define BINDER_SET_IDLE_TIMEOUT _IOW('b', 3, int64_t)
#define BINDER_SET_MAX_THREADS _IOW('b', 6, size_t)
#define BINDER_SET_IDLE_PRIORITY _IOW('b', 7, int)
#define BINDER_SET_CONTEXT_MGR _IOW('b', 8, int)
#define BINDER_THREAD_EXIT _IOW('b', 9, int)
#define BINDER_VERSION _IOWR('b', 10, struct binder_version)

struct binder_write_read {
  __u64 write_size;
  __u64 write_consumed;
  __u64 write_buffer;
  __u64 read_size;
  __u64 read_consumed;
  __u64 read_buffer;
};

struct binder_version {
  int32_t protocol_version;
};
#endif

#define BINDER_DEVICE "/dev/binderfs/binder"
#define BINDERFS_DEVICE "/dev/binder"

// Simple test for binder functionality
int main() {
  printf("Starting Android Binder Test...\n");

  // Try to open the binder device
  int fd = open(BINDER_DEVICE, O_RDWR);
  if (fd == -1) {
    fd = open(BINDERFS_DEVICE, O_RDWR);
  }

  printf("Binder device opened successfully\n");

  // Get binder version
  struct binder_version version;
  memset(&version, 0, sizeof(version));

  if (ioctl(fd, BINDER_VERSION, &version) == -1) {
    printf("Could not get binder version: %s\n", strerror(errno));
    close(fd);
    return 1;
  }

  printf("Binder version: %d\n", version.protocol_version);

  close(fd);

  printf("\nBinder test completed successfully!\n");
  return 0;
}
