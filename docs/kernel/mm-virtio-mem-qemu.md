## virtio mem qemu 侧分析

```txt
(qemu) info memory-devices
Memory device [virtio-mem]: "vm0"
  memaddr: 0x140000000
  node: 0
  requested-size: 134217728
  size: 134217728
  max-size: 2147483648
  block-size: 2097152
  memdev: /objects/mem3
Memory device [virtio-mem]: "vm1"
  memaddr: 0x1c0000000
  node: 1
  requested-size: 83886080
  size: 83886080
  max-size: 2147483648
  block-size: 2097152
  memdev: /objects/mem4
```

- [ ] 这几个字段是啥意思


https://patchwork.kernel.org/project/kvm/cover/20191212171137.13872-1-david@redhat.com/

```txt
qom-set vm0 requested-size 64M
```

可以得到这个日志:

```txt
[  127.372411] virtio_mem virtio0: plugged size: 0x40000000
[  127.376059] virtio_mem virtio0: requested size: 0x4000000
```

- [ ] 为什么没有检测到 zone movable 里面的 pages 的数量，里面总是 0


```c
struct VirtIOMEM {

    /* usable region size (<= region_size) */
    uint64_t usable_region_size;

    /* actual size (how much the guest plugged) */
    uint64_t size;

    /* requested size */
    uint64_t requested_size;

    /* block size and alignment */
    uint64_t block_size;
```

- size : 当前在 plug 的大小
- requested_size : 期望的大小

## qemu

- virtio_mem_info 和 virtio_mem_pci_info 居然不是父子关系 ?

```c
struct VirtIOMEMPCI {
    VirtIOPCIProxy parent_obj; // @todo 这是个啥 ?
    VirtIOMEM vdev;
    Notifier size_change_notifier;
};
```

- [ ] sbm 和 bbm 的兼容性如何?

## [ ] QEMU 也是
- virtio_mem_handle_request

## [ ] 检测 QEMU 发送消息的路径
