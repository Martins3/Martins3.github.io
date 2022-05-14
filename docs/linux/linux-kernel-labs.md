# Linux kernel Labs 笔记

## 回答的问题
- [ ] 浏览一下 fs 中内容
- [ ] network
- [ ] device model
- [ ] kgdb
- [ ] user model linux

## 基本操作
1. [搭建开发环境](https://linux-kernel-labs.github.io/refs/heads/master/info/vm.html)
```sh
git clone https://github.com/linux-kernel-labs/linux linux-kernel-labs
cd linux-kernel-labs/tools/labs
make boot
```

- [ ] make 和 make build 的各自的工作内容。
- [ ] 开发环境的内容也需要吸收一下

## Makefile, shell scripts and python scripts internals
1. img :
    1. 100M myimg : what's it's purpose ?
    2. why, without special configuration, why already have vi ?
        3. I create a.md under /home/root, so where is a.md, in which disk ?
    3. what's realtion with bzimage disk1.img disk2.img and mydisk.img ?
2. change the login password ? set password, change user name
4. ldd3 : kernel debug chapter! read it.

> 6. The analysis of the kernel image is a method of static analysis. If we want to perform dynamic analysis (analyzing how the kernel runs, not only its static image) we can use /proc/kcore;
> maybe, this should be done when the kernel version on host and qemu are consistent !


## Notes
1. Using the dynamic image of the kernel is useful for detecting rootkits.

# 10 Device model

Plug and Play is a technology that offers support for automatically adding and removing devices to your system. This reduces conflicts with the resources they use by automatically configuring them at system startup. In order to achieve these goals, the following features are required:

- Automatic detection of adding and removing devices in the system (the device and its bus must notify the appropriate driver that a configuration change occurred).
- Resource management (addresses, irq lines, DMA channels, memory areas), including resource allocation to devices and solving conflicts that may arise.
- Devices must allow for software configuration (device resources - ports, interrupts, DMA resources - must allow for driver assignment).
- The drivers required for new devices must be loaded automatically by the operating system when needed.
- When the device and its bus allow, the system should be able to add or remove the device from the system while it is running, without having to reboot the system ( hotplug ).

## 笔记
1. `bus_find_device_by_name` : 存在这种函数 !
2. **Other possible operations on a bus are browsing the drivers or devices attached to it**.
Although we can not directly access them (lists of drives and devices being stored in the private data of the driver, the subsys_private * p field ),
these can be scanned using the *bus_for_each_dev* and *bus_for_each_drv* macrodefines .


## 资料

```c
static inline const char *dev_name(const struct device *dev)
{
  /* Use the init name until the kobject becomes available */
  if (dev->init_name)
    return dev->init_name;

  return kobject_name(&dev->kobj);
}

static inline const char *kobject_name(const struct kobject *kobj)
{
  return kobj->name;
}
```

```c
/**
 * dev_set_name - set a device name
 * @dev: device
 * @fmt: format string for the device's name
 */
int dev_set_name(struct device *dev, const char *fmt, ...)
{
  va_list vargs;
  int err;

  va_start(vargs, fmt);
  err = kobject_set_name_vargs(&dev->kobj, fmt, vargs); // 调用kvasprintf_const，然后给 kobj 赋值
  va_end(vargs);
  return err;
}
```

[IOMMU](https://en.wikipedia.org/wiki/Input%E2%80%93output_memory_management_unit)

```c

/**
 * A bus is a channel between the processor and one or more devices. For the
 * purposes of the device model, all devices are connected via a bus, even if
 * it is an internal, virtual, "platform" bus. Buses can plug into each other.
 * A USB controller is usually a PCI device, for example. The device model
 * represents the actual connections between buses and the devices they control.
 * A bus is represented by the bus_type structure. It contains the name, the
 * default attributes, the bus' methods, PM operations, and the driver core's
 * private data.
 */
struct bus_type {
  const char    *name;
  const char    *dev_name;
  struct device   *dev_root;
  const struct attribute_group **bus_groups;
  const struct attribute_group **dev_groups;
  const struct attribute_group **drv_groups;

  int (*match)(struct device *dev, struct device_driver *drv);
  int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
  int (*probe)(struct device *dev);
  int (*remove)(struct device *dev);
  void (*shutdown)(struct device *dev);

  int (*online)(struct device *dev);
  int (*offline)(struct device *dev);

  int (*suspend)(struct device *dev, pm_message_t state);
  int (*resume)(struct device *dev);

  int (*num_vf)(struct device *dev);

  int (*dma_configure)(struct device *dev);

  const struct dev_pm_ops *pm; // 电池管理

  const struct iommu_ops *iommu_ops;

  struct subsys_private *p;
  struct lock_class_key lock_key;

  bool need_parent_lock;
};
```


[](http://www.staroceans.org/kernel-and-driver/The%20Linux%20Kernel%20Driver%20Model.pdf)


```c
struct attribute {
  const char    *name;
  umode_t     mode;
};

/**
 * struct attribute_group - data structure used to declare an attribute group.
 * @name: Optional: Attribute group name
 *    If specified, the attribute group will be created in
 *    a new subdirectory with this name.
 * @is_visible: Optional: Function to return permissions associated with an
 *    attribute of the group. Will be called repeatedly for each
 *    non-binary attribute in the group. Only read/write
 *    permissions as well as SYSFS_PREALLOC are accepted. Must
 *    return 0 if an attribute is not visible. The returned value
 *    will replace static permissions defined in struct attribute.
 * @is_bin_visible:
 *    Optional: Function to return permissions associated with a
 *    binary attribute of the group. Will be called repeatedly
 *    for each binary attribute in the group. Only read/write
 *    permissions as well as SYSFS_PREALLOC are accepted. Must
 *    return 0 if a binary attribute is not visible. The returned
 *    value will replace static permissions defined in
 *    struct bin_attribute.
 * @attrs:  Pointer to NULL terminated list of attributes.
 * @bin_attrs:  Pointer to NULL terminated list of binary attributes.
 *    Either attrs or bin_attrs or both must be provided.
 */
struct attribute_group {
  const char    *name;
  umode_t     (*is_visible)(struct kobject *,
                struct attribute *, int);
  umode_t     (*is_bin_visible)(struct kobject *,
              struct bin_attribute *, int);
  struct attribute  **attrs;
  struct bin_attribute  **bin_attrs;
};
```

```c
/*
 * The type of device, "struct device" is embedded in. A class
 * or bus can contain devices of different types
 * like "partitions" and "disks", "mouse" and "event".
 * This identifies the device type and carries type-specific
 * information, equivalent to the kobj_type of a kobject.
 * If "name" is specified, the uevent will contain it in
 * the DEVTYPE variable.
 */
struct device_type {
  const char *name;
  const struct attribute_group **groups;
  int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
  char *(*devnode)(struct device *dev, umode_t *mode,
       kuid_t *uid, kgid_t *gid);
  void (*release)(struct device *dev);

  const struct dev_pm_ops *pm;
};
```

```c
/**
 * The device driver-model tracks all of the drivers known to the system.
 * The main reason for this tracking is to enable the driver core to match
 * up drivers with new devices. Once drivers are known objects within the
 * system, however, a number of other things become possible. Device drivers
 * can export information and configuration variables that are independent
 * of any specific device.
 */
struct device_driver {
  const char    *name;
  struct bus_type   *bus;

  struct module   *owner;
  const char    *mod_name;  /* used for built-in modules */

  bool suppress_bind_attrs; /* disables bind/unbind via sysfs */
  enum probe_type probe_type;

  const struct of_device_id *of_match_table;
  const struct acpi_device_id *acpi_match_table;

  int (*probe) (struct device *dev);
  int (*remove) (struct device *dev);
  void (*shutdown) (struct device *dev);
  int (*suspend) (struct device *dev, pm_message_t state);
  int (*resume) (struct device *dev);
  const struct attribute_group **groups;

  const struct dev_pm_ops *pm;
  void (*coredump) (struct device *dev);

  struct driver_private *p;
};
```
> @todo 所以，是不是说，device_driver 让 device 和 driver 分离了 ?


## 问题
1. hotplug 和 plug and play 的关系是什么 ?
2. uevent ?
3. device 和 device driver 的区别是什么 ?
4. 居然存在两个 init_module，那么到底如何调用 ?
   1. 居然就是生成两个 ko 文件
5. 初始化分别注册了 : device_driver 和 device 关系是什么 ?
6. 之前的 char 以及 block 设备 没有使用现在的内容但是依旧非常的正常呀 ?

## Plans for next
1. 设备驱动的书籍必然会存在关于 bus usb 等等的内容，找到另一本 device driver 的书籍


## bug

```c
? device_create_file+0x34/0xa0
device_add+0x3aa/0x630
? bex_unregister_driver+0x10/0x10 [bex]
? bex_unregister_driver+0x10/0x10 [bex]
device_register+0x16/0x20


EIP: bus_add_device+0x5d/0x150
```

```c
  /*
   * device_add does this:
   *    bus_add_device(dev)
   *    ->device_attach(dev)
   *      ->for each driver drv registered on the bus that dev is on
   *          if (dev.drv)  **  device already has a driver **
   *            ** not sure we could ever get here... **
   *          else
   *            if (bus.match(dev,drv)) [visorbus_match]
   *              dev.drv = drv
   *              if (!drv.probe(dev))  [visordriver_probe_device]
   *                dev.drv = NULL
   *
   * Note that device_add does NOT fail if no driver failed to claim the
   * device.  The device will be linked onto bus_type.klist_devices
   * regardless (use bus_for_each_dev).
   */
```

```c
/**
 * device_add - add device to device hierarchy.
 * @dev: device.
 *
 * This is part 2 of device_register(), though may be called
 * separately _iff_ device_initialize() has been called separately.
 *
 * This adds @dev to the kobject hierarchy via kobject_add(), adds it
 * to the global and sibling lists for the device, then
 * adds it to the other relevant subsystems of the driver model.
 *
 * Do not call this routine or device_register() more than once for
 * any device structure.  The driver model core is not designed to work
 * with devices that get unregistered and then spring back to life.
 * (Among other things, it's very hard to guarantee that all references
 * to the previous incarnation of @dev have been dropped.)  Allocate
 * and register a fresh new struct device instead.
 *
 * NOTE: _Never_ directly free @dev after calling this function, even
 * if it returned an error! Always use put_device() to give up your
 * reference instead.
 */
int device_add(struct device *dev)
```

# 9 network
@todo 主要介绍了比较上层的东西，对应的实验并没有做，做一下对于 socket 和 netfilter 的理解帮助应该是很大的

@todo 其中 e1000 的部分应该是之后的实验了，似乎非常的有趣。都是最近两个月添加，看上去非常的有意思啊!

## prepare-image.sh
- 似乎使用传统的启动方法，等等，你真的理解 **传统方法吗 ?**
  - 传统方法和现在的方法的区别是什么 ?
    - linuxrc
    - 还是命令行参数 root=/dev/sda
- 真的可以总结清楚 initramfs 在 boot 时候的作用吗 ?
  - 如何系统找到 initramfs ?
      - qemu 可以显示的指定
      - bootloader : 同时找到 kernel 和 rootfs
  - initramfs 的作用是什么 ?

# 8 fs
In particular, however, the VFS can use a normal file as a virtual block device, so it is possible to mount disk file systems over normal files. This way, stacks of file systems can be created.
> 这么神奇，能不能操作一下。

- `mount_bdev()`, which mounts a file system stored on a block device
- `mount_single()`, which mounts a file system that shares an instance between all mount operations
- `mount_nodev()`, which mounts a file system that is not on a physical device
- `mount_pseudo()`, a helper function for pseudo-file systems (sockfs, pipefs, generally file systems that can not be mounted)
> @todo 所以 `mount_nodev` 除了是 memory, 可以用于 mount 到文件中的吗?

`__bread()`: reads a block with the given number and given size in a buffer_head structure; in case of success returns a pointer to the buffer_head structure, otherwise it returns NULL;
sb_bread(): does the same thing as the previous function, but the size of the read block is taken from the superblock, as well as the device from which the read is done;
mark_buffer_dirty(): marks the buffer as dirty (sets the BH_Dirty bit); the buffer will be written to the disk at a later time (from time to time the bdflush kernel thread wakes up and writes the buffers to disk);
brelse(): frees up the memory used by the buffer, after it has previously written the buffer on disk if needed;
map_bh(): associates the buffer-head with the corresponding sector.
> 总结到位 !

fs 特定的 alloc_inode 作用 : 创建一个包含 vfs inode 的 inode 比如 minfs_inode，然后利用 init_node_once 初始化 vfs inode

## 问题
1. fs 和 dev 如何挂钩的 ?
    3. minfs.c:sb_bread 利用 superblock 中间的设备

1. 可以分析一下的内容:
    1. `inode_init_owner`
    2. `get_next_ino`

2. 在 myfs 和 minfs 中间选择的 `kill_sb` 的选择由于是否 block backed 而不同，不同之处在哪里 ?


3. `minfs_super_block` 和 `minfs_sb_info`


4. 如下的信息是否可以读懂
```plain
BUG: unable to handle kernel NULL pointer dereference at 00000000
*pde = 00000000
Oops: 0000 [#2] SMP
CPU: 0 PID: 294 Comm: mount Tainted: G      D    O      4.19.0+ #6
Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS ?-20191223_100556-ana4
EIP:   (null)
Code: Bad RIP value.
EAX: c8829000 EBX: 00000000 ECX: c6fcec40 EDX: 00008000
ESI: 00000000 EDI: c8829000 EBP: c6e6bed4 ESP: c6e6beb4
DS: 007b ES: 007b FS: 00d8 GS: 00e0 SS: 0068 EFLAGS: 00000296
CR0: 80050033 CR2: ffffffd6 CR3: 06a10000 CR4: 00000690
Call Trace:
 ? mount_fs+0x21/0x9e
 ? alloc_vfsmnt+0x104/0x170
 vfs_kern_mount.part.0+0x41/0x130
 ? ns_capable_common+0x3a/0x90
 do_mount+0x16f/0xc90
 ? _copy_to_user+0x51/0x70
 ksys_mount+0x6b/0xb0
 sys_mount+0x21/0x30
 do_int80_syscall_32+0x6a/0x1a0
 entry_INT80_32+0xcf/0xcf
EIP: 0x4490671e
Code: 90 66 90 66 90 66 90 66 90 66 90 90 57 56 53 8b 7c 24 20 8b 74 24 1c 8b 54 b
EAX: ffffffda EBX: bfd1df29 ECX: bfd1df32 EDX: bfd1df23
ESI: 00008000 EDI: 00000000 EBP: 00008000 ESP: bfd1d080
DS: 007b ES: 007b FS: 0000 GS: 0033 SS: 007b EFLAGS: 00000292
Modules linked in: minfs(O)
CR2: 0000000000000000
---[ end trace 206623a013b0f780 ]---
EIP:   (null)
Code: Bad RIP value.
EAX: c8829000 EBX: 00000000 ECX: c6fce720 EDX: 00008000
ESI: 00000000 EDI: c8829000 EBP: c7889ed4 ESP: c196747c
DS: 007b ES: 007b FS: 00d8 GS: 00e0 SS: 0068 EFLAGS: 00000296
CR0: 80050033 CR2: ffffffd6 CR3: 06a10000 CR4: 00000690
Killed
```

5. 找到一下获取 root inode 信息的代码在哪里 ? 从 ext2 中间分析。

6. 为什么需要设置 `inc_nlink` == 2 给 root inode

7. inode number 这个东西是全局共享的，还是每一个文件系统都是自己排布的 ?

8. 文件系统 和 设备自这两个试验之后，所以最终的问题在于 :
    1. 现在存在一个 hard disk，也存在一个 device driver，那么最终这个 device deriver 是如何找到这个 hard disk 的 ? 来分析一波 ucore 吧!

## 7 block devices
Although the register_blkdev() function obtains a major, it does not provide a device (disk) to the system. For creating and using block devices (disks), a specialized interface defined in linux/genhd.h is used.

Request queues implement an interface that allows the use of multiple I/O schedulers. A scheduler must sort the requests and present them to the driver in order to maximize performance. The scheduler also deals with the combination of adjacent requests (which refer to adjacent sectors of the disk).
> @todo 现在非常清楚将请求添加到 request queue 中间，但是
> 1. request queue 和 IO scheduler 的联系
> 2. IO scheduler 和 block device 驱动 ?
> 3. 从软件架构上 block device 提供了 request queue，那么岂不是其实 IO scheduler 其实只是一个可选项 ?

The function of type `request_fn_proc` is used to handle requests for working with the block device. This function is the equivalent of read and write functions encountered on character devices.


`blk_init_queue` 和  被取消掉

A request for a block device is described by **struct request** structure.

To use the content of a struct bio structure, the structure’s support pages must be mapped to the kernel address space from where they can be accessed. For mapping /unmapping, use the `kmap_atomic` and the `kunmap_atomic` macros.

For simplify the processing of a struct bio, use the `bio_for_each_segment` macrodefinition.
In case request queues are used and you needed to process the requests at struct bio level, use the `rq_for_each_segment` macrodefinition instead of the `bio_for_each_segment` macrodefinition.

`bio`, a dynamic list of struct bio structures that is a set of buffers associated to the request; this field is accessed by macrodefinition rq_for_each_segment if there are multiple buffers, or by bio_data macrodefinition in case there is only one associated buffer;
> segment 的含义，其实可以猜测为 bio 中间对应的一个 buffer 为一个 segment，表示连续的物理地址区间。
> 所以 bio_for_each_segment : 处理 bio 中间的所有 buffer，rq_for_each_segment 处理 rq 中间每一个 bio 的每一个 segment


9. 试验 6 : USE_BIO_TRANSFER 和之前的区别体现在什么地方 ?
    1. 测试显示，只是巧合而已，由于 bio 机制的原因，当 page 来自于同一个地方，那么并没有什么价值。
    2. 似乎使用 kmap_atomic 与否都是无所谓的 ? 并不是，还是相同的原因，bio_data 在只有一个 page 的时候可以用于获取地址。
    3. @todo 但是，kmap 在存在 highmem 下的作用还是一个谜!

10. 通过两个部分的试验 : bio request_queue 是上层，block driver 对于 bio 处理为下层任务
    1. 但是 IO scheduler 或者 block layer 对于 request_queue 的整合，现在不清楚的。
    2. 和具体物理设备如何打交道，目前不知道。

## TODO
1. IO scheduler 或者之类的东西到底是个什么情况 ?
2. partition 完全没有涉及到
3. 对于 blockdev 能不能采用类似 chardev 的 mknod ?
4. gendisk 和 blockdevice 两个结构体的作用是什么 ?

5. struct block_device 和 试验中间的 my_block_dev 是对称的存在的 ?
    1. 都存在 gendisk 和 request_queue
    2. 一个可用的 block device 总是需要持有: block_device
    3. 但是 data 和 size 找不到对应，很想知道最后走到 io 总线的位置是怎么样子的!
```c
static struct my_block_dev {
    spinlock_t lock;
    struct request_queue *queue;
    struct gendisk *gd;
    u8 *data;
    size_t size;
} g_dev;

struct block_device {
    dev_t           bd_dev;  /* not a kdev_t - it's a search key */
    int         bd_openers;
    struct inode *      bd_inode;   /* will die */
    struct super_block *    bd_super;
    struct mutex        bd_mutex;   /* open/close mutex */
    void *          bd_claiming;
    void *          bd_holder;
    int         bd_holders;
    bool            bd_write_holder;
#ifdef CONFIG_SYSFS
    struct list_head    bd_holder_disks;
#endif
    struct block_device *   bd_contains;
    unsigned        bd_block_size;
    u8          bd_partno;
    struct hd_struct *  bd_part;
    /* number of times partitions within this device have been opened. */
    unsigned        bd_part_count;
    int         bd_invalidated;
    struct gendisk *    bd_disk;
    struct request_queue *  bd_queue;
    struct backing_dev_info *bd_bdi;
    struct list_head    bd_list;
    /*
     * Private data.  You must have bd_claim'ed the block_device
     * to use this.  NOTE:  bd_claim allows an owner to claim
     * the same device multiple times, the owner must take special
     * care to not mess up bd_private for that case.
     */
    unsigned long       bd_private;

    /* The counter of freeze processes */
    int         bd_fsfreeze_count;
    /* Mutex for freeze */
    struct mutex        bd_fsfreeze_mutex;
} __randomize_layout;
```

6. 对于 insmod 的理解有点问题，为什么 /dev/myblock 自动给安排上了 ?

7. /dev/vbd 是什么时候注册的，哪一个驱动在管理其 ?

8. kmap_atomic
    1. 为什么会产生 bug
    2. 产生的 bug 信息为什么看不懂 ?

##
1. register_blkdev 和 add_disk 的区别:
    1. register_blkdev : 只要使用一个数值即可，和 register_chrdev_region 对称，cdev_add 之后将 cdev 和 设备号关联起来。

## 问题
需要找到硬件


# 5 IO Access
1. UART
2. IRQF_SHARED announces the kernel that the interrupt can be shared with other devices. If this flag is not set, then if there is already a handler associated with the requested interrupt, the request for interrupt will fail. A shared interrupt is handled in a special way by the kernel: all of the associated interrupt handlers will be executed until the device that generated the interrupt will be identified. But how can a device driver know if the interrupt handling routine was activated by an interrupt generated by the device it manages? Virtually all devices that offer interrupt support have a status register that can be interrogated in the handling routine to see if the interrupt was or was not generated by the device (for example, in the case of the 8250 serial port, this status register is IIR - Interrupt Information Register). When requesting a shared interrupt, the dev_id argument must be unique and it must not be NULL. Usually it is set to module’s private data.
> so what's purpose of dev_id of request_irq, it's not useful as a approach to distinguish who is real source of one interrupt.
3. ioperm

4. it right or false : interrupt context can't be put to sleep, but it still can be interrupt by other interrupt handler.
    1. nobody can sleep while holding a spin lock ?
    2. you can sleep with spinlock holed unless the spinlock will never required in the top half.
    3. You have to supress interrupt, if the spinlock is access on the top half, no matter you are the process context or interrupt context.

# Notes
1. 内核启动 arch/x86/kernel/setup.c 中间存在对于 IO 端口的硬编码来确定到底给谁使用。
2. 即使是已经端口被注册了，但是还是可以使用为该端口注册 irq。
4. 一共申请的资源为 :
    1. 设备号 : ...
    2. 中断号 : register_irq
    3. io 端口 : request_region
5. file->private 的作用


https://en.wikipedia.org/wiki/COM_(hardware_interface)
> device address port is hard coded !

Because such a situation(do a blocking operation) is relatively common, the kernel provides the request_threaded_irq() function to write interrupt handling routines running in two phases: a process-phase and an interrupt context phase:

During the initialization function (init_module()), or in the function that opens the device, interrupts must be activated for the device. This operation is dependent on the device, but most often involves setting a bit from the control register.

Because the interrupt handlers run in interrupt context the actions that can be performed are limited: unable to access user space memory, can’t call blocking functions.
**Also synchronization using spinlocks is tricky and can lead to deadlocks if the spinlock used is already acquired by a process that has been interrupted by the running handler.**

IFF you know that the spinlocks are never used in interrupt handlers, you can use the non-irq versions.

If we want to disable interrupts at the interrupt controller level (not recommended because disabling a particular interrupt is slower, we can not disable shared interrupts) we can do this with disable_irq(), disable_irq_nosync(), and enable_irq(). Using these functions will disable the interrupts on all processors.

> https://stackoverflow.com/questions/5934402/can-an-interrupt-handler-be-preempted
> All irqs are not disabled by default only the same irq is disabled on all processors. but with flags in request_irq, you can disable all other interrupts on local processor while serving the interrupt.

The KeyBoard driver is really intesting :
1. Keyboard initialization function i8042_setup_kbd()
2. The AT or PS/2 keyboard interrupt function atkbd_interrupt()

```c
static int __init i8042_setup_kbd(void)
{
    int error;

    error = i8042_create_kbd_port();
    if (error)
        return error;

    error = request_irq(I8042_KBD_IRQ, i8042_interrupt, IRQF_SHARED,
                "i8042", i8042_platform_device);
    if (error)
        goto err_free_port;

    error = i8042_enable_kbd_port(); // tell the device, everything is ok, start working now !
    if (error)
        goto err_free_irq;

    i8042_kbd_irq_registered = true;
    return 0;

 err_free_irq:
    free_irq(I8042_KBD_IRQ, i8042_platform_device);
 err_free_port:
    i8042_free_kbd_port();
    return error;
}


/*
 * i8042_interrupt() is the most important function in this driver -
 * it handles the interrupts from the i8042, and sends incoming bytes
 * to the upper layers.
 */

static irqreturn_t i8042_interrupt(int irq, void *dev_id)
// /home/shen/Core/linux/drivers/input/serio/i8042.c
// XXX although very important, but it's fairly simple
// read data by i8042_read_data

static inline int i8042_read_data(void)
{
    return inb(I8042_DATA_REG);
}
```

```c
struct serio_driver { // todo so, what's the relation between serio_driver and keyboard ?
    const char *description;

    const struct serio_device_id *id_table;
    bool manual_bind;

    void (*write_wakeup)(struct serio *);
    irqreturn_t (*interrupt)(struct serio *, unsigned char, unsigned int);
    int  (*connect)(struct serio *, struct serio_driver *drv);
    int  (*reconnect)(struct serio *);
    int  (*fast_reconnect)(struct serio *);
    void (*disconnect)(struct serio *);
    void (*cleanup)(struct serio *);

    struct device_driver driver;
};
```

## Exercise
1. [](https://en.wikipedia.org/wiki/Keyboard_controller_(computing))
2. https://wiki.osdev.org/%228042%22_PS/2_Controller#Initialising_the_PS.2F2_Controller


Let us do more thinking before fulfill the simple task :
1. register_chrdev_region
2. cdev_init
3. cdev_add

why we have to register char device,

# 2 Kernel module

1. `module_init()` and `module_exit()`
2. `ignore_loglevel`
3. why pr_debug() can print to the terminal ?
    1. what's the difference with printk ?
4. why we can corrupt /tmp/tmp.QXJzapOmjJ ?  What's that ?

http://tldp.org/LDP/lkmpg/2.6/html/x323.html

5. https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=0b999ae3614d09d97a1575936bcee884f912b10e
```c
static int __init cmd_init(void)
{
    pr_info("Early bird gets %s\n", str);
    return 0;
}
```
6. we are not able to boot : /home/shen/Core/hack-linux-kernel/tools/labs/core-image-sato-qemux86.ext4
7. extra exercise 1 4 5 : didn't worked
    1. echo hvc0 > /sys/module/kgdboc/parameters/kgdboc :
    2. echo g > /proc/sysrq-trigger
    3. ctrl+O g
    4. /debug/dynamic_debug/control is protected from writing !
