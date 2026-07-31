# codex 的调研结果

当前这个源码树（v11.1.0-rc1-48-g05e27e70df42）里，QEMU 对 Rust 的定位可以概括为：

> Rust 已正式接入 QEMU 构建系统，并用于实现真实设备模型，但仍是默认关闭、范围受限的渐进式引入。

### 目前实际用在哪里

当前有两个完整的 Rust 设备模型：

- PL011 ARM UART
- HPET x86 高精度定时器

它们不是额外的测试设备，而是对应 C 实现的功能替代品，见 docs/devel/rust.rst:103。

启用 Rust 后，Kconfig 会直接选择 Rust 实现：

- PL011：选择 X_PL011_RUST，不再编译 pl011.c，见 hw/char/Kconfig:12
- HPET：选择 X_HPET_RUST，不再编译 hpet.c，见 hw/timer/Kconfig:12

因此这是编译期替换，不是运行时选择：

--disable-rust（默认） → PL011/HPET 使用 C
--enable-rust         → PL011/HPET 使用 Rust

### Rust 代码的整体结构

大致可以分成三层：

Rust 设备模型
  pl011、hpet
       ↓
安全/高层封装
  qom、hwcore、system、migration、bql、chardev、util、trace
       ↓
bindgen 生成的原始 FFI
  qom-sys、system-sys、hwcore-sys、migration-sys、util-sys...
       ↓
QEMU 原有 C API

高层 crate 已经覆盖了编写 SysBus 设备所需的大部分基础设施：

- QOM 对象、继承与类型注册
- SysBusDevice、DeviceState
- MMIO MemoryRegion
- IRQ
- VMState 和迁移
- BQL 保护下的可变状态
- timer、错误处理、日志
- QEMU tracepoint
- #[derive(qom::Object)]、#[derive(hwcore::Device)] 等过程宏

当前主要目标仍是让 SysBusDevice 能以相对安全的 Rust 编写；PCI/DMA 设备、完整 machine、block backend 尚不在成熟支持范围内，见 docs/devel/rust.rst:6。

### 构建方式

Rust 不是由 Cargo 主导构建的，而是：

1. Meson 调用 bindgen 生成 C API bindings。
2. Meson 直接调用 rustc，将各 crate 编译成静态库。
3. Rust 静态库和 QEMU 的 C 对象一起链接进 qemu-system-*。

相关入口在 meson.build:4368 和 rust/meson.build:1。

Cargo 主要用于开发工具：

make rustfmt
make clippy
make rustdoc
make check-rust

普通 cargo build/test 不能独立完成完整构建，因为 Rust crate 依赖 Meson 生成的 bindings、trace 文件以及 QEMU C 对象，见 docs/devel/rust.rst:24。

### 默认状态和工具要求

Rust 支持默认关闭：

option('rust', type: 'feature', value: 'disabled')

见 meson_options.txt:384。

启用方式：

../configure --enable-rust
make

要求：

- Rust edition 2021
- rustc >= 1.83.0
- bindgen >= 0.60.0
- host 和 target 两侧都需要 Rust compiler
- 只用于 qemu-system-*，不用于 linux-user/bsd-user

版本检查见 meson.build:95，workspace 配置见 rust/Cargo.toml:1。

### 成熟度判断

目前不能说 QEMU 正在全面 Rust 化。更准确的说法是：

- 构建、FFI、QOM、迁移、MMIO、IRQ、测试和 tracing 的基本框架已经成形。
- PL011 和 HPET 已经证明能够用 Rust 实现真实、可迁移的 QEMU 设备。
- Rust 仍默认关闭，只服务于 system emulator。
- 当前主要适合新增或移植简单的 SysBus/MMIO 设备。
- PCI、DMA、block、TCG 等核心领域暂时仍是 C 的天下。
- 整个 rust/ 当前约 1.17 万行 .rs，规模仍远小于 QEMU 的 C 代码。

另外，文档第 123 行称两个设备“缺少 tracing”，但当前源码中 PL011 和 HPET 都已经调用生成的 tracepoint，因此这一条文档看起来已经落后于实现。总体上，这仍是一个发展较
快、接口尚在继续完善的子系统。



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
