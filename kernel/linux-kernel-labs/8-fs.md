## Notes
In particular, however, the VFS can use a normal file as a virtual block device, so it is possible to mount disk file systems over normal files. This way, stacks of file systems can be created.
> 这么神奇，能不能操作一下。

- mount_bdev(), which mounts a file system stored on a block device
- mount_single(), which mounts a file system that shares an instance between all mount operations
- mount_nodev(), which mounts a file system that is not on a physical device
- mount_pseudo(), a helper function for pseudo-file systems (sockfs, pipefs, generally file systems that can not be mounted)
> @todo 所以 mount_nodev 除了是 memory, 可以用于 mount 到文件中的吗?

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
    1. inode_init_owner
    2. get_next_ino

2. 在 myfs 和 minfs 中间选择的 kill_sb 的选择由于是否 block backed 而不同，不同之处在哪里 ?


3. minfs_super_block 和 minfs_sb_info


4. 如下的信息是否可以读懂
```
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

6. 为什么需要设置 inc_nlink == 2 给 root inode

7. inode number 这个东西是全局共享的，还是每一个文件系统都是自己排布的 ?

8. 文件系统 和 设备自这两个试验之后，所以最终的问题在于 :
    1. 现在存在一个 hard disk，也存在一个 device driver，那么最终这个 device deriver 是如何找到这个 hard disk的 ? 来分析一波 ucore 吧!
