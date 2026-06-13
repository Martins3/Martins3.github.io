# Linux kernel Labs 笔记


## 问题
1. hotplug 和 plug and play 的关系是什么 ?
2. uevent ?
4. 居然存在两个 init_module，那么到底如何调用 ?
   1. 居然就是生成两个 ko 文件
5. 初始化分别注册了 : device_driver 和 device 关系是什么 ?
6. 之前的 char 以及 block 设备 没有使用现在的内容但是依旧非常的正常呀 ?

## Plans for next
1. 设备驱动的书籍必然会存在关于 bus usb 等等的内容，找到另一本 device driver 的书籍

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
3. 对于 blockdev 能不能采用类似 chardev 的 mknod ?

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
```

6. 对于 insmod 的理解有点问题，为什么 /dev/myblock 自动给安排上了 ?

# 5 IO Access
1. UART
2. IRQF_SHARED announces the kernel that the interrupt can be shared with other devices. If this flag is not set, then if there is already a handler associated with the requested interrupt, the request for interrupt will fail. A shared interrupt is handled in a special way by the kernel: all of the associated interrupt handlers will be executed until the device that generated the interrupt will be identified. But how can a device driver know if the interrupt handling routine was activated by an interrupt generated by the device it manages? Virtually all devices that offer interrupt support have a status register that can be interrogated in the handling routine to see if the interrupt was or was not generated by the device (for example, in the case of the 8250 serial port, this status register is IIR - Interrupt Information Register). When requesting a shared interrupt, the dev_id argument must be unique and it must not be NULL. Usually it is set to module’s private data.
> so what's purpose of dev_id of request_irq, it's not useful as a approach to distinguish who is real source of one interrupt.
3. ioperm

4. it right or false : interrupt context can't be put to sleep, but it still can be interrupt by other interrupt handler.
    1. nobody can sleep while holding a spin lock ?
    2. you can sleep with spinlock holed unless the spinlock will never required in the top half.
    3. You have to supress interrupt, if the spinlock is access on the top half, no matter you are the process context or interrupt context.


# 2 Kernel module
2. `ignore_loglevel`

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
