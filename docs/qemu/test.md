# qemu 如何做测试的
<!-- 71610027-687c-4ae8-829f-e7ff51a0be09 -->

- https://www.qemu.org/docs/master/devel/testing/main.html
- https://wiki.qemu.org/Testing/Machines
- docs/kvm/kvm-forum/2025.md 中的 The next generation QEMU functional testing framework 也可以看看

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

## qtest
./qtest.md 中分析

## Unit test

单元测试会编译成普通的宿主机测试程序，把部分 QEMU object 链接进去，然后直接调用接口：
源码通常在 tests/unit，由 GLib 的 g_test 框架组织。

类似这种
```txt
  result = function_under_test(input);
  g_assert_cmpint(result, ==, expected);
```

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
