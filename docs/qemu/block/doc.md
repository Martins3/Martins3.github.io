# https://www.linux-kvm.org/images/b/b5/2012-fourm-block-overview.pdf


On the command line:
-hda test.img
...is a shortcut for...
-drive file=test.img,if=ide,cache=writeback,aio=threads
...is a shortcut for...
-drive file=test.img,id=ide0-hd0,if=none,cache=writeback,aio=threads
-device ide-drive,bus=ide.0,drive=ide0-hd0

SCSI passthrough

访问方法:
```plain
x86_64-softmmu/qemu-system-x86_64 -device ide-drive,help
```

VMDK, VHD, VDI...
- Provided for compatibility
- Best to convert to raw or qcow2 for running VMs


- Usually you don’t want to use the host page cache
    - The guest has already a page cache
    - Data would be duplicated – waste of memory
- But it can make sense in some cases
    - Many guests sharing the host cache
    - Short-lived guests
- Must bypass host page cache for safe live migration

**External snapshots** (backing files):
    -- base -- sn1 -- sn2 -- sn3
- COW layer over backing files (of any image format) saves delta
- Cheap to create
- Deleting a snapshot means copying all data

**Internal snapshots** (savevm/loadvm, qcow2 only):
- Snapshot saved in the same image file
- Creation and deletion both with some cost
- Modify metadata, but no copy of data required
- Can contain VM state
- No live snapshots (VM stops while saving snapshot)
- Receives less testing ⇒ Stability?

- [ ] 表示并没有太看懂 Internal snapshots
    - 如果 Snapshot 都是在一个文件的，那么是不是相同的部分也拷贝了
    - 什么叫做修改 metadata 但是不用拷贝数据
    - 为什么 external VM 不能进行含有 VM state
    - 为什么不能 live snapshot 的，是不是 live snapshot 就是将运行的所有的 io 捕获下来的。

- Long-running background jobs on block devices
    - Live storage migration
    - Deleting external snapshots

**image streaing**
Use cases:
- Copy an image from a slow source in the background while running the VM
- Delete topmost external snapshots

- [ ] 迷茫啊？

# http://events17.linuxfoundation.org/sites/events/files/slides/talk_11.pdf

总是如此，直接穿过这里的:
- Data manipulation:
    - Filter drivers (e.g. throttle, quorum)
- Interpret image formats:
    - Format drivers (e.g. qcow2)
- Accessing host storage:
    - Protocol drivers (e.g. file, nbd)

- `BlockdevOptionsFile`

- [ ] 前面还是知道在干什么的，但是后面的就感觉活在梦里了。
