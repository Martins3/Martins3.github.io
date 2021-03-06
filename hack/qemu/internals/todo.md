# qemu overview

- [ ] ./replay 只有 1000 行左右，值得分析一下。
- [ ] tcg 相关联的代码在什么位置 ?

## qga
生成一个运行在虚拟机中间的程序，然后和 host 之间进行通信。

## structure
- 入口应该是 ./softmmu/main.c

- virtio
  - hw/block/virtio-blk.c
  - hw/net/virtio-blk.c
  - hw/virtio

- hw/vfio


## chardev
chardev 的一种使用方法[^2][^3], 可以将在 host 和 guest 中间同时创建设备，然后 guest 和 host 通过该设备进行交互。
-chardev 表示在 host 创建的设备，需要有一个 id, -device 指定该设备

- [x] -device virtio-serial 是必须的 ?
  - [x] 和 -device virtio-serial-bus 的关系是什么 ?
    - 似乎只有存在了 virtio-serial-bus 之后才可以将  virtio console 挂载到上面去

./chardev 就是为了支持方式将 guest 的数据导出来, 但是 guest 那边的数据一般来说 virtio 设备了
./hw/char 中间是为了对于 guest 的模拟和 host 端的 virtio 实现

## blockdev
- qemu 的 image 是支持多种模式的, 而 kvmtool 只是支持一个模式，如果
- qcow2 : qemu copy on write 的 image 格式

blockdev 文件夹下为了支持各种种类 image 访问方法，甚至可以直接访问 nvme 的方法


## capstone
- 显然 capstone 是被调用过的，在 qemu 看到的代码都是直接一条条的分析的

编译方法
```
➜  capstone git:(master) ✗ CAPSTONE_ARCHS="x86" bear make -j10
```

和 capstone 的玩耍:
- ./capstone
- [ref](http://www.capstone-engine.org/lang_c.html)

其实每一个架构的代码是很少的

## migration
- [ ] 有意思的东西

## monitor
qmp 让 virsh 可以和 qemu 交互

- [ ] 学会使用 :  https://libvirt.org/manpages/virsh.html

## scsi
scsi 多增加了一个抽象层次，导致其性能上有稍微的损失，但是存在别的优势。[^5][^6]
> Shortcomings of virtio-blk include a small feature set (requiring frequent updates to both the host and the guests) and limited scalability. [^7]

和实际上，scsi 文件夹下和 vritio 关系不大，反而是用于 persistent reservation
https://qemu.readthedocs.io/en/latest/tools/qemu-pr-helper.html

- [ ] pr 只是利用了 scsi 机制，但是非要使用 scsi, 不知道

## trace
- [ ] 为什么需要使用 ftrace，非常的 interesting !



[^1]: https://developer.apple.com/documentation/hypervisor
[^2]: https://stackoverflow.com/questions/63357744/qemu-socket-communication
[^3]: https://wiki.qemu.org/Features/ChardevFlowControl
[^4]: https://qkxu.github.io/2019/03/24/Qemu-Guest-Agent-(QGA)%E5%8E%9F%E7%90%86%E7%AE%80%E4%BB%8B.html
[^5]: https://mpolednik.github.io/2017/01/23/virtio-blk-vs-virtio-scsi/
[^6]: https://stackoverflow.com/questions/39031456/why-is-virtio-scsi-much-slower-than-virtio-blk-in-my-experiment-over-and-ceph-r
[^7]: https://wiki.qemu.org/Features/SCSI
