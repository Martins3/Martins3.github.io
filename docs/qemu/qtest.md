# qtest

qtest 主要是用于设备模拟的验证，所谓设备模拟，就是对于设备发送了命令，
然后设备的响应应该是预期的。

## 基本操作
```txt
cd build
QTEST_QEMU_BINARY=./qemu-system-x86_64 ./tests/qtest/qtest-demo-test --verbose
QTEST_QEMU_BINARY=./qemu-system-x86_64 QTEST_LOG=1 ./tests/qtest/qtest-demo-test --verbose
QTEST_QEMU_BINARY=./qemu-system-x86_64 QTEST_LOG=1 ./tests/qtest/qtest-demo-test -p /x86_64/qtest-demo/basics --verbose

QTEST_QEMU_BINARY=./qemu-system-x86_64 QTEST_QEMU_IMG=./qemu-img ./tests/qtest/qos-test \
-p /x86_64/pc/i440FX-pcihost/pci-bus-pc/pci-bus/virtio-blk-pci/virtio-blk/virtio-blk-tests/basic \
      virtio-blk-tests/basic
```

然后会启动的内核参数为:
```txt
exec ./qemu-system-x86_64 -qtest unix:/tmp/qtest-1606169.sock -qtest-log /dev/null -chardev socket,path=/tmp/qtest-1606169.qmp,id=char0 -object monitor-qmp,id=qmp0,chardev=char0 -display none -audio none -run-with exit-with-parent=on -M pc  -device virtio-blk-pci,id=drv0,drive=drive0,addr=4.0 -drive if=none,id=drive0,file=/tmp/qtest.VH3TT3,format=raw,auto-read-only=off -drive if=none,id=drive1,file=null-co://,file.read-zeroes=on,format=raw -accel qtest
```

直接:
```txt
QTEST_QEMU_BINARY=./build/qemu-system-x86_64 QTEST_QEMU_IMG=./build/qemu-img gdb  \
--args ./build/tests/qtest/qos-test -p /x86_64/pc/i440FX-pcihost/pci-bus-pc/pci-bus/virtio-blk-pci/virtio-blk/virtio-blk-tests/basic
```

如何测试 tests/qtest/virtio-blk-test.c

qtest 协议是简单的行式请求/响应协议，例如：

```txt
writel 0xfee00000 0x1234
readl 0xfee00000
clock_step
```

## 基础设施

### -accel qtest

好吧，这真的非常有想象力了，给我老 QEMU 玩家一点小小心灵震撼
也让我重新梳理了一下 QEMU 的运行模型:

普通的 io 模型中:

```txt
vCPU		device simulation				physical device

写 mmio
		vCPU thread 执行模拟逻辑(没有 ioeventfd)
								提交给硬件

		main loop 中接受到 physical device
		完成任务，模拟完成剩下的逻辑
```
那么，vCPU 主要做 device 行为的触发

```c
static void qtest_accel_ops_class_init(ObjectClass *oc, const void *data)
{
    AccelOpsClass *ops = ACCEL_OPS_CLASS(oc);

    ops->create_vcpu_thread = dummy_start_vcpu_thread;
    ops->get_virtual_clock = qtest_get_virtual_clock;
    ops->set_virtual_clock = qtest_set_virtual_clock;
    ops->handle_interrupt = generic_handle_interrupt;
};
```

### -object qtest

1. system/qtest.c 来在 qemu 内部，负责接受

2. 测试客户端是 tests/qtest/libqtest.c ，负责制作这样的命令
```txt
write <MMIO地址> <长度> <数据>
```
- libqos：像一个很小的 guest driver library，封装 PCI 枚举、virtqueue、内存分配等。
- qgraph：描述 machine、bus、device、driver 的连接关系，自动为同一设备生成多种机器配置的测试路径。

### 运行模型

#### 1. 直接构造 guest 内存

tests/qtest/virtio-blk-test.c 中的 test_basic()：

- 分配 guest 内存
- 填写 VirtIOBlock 请求头
- 构造 descriptor chain
- 把 descriptor head 放进 available ring
- 调用 qvirtqueue_kick()

#### 2. 模拟 vCPU 内存

qvirtqueue_kick() 写 notify 寄存器

最后执行到 tests/qtest/libqos/virtio-pci-modern.c

qpci_io_writew(..., notify_offset, vq->index);

这个写操作由 libqtest 转换成类似下面的

QEMU 收到 qtest 命令，将其传递给设备模拟:

- qtest_process_command
  - address_space_write
    - memory_region_dispatch_write_eventfds
      - event_notifier_set
        - AIO 主循环收到 eventfd
          - virtio_queue_host_notifier_read
            - virtio_queue_notify_vq
              - vq->handle_output()

### 实际测试

每一个 qtest 本来是一个 process ，然后在 fork 出来 qemu ，然后和 qemu 通信:

- main
  - g_test_run
    - g_test_run_suite
      - g_test_run_suite_internal
        - g_test_run_suite_internal
          - g_test_run_suite_internal
            - test_qtest_demo

codex 提供了一个很强的技巧来调试被 fork 出来的 qemu
```sh
#!/usr/bin/env bash

set -E -e -u -o pipefail

export QTEST_QEMU_BINARY='sh -c '\''case " $* " in *" -machine none "*) exec ./build/qemu-system-x86_64 "$@" ;; *) exec gdb -q -ex "break virtio_blk_handle_vq" --args ./build/qemu-system-x86_64 "$@" ;; esac'\'' sh'
export QTEST_QEMU_IMG='./build/qemu-img'

exec ./build/tests/qtest/qos-test \
	-p '/x86_64/pc/i440FX-pcihost/pci-bus-pc/pci-bus/virtio-blk-pci/virtio-blk/virtio-blk-tests/basic'
```

可以观察到， qemu 中还是原来的 thread ，只是 vCPU thread 换掉了，以前 vCPU 触发的动作，
现在可以让 qtest 协议通过外部来触发,
```txt
$ info threads
  Id   Target Id                                             Frame
* 1    Thread 0x7ffff4628f40 (LWP 1820374) "qemu-system-x86" virtio_blk_handle_vq (s=0x5555582bd870, vq=0x5555582f2770) at ../hw/block/virtio-blk.c:1020
  2    Thread 0x7ffff43ff6c0 (LWP 1820377) "call_rcu"        0x00007ffff7d2320d in syscall () from /nix/store/57iz36553175g3178pvxjij8z5rcsd4n-glibc-2.42-61/lib/libc.so.6
  3    Thread 0x7ffff31fc6c0 (LWP 1820378) "IO mon_iothread" 0x00007ffff7ca6922 in __syscall_cancel_arch () from /nix/store/57iz36553175g3178pvxjij8z5rcsd4n-glibc-2.42-61/lib/libc.so.6
  4    Thread 0x7ffff1f586c0 (LWP 1820379) "CPU 0/DUMMY"     0x00007ffff7ca6922 in __syscall_cancel_arch () from /nix/store/57iz36553175g3178pvxjij8z5rcsd4n-glibc-2.42-61/lib/libc.so.6
  5    Thread 0x7fffe2bff6c0 (LWP 1820380) "worker"          0x00007ffff7ca6922 in __syscall_cancel_arch () from /nix/store/57iz36553175g3178pvxjij8z5rcsd4n-glibc-2.42-61/lib/libc.so.6
```

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - tcp_chr_read
                  - qtest_process_inbuf
                    - qtest_process_command
                      - address_space_stm_internal
                        - memory_region_dispatch_write
                          - memory_region_dispatch_write_eventfds

## 跨架构支持

我发现其实在 x86 上也是支持运行 aarch64 的测试的，仔细想了一下，这是非常合理的:

在 x86 主机上，qemu-system-aarch64 本身是一个 x86 原生程序，而且由于 qtest 使用了自己定义的
accel engine ，连 tcg 都不需要了，所以 x86 完全可以模拟任何 aarch64 的设备行为。

## 参考资料
1. https://www.bilibili.com/video/BV1pA411A7rZ
2. codex

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
