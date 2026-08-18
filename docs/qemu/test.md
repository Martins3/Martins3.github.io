# qemu 如何做测试的
<!-- 71610027-687c-4ae8-829f-e7ff51a0be09 -->

- https://www.qemu.org/docs/master/devel/testing/main.html
- https://wiki.qemu.org/Testing/Machines
- docs/kvm/kvm-forum/2025.md 中的 The next generation QEMU functional testing framework 也可以看看

<!-- 差不多了，对于 qtest 到时候可以继续清理下 -->

## 基本尝试

- make check V=1 -j
	- 执行所有的测试
- make check-unit V=1

看来 unit test 也是链接所有的东西
```txt
🧀  l tests/unit/test-aio
Permissions Size User     Date Modified Name
.rwxr-xr-x   11M martins3  6 Jan 21:58   tests/unit/test-aio
```

- make check-qtest

```txt
🧀  make check-help
Regression testing targets:
 make check                    Run block, qapi-schema, unit, softfloat, qtest and decodetree tests
 make bench                    Run speed tests

Individual test suites:
 make check-qtest-TARGET       Run qtest tests for given target
 make check-qtest              Run qtest tests
 make check-functional         Run python-based functional tests
 make check-functional-TARGET  Run functional tests for a given target
 make check-unit               Run qobject tests
 make check-qapi-schema        Run QAPI schema tests
 make check-tracetool          Run tracetool generator tests
 make check-block              Run block tests
 make check-tcg                Run TCG tests
 make check-softfloat          Run FPU emulation tests

 make check-report.junit.xml   Generates an aggregated XML test report
 make check-venv               Creates a Python venv for tests
 make check-clean              Clean the tests and related data

The following are useful for CI builds
 make check-build              Build most test binaries


The variable SPEED can be set to control the gtester speed setting.
Default options are -k and (for make V=1) --verbose; they can be
changed with variable GTESTER_OPTIONS.
```


> [!NOTE]
> 参考神奇海螺的意见，有待验证

## 总结

可以用被修改代码的边界判断应该添加什么测试
  - 改工具函数、容器、解析器：写 unit test。
  - 改 PCI、virtio、网卡、控制器设备模型：优先 qtest。
  - 改启动流程、machine、固件交互：functional test。
  - 改 qcow2、block job、NBD：qemu-iotests。
  - 改 target/*、TCG helper 或指令翻译：check-tcg。
  - 改 QAPI/schema/generator：参考输出测试。

  一个比较理想的设备修复通常是：

  小型 qtest 精确复现 bug
          +
  functional test 验证真实 guest 场景（必要时）

  前者负责快速、稳定、精确地防止回归；后者负责确认完整系统组合确实工作。

• QEMU 的测试可以理解成一套“分层验证体系”：越靠下越快、越精确，越靠上越接近真实虚拟机，但更慢、变量也更多。

   测试类型                 驱动对象                              主要验证
  ━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   Unit test                直接调用 C 函数                       数据结构、算法、公共组件
  ───────────────────────  ────────────────────────────────────  ──────────────────────────────────────
   qtest                    启动 QEMU，但通常不运行 guest CPU     PCI/virtio/中断/DMA 等设备模型
  ───────────────────────  ────────────────────────────────────  ──────────────────────────────────────
   Functional test          启动完整虚拟机和 guest payload        启动、固件、设备、整机功能
  ───────────────────────  ────────────────────────────────────  ──────────────────────────────────────
   qemu-iotests             QEMU block layer、qemu-img、NBD 等    qcow2、快照、块任务、错误路径
  ───────────────────────  ────────────────────────────────────  ──────────────────────────────────────
   check-tcg                在目标架构执行小程序                  指令翻译、异常、系统调用
  ───────────────────────  ────────────────────────────────────  ──────────────────────────────────────
   Schema/generator test    输入与参考输出对比                    QAPI、decodetree、tracetool 等生成器

```txt
  make check-help
  make check
  make check-unit
  make check-qtest
  make check-functional
  make check-block
  make check-tcg
```

  当前源码里的入口可见 tests/Makefile.include:3，总说明在 docs/devel/testing/main.rst:1。

## Unit test

单元测试会编译成普通的宿主机测试程序，把部分 QEMU object 链接进去，然后直接调用接口：
源码通常在 tests/unit，由 GLib 的 g_test 框架组织。

类似这种
```txt
  result = function_under_test(input);
  g_assert_cmpint(result, ==, expected);
```

## qtest

qtest 主要是用于设备模拟的验证，所谓设备模拟，就是对于设备发送了命令，
然后设备应该

基本操作
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
exec ./qemu-system-x86_64 -qtest unix:/tmp/qtest-1606169.sock -qtest-log /dev/null -chardev socket,path=/tmp/qtest-1606169.qmp,id=char0 -object monitor-qmp,id=qmp0,chardev=char0 -display none -audio none -run-with exit-with-parent=on -M pc  -device virtio-blk-pci,id=drv0,drive=drive0,addr=4.0 -drive if=none,id=drive0,file=/tmp/qtest.VH3TT3,format=raw,auto-read-only=off -drive if=none,id=drive1,file=null-co://,file.read-zeroes=on,format=raw   -accel qtest
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

### qtest 实现了一个自定义的 accel engine

好吧，这真的非常有想象力了。

协议实现在 system/qtest.c
测试客户端是 tests/qtest/libqtest.c

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


### 运行模型

1. 测试构造块请求

tests/qtest/virtio-blk-test.c:114 中的 test_basic()：

- 分配 guest 内存
- 填写 VirtIOBlock 请求头
- 构造 descriptor chain
- 把 descriptor head 放进 available ring
- 调用 qvirtqueue_kick()

2. qvirtqueue_kick() 写 notify 寄存器

tests/qtest/libqos/virtio.c:399：

qvirtqueue_kick(...)
{
    /* 更新 avail->ring 和 avail->idx */
    ...
    d->bus->virtqueue_kick(d, vq);
}

现代 virtio-pci 最后执行：

tests/qtest/libqos/virtio-pci-modern.c:250

qpci_io_writew(..., notify_offset, vq->index);

这个写操作由 libqtest 转换成类似下面的 qtest 协议命令：

write <MMIO地址> <长度> <数据>

3. QEMU 收到 qtest 命令

system/qtest.c:355 中的 qtest_process_command() 解析 write，然后调用：

address_space_write(first_cpu->as, ...);

所以 qtest 并没有一个这样的高级命令：

submit-virtio-blk-request

它只提供 read/write/outw 这类总线访问原语。真正懂 virtio 协议、构造 descriptor 的是测试端的
libqos。

4. MMIO 写触发 virtqueue kick

你这个测试启用了 ioeventfd，因此实际路径是：

qtest_process_command
  → address_space_write
  → memory_region_dispatch_write_eventfds
  → event_notifier_set
  → AIO 主循环收到 eventfd
  → virtio_queue_host_notifier_read
  → virtio_queue_notify_vq
  → vq->handle_output()

队列回调在 hw/block/virtio-blk.c:1817 注册：

virtio_add_queue(vdev, conf->queue_size,
                 virtio_blk_handle_output);

然后：

virtio_blk_handle_output
  → virtio_blk_handle_vq
  → virtio_blk_handle_request
  → blk_aio_pwritev / blk_aio_preadv


#### 验证确认

- main
  - g_test_run
    - g_test_run_suite
      - g_test_run_suite_internal
        - g_test_run_suite_internal
          - g_test_run_suite_internal
            - test_qtest_demo

codex 提供了一个很强的技巧来调试
```txt
#!/usr/bin/env bash

set -E -e -u -o pipefail

export QTEST_QEMU_BINARY='sh -c '\''case " $* " in *" -machine none "*) exec ./build/qemu-system-x86_64 "$@" ;; *) exec gdb -q -ex "break virtio_blk_handle_vq" --args ./build/qemu-system-x86_64 "$@" ;; esac'\'' sh'
export QTEST_QEMU_IMG='./build/qemu-img'

exec ./build/tests/qtest/qos-test \
	-p '/x86_64/pc/i440FX-pcihost/pci-bus-pc/pci-bus/virtio-blk-pci/virtio-blk/virtio-blk-tests/basic'
```

可以观察到， qemu 中还是原来的 thread ，只是 vCPU thread 换掉了，以前 vCPU 触发的动作，
现在可以让 qtest 协议通过外部来触发, 进而提交命令给
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
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_machine_creation_done
        - qdev_machine_creation_done
          - qemu_system_reset
            - pc_machine_reset
              - resettable_reset
                - resettable_assert_reset
                  - resettable_phase_hold
                    - resettable_child_foreach
                      - resettable_container_child_foreach
                        - resettable_phase_hold
                          - resettable_child_foreach
                            - bus_reset_child_foreach
                              - resettable_phase_hold
                                - hwcore::qdev::rust_resettable_hold_fn<hpet::device::HPETState>
                                  - hpet::device::HPETState::reset_hold
                                    - hwcore::irq::InterruptSource<bool>::set<bool>
                                      - qemu_set_irq
                                        - pit_irq_control
                                          - pit_irq_timer_update
                                            - qemu_set_irq
                                              - hwcore::qdev::DeviceMethods::init_gpio_in::rust_irq_handler<hpet::device::HPETState, fn
                                                - }::call<fn
                                                  - common::callbacks::{impl
                                                    - core::ops::function::Fn::call<fn
                                                      - hpet::device::HPETState::handle_legacy_irq
                                                        - hwcore::irq::InterruptSource<bool>::set<bool>
                                                          - qemu_set_irq
                                                            - gsi_handler
                                                              - qemu_set_irq
                                                                - ioapic_service
                                                                  - address_space_stm_internal
                                                                    - memory_region_dispatch_write
                                                                      - memory_region_dispatch_write_eventfds
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

所以，基本流程的确就是如此:


### 虚拟时钟

qtest 允许测试程序直接推进 QEMU_CLOCK_VIRTUAL：

```txt
  clock_step
  clock_step 1000000
  clock_set 5000000
```

因此定时器测试可以变成确定性过程：

```txt
  设置设备定时器
  → 精确推进 1 ms
  → 检查中断是否出现
```

  这比依赖真实时间稳定得多。

### libqtest、libqos 和 qgraph

这三层可以这样理解：

- libqtest：原始能力，如读写 MMIO、内存、IRQ、时钟。
- libqos：像一个很小的 guest driver library，封装 PCI 枚举、virtqueue、内存分配等。
- qgraph：描述 machine、bus、device、driver 的连接关系，自动为同一设备生成多种机器配置的测试路径。

### 跨架构

我发现其实在 x86 上也是支持运行 aarch64 的测试的，仔细想了一下，这是非常合理的:

在 x86 主机上，qemu-system-aarch64 本身是一个 x86 原生程序，而且由于 qtest 使用了自己定义的
accel engine ，连 tcg 都不需要了，所以 x86 完全可以模拟任何 aarch64 的设备行为。


## Functional test：启动真实的虚拟机测试
说明见 docs/devel/testing/functional.rst

Functional test 通常是 Python 测试，使用 QemuSystemTest 启动完整 QEMU，加载 kernel、initrd、固件或磁盘镜像，然后通过：

  - 串口输出；
  - QMP；
  - guest 返回值；
  - 文件或网络结果；

  判断测试是否成功。

  例如：

  启动 ARM 开发板模型
  → 加载 Linux kernel
  → 等待串口出现登录提示
  → 执行或观察某项功能
  → 检查输出

  它能覆盖 qtest 覆盖不到的组合：

  CPU + firmware + machine + device + guest kernel

  代价是启动慢、可能需要下载测试镜像，而且更容易受到超时、网络和 guest 行为影响。

  入口是：

  make check-functional
  make check-functional-x86_64


## 领域专用
### qemu-iotests：块层的专门测试

测试

- raw / qcow2 / vmdk
- 存储协议: file / NBD / iSCSI
- snapshot / bitmap / mirror / commit / backup

  cd build/tests/qemu-iotests

```txt
  ./check -qcow2
  ./check -raw
  ./check -qcow2 001 030 153
```

### check-tcg：验证 CPU 翻译执行

TCG 测试与 qtest 恰好相反：

- qtest 尽量不运行 guest CPU，主要测试设备。
- check-tcg 专门让 guest CPU 执行代码，主要测试翻译器。

基本过程：

交叉编译一个目标架构的小程序
→ 用 qemu-aarch64 或 qemu-system-aarch64 执行
→ 检查寄存器、内存、异常或程序退出结果

它可以验证：

- 某条指令的语义；
- condition flags；
- 原子操作；
- MMU/异常处理；
- linux-user syscall；
- TCG plugin 和 gdbstub。

通常需要目标架构的交叉编译器：

make check-tcg
make run-tcg-tests-aarch64-linux-user

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
