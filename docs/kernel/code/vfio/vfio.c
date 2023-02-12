#include <assert.h> // assert
#include <errno.h>  // strerror
#include <fcntl.h>  // open
#include <limits.h> // INT_MAX
#include <linux/vfio.h>
#include <math.h>    // sqrt
#include <stdbool.h> // bool false true
#include <stdio.h>
#include <stdlib.h> // malloc sort
#include <string.h> // strcmp
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h> // sleep
                    //
void init() {
  int res;
  int container, group, device, i;
  struct vfio_iommu_type1_info iommu_info = {.argsz = sizeof(iommu_info)};
  struct vfio_iommu_type1_dma_map dma_map = {.argsz = sizeof(dma_map)};

  /* Create a new container */
  container = open("/dev/vfio/vfio", O_RDWR);

  if (ioctl(container, VFIO_GET_API_VERSION) != VFIO_API_VERSION) {

    /* Unknown API version */

    if (!ioctl(container, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)) {
      /* Doesn't support the IOMMU driver we want. */

      /* Enable the IOMMU model we want */
      res = ioctl(container, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU);
      printf("[huxueshi:%s:%d] %d\n", __FUNCTION__, __LINE__, res);
    }

  } else {
    printf("VFIO API VERSION\n");
  }

  /* Get addition IOMMU info */
  res = ioctl(container, VFIO_IOMMU_GET_INFO, &iommu_info);
  printf("[huxueshi:%s:%d] %d\n", __FUNCTION__, __LINE__, res);

  /* Allocate some space and setup a DMA mapping */
  dma_map.vaddr = (long long)mmap(0, 1024 * 1024, PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  dma_map.size = 1024 * 1024;
  dma_map.iova = 0; /* 1MB starting at 0x0 from device view */
  dma_map.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;

  res = ioctl(container, VFIO_IOMMU_MAP_DMA, &dma_map);
  printf("[huxueshi:%s:%d] %d\n", __FUNCTION__, __LINE__, res);

  // ---------------- check ---------------------------

  struct vfio_group_status group_status = {.argsz = sizeof(group_status)};
  /* Open the group */
  group = open("/dev/vfio/26", O_RDWR);

  /* Test the group is viable and available */
  ioctl(group, VFIO_GROUP_GET_STATUS, &group_status);

  if (!(group_status.flags & VFIO_GROUP_FLAGS_VIABLE))
    /* Group is not viable (ie, not all devices bound for vfio) */

    /* Add the group to the container */
    ioctl(group, VFIO_GROUP_SET_CONTAINER, &container);

  /* Get a file descriptor for the device */
  device = ioctl(group, VFIO_GROUP_GET_DEVICE_FD, "0000:06:0d.0");
}

int main(int argc, char *argv[]) { return 0; }
