# block


## block device


## block layer
[nvme 的网站](https://nvmexpress.org/education/drivers/linux-driver-information/)的图:
![](https://nvmexpress.org/wp-content/uploads/Linux-storage-stack-diagram_v4.10-e1575939041721.png)

1. 理解一下 IO scheduler 和 blkmq 的关系:
https://kernel.dk/blk-mq.pdf 


## ldd3 的驱动理解

- [ ] register_blkdev , add_disk, get_gendisk
  - [x] register_blkdev 注册了 major number 在 `major_names` 中间，但是 `major_names` 除了使用在 blkdev_show (cat /proc/devices) 之外没有看到其他的用处
    - https://linux-kernel-labs.github.io/refs/heads/master/labs/block_device_drivers.html : 中说，register_blkdev 是会取消掉的
    - 从 virtio_blk.c 中间来看 : major 放到局部变量中间，所以实际功能是分配 major number

- [ ] 似乎很多的初始化工作是放到了 init 中间了
