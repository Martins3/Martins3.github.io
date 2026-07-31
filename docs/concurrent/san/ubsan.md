## ubsan

Documentation/dev-tools/ubsan.rst
  - https://www.kernel.org/doc/html/latest/dev-tools/ubsan.html

在没有打开 CONFIG_UBSAN=y 的情况下， 可以给一个模块打开吗?

跑的测试一下再说吧，感觉不是很靠谱的样子啊 这就是全部的东西 lib/ubsan.c ?

而且 ub 不是静态检查的时候就可以发现的吗?

## 效果
```txt
[ 6968.674846] ------------[ cut here ]------------
[ 6968.675334] UBSAN: array-index-out-of-bounds in arch/aarch64/sysreg.c:63:3
[ 6968.676013] index 2 is out of range for type 'int [2]'
[ 6968.676042] CPU: 1 UID: 0 PID: 2633 Comm: tee Tainted: G           O        6.16.0 #40 PREEMPT(full)
[ 6968.676047] Tainted: [O]=OOT_MODULE
[ 6968.676049] Hardware name: QEMU KVM Virtual Machine, BIOS 0.0.0 02/06/2015
[ 6968.676056] Call trace:
[ 6968.676059]  show_stack+0x34/0x98 (C)
[ 6968.676071]  dump_stack_lvl+0x7c/0xb0
[ 6968.676076]  dump_stack+0x18/0x24
[ 6968.676079]  ubsan_epilogue+0x10/0x48
[ 6968.676084]  __ubsan_handle_out_of_bounds+0xa0/0xd0
[ 6968.676092]  test_sysreg+0x268/0x280 [martins3]
[ 6968.676104]  sysreg_store+0xd0/0x120 [martins3]
[ 6968.676110]  kobj_attr_store+0x18/0x30
[ 6968.676116]  sysfs_kf_write+0x58/0x90
[ 6968.676121]  kernfs_fop_write_iter+0x134/0x208
[ 6968.676124]  vfs_write+0x224/0x3b0
[ 6968.676130]  ksys_write+0x78/0x120
[ 6968.676133]  __arm64_sys_write+0x24/0x40
[ 6968.676135]  invoke_syscall.constprop.0+0x58/0xf0
[ 6968.676138]  do_el0_svc+0x48/0xf0
[ 6968.676141]  el0_svc+0x5c/0x240
[ 6968.676147]  el0t_64_sync_handler+0x10c/0x138
[ 6968.676150]  el0t_64_sync+0x198/0x1a0
[ 6968.676154] ---[ end trace ]---
[ 6968.676380] 1b0020 13c039
```

## ubsan 的原理是什么?

先需要看
https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html

https://maskray.me/blog/2023-01-29-all-about-undefined-behavior-sanitizer

然后看 lib/ubsan.h 中，估计就是当添加的代码检查出来错误，那么会直接跳转到对应的位置上
然后报错。

## 类似的 san
- https://github.com/junwha/awesome-sanitizer
- https://github.com/realtime-sanitizer/rtsan

## 替换内核启动，有时候可以注意到
```txt
fuse: Unknown symbol __ubsan_handle_out_of_bounds (err -2)
fuse: Unknown symbol __ubsan_handle_load_invalid_value (err -2)
fuse: Unknown symbol __ubsan_handle_out_of_bounds (err -2)
fuse: Unknown symbol __ubsan_handle_load_invalid_value (err -2)
```

## 内核如何实现 UBSAN

一句话概括：**UBSAN 的检查逻辑由编译器插入，Linux 内核提供配置、编译参数和错误处理 runtime。** 它并不是 `lib/ubsan.c` 主动扫描内核代码。

### 1. Kbuild 选择要启用的检查

入口是 `CONFIG_UBSAN` 及其子选项，定义在 `lib/Kconfig.ubsan`，例如：

- `CONFIG_UBSAN_BOUNDS`：数组下标越界；
- `CONFIG_UBSAN_SHIFT`：非法移位；
- `CONFIG_UBSAN_BOOL`、`CONFIG_UBSAN_ENUM`：装载非法值；
- `CONFIG_UBSAN_ALIGNMENT`：未对齐访问；
- `CONFIG_UBSAN_INTEGER_WRAP`：整数溢出，目前仍是实验选项。

顶层 `Makefile` 在 `CONFIG_UBSAN=y` 时包含 `scripts/Makefile.ubsan`。后者将这些配置翻译成编译器参数，例如：

```make
-fsanitize=array-bounds
-fsanitize=shift
-fsanitize=bool
-fsanitize=enum
```

`scripts/Makefile.lib` 再把 `CFLAGS_UBSAN` 加到具体目标文件。目录或单个文件可以用下面的 Kbuild 变量关闭或开启插桩：

```make
UBSAN_SANITIZE := n
UBSAN_SANITIZE_foo.o := n
UBSAN_SANITIZE_foo.o := y
```

UBSAN runtime 自己必须避免再次触发 UBSAN，所以 `lib/Makefile` 中有：

```make
UBSAN_SANITIZE_ubsan.o := n
```

### 2. 编译器插入检查和元数据

假设源码中有：

```c
int a[2];

return a[index];
```

开启数组边界检查后，编译器生成的逻辑可以近似理解为：

```c
if ((unsigned long)index >= 2)
	__ubsan_handle_out_of_bounds(&metadata,
				     (void *)(unsigned long)index);

return a[index];
```

`metadata` 也是编译器生成的静态数据，包含源码文件、行列号、数组类型和下标类型等信息。`lib/ubsan.h` 中的 `struct source_location`、`struct type_descriptor`、`struct out_of_bounds_data` 等结构，就是内核对编译器 UBSAN handler ABI 的实现。

因此示例调用栈中的路径是：

```txt
test_sysreg()
  -> 编译器插入的边界检查失败
  -> __ubsan_handle_out_of_bounds()
  -> ubsan_prologue()
  -> dump_stack()
```

检查点是编译器生成的；`lib/ubsan.c` 只负责接收编译器传来的元数据和值，并把它们转换成可读的内核日志。

### 3. `lib/ubsan.c` 如何报告错误

普通的非 trap 模式下，`lib/ubsan.c` 实现并导出了多个编译器约定的 handler：

```c
__ubsan_handle_out_of_bounds()
__ubsan_handle_shift_out_of_bounds()
__ubsan_handle_load_invalid_value()
__ubsan_handle_type_mismatch_v1()
```

每个 handler 大致执行以下步骤：

1. 根据编译器传入的类型描述解析出值、符号和位宽；
2. 通过 `ubsan_prologue()` 输出错误类型、文件名和行列号；
3. 输出该类错误的细节，例如越界下标和数组类型；
4. 通过 `ubsan_epilogue()` 打印调用栈，并执行 `panic_on_warn` 策略。

内核还做了两层抑制：

- `source_location.reported` 中借用一位记录该检查点是否已经报告，避免同一位置反复刷屏；
- `task_struct.in_ubsan` 防止报告错误的代码本身再次进入 UBSAN，形成递归。

默认的 recover 模式通常在报告后返回，程序继续执行，所以 UBSAN 主要是**发现和定位问题**，并不保证阻止后续越界访问。`panic_on_warn` 可以让报告升级为 panic；`__builtin_unreachable()` 对应的 handler 则不能安全返回，会直接 panic。

### 4. `CONFIG_UBSAN_TRAP` 是另一条路径

打开 `CONFIG_UBSAN_TRAP=y` 后，Kbuild 增加：

```make
-fsanitize-trap=undefined
```

此时编译器不再调用详细的 `__ubsan_handle_*()`，而是在检查失败处生成 trap 指令。x86 使用 UBSAN 类型的 `UD1`，arm64 使用带立即数的 `BRK`；立即数编码了哪一类 sanitizer 检查失败。

异常进入架构 trap handler 后：

- x86 在 `arch/x86/kernel/traps.c` 中识别 `BUG_UD1_UBSAN`；
- arm64 在 `arch/arm64/kernel/traps.c` 中由 `ubsan_brk_handler()` 处理；
- `lib/ubsan.c:report_ubsan_failure()` 把检查编号转换成简短的错误名称。

这种模式不需要携带完整的文件、类型和值等报告路径，内核体积更小，但会直接 Oops，日志也没有普通模式详细。

### 5. 为什么编译期不能发现所有 UB

如果操作数是常量，编译器或静态分析工具确实可能在编译期直接告警。但内核中的下标、移位量、指针和值往往来自设备、用户输入、并发状态或其他运行时路径，编译器无法在编译期确定它们。

UBSAN 所谓“compile-time instrumentation”的含义不是“在编译期找完所有错误”，而是**在编译期插入检查，在运行期用真实值判断**。因此它只能发现实际执行到的已插桩路径。

### 6. 能否只给模块打开 UBSAN

使用内核原生的 handler 模式时，不能在运行内核没有启用 `CONFIG_UBSAN` 的情况下，只给一个模块可靠地打开 UBSAN：

- `CONFIG_UBSAN=n` 时，`scripts/Makefile.lib` 不会自动给模块添加 UBSAN 参数；
- 即使手工给模块添加 `-fsanitize=...`，编译器仍会生成对 `__ubsan_handle_*()` 的引用；
- `lib/ubsan.o` 只在 `CONFIG_UBSAN=y` 时链接进内核。handler 虽然有 `EXPORT_SYMBOL()`，但没有被构建时仍然不存在，于是模块加载就会出现前面的 `Unknown symbol __ubsan_handle_*`。

正确做法是先让运行内核以 `CONFIG_UBSAN=y` 构建，再通过 `UBSAN_SANITIZE_foo.o := y` 选择模块目标，并保证模块是针对这个内核的配置和构建产物编译的。可以用 `CONFIG_TEST_UBSAN=m` 构建 `lib/test_ubsan.c`，验证整条插桩和报告路径。

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
