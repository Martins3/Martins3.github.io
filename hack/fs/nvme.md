# nvme
https://www.youtube.com/watch?v=NtkKHhXf3V4

- [ ] host 和 target 两个文件夹指的是 ?

- `nvme_create_io_queues` => `nvme_alloc_queue` => `dma_alloc_coherent`

- NVMe-over-Fabrics Performance Characterization and the Path to Low-Overhead Flash Disaggregation
  - https://dl.acm.org/doi/pdf/10.1145/3078468.3078483


## 一个奇怪的事情

在 qemu 中， block/nvme.c 是
- nvme_init
  - qemu_vfio_open_pci
    - qemu_vfio_init_pci
