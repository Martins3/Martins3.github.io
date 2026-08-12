# Hyperlight

- 项目：https://github.com/hyperlight-dev/hyperlight
- 介绍：https://opensource.microsoft.com/blog/2024/11/07/introducing-hyperlight-virtual-machine-based-security-for-functions-at-scale/
- Hacker News 讨论：https://news.ycombinator.com/item?id=42078476
- Wasm runtime：https://github.com/hyperlight-dev/hyperlight-wasm

Hyperlight 是一个可以嵌入 Rust 程序的轻量 VMM。它不是启动一个完整 Linux guest，
而是通过 KVM/WHP/MSHV 运行一个没有内核和 OS 的 `no_std` guest ELF，再通过 host/guest
函数调用传递参数和返回值。

### 结论

2026-08-10 在下面的环境中验证，Hyperlight 本身可以跑通：

- Hyperlight commit：`573c2dc2bb3a71bee283d61f6ba373ebec2069ae`
- Fedora 44，Linux `7.1.3-201.fc44.x86_64`
- Rust `1.94.1`
- `cargo-hyperlight 0.1.12`
- `/dev/kvm` 对当前用户可读写

`just rg` 能成功构建并复制 debug/release 两套 Rust guest，随后
`cargo run --example hello-world` 能通过 KVM 启动 guest，并输出：

```text
Hello, World! I am executing inside of a VM :)
```

用 `strace -f -e trace=openat` 验证时，进程既打开了 guest ELF，也打开了 `/dev/kvm`：

```text
openat(..., ".../src/tests/rust_guests/bin/debug/simpleguest", O_RDONLY|O_CLOEXEC) = 3
openat(..., "/dev/kvm", O_RDWR|O_CLOEXEC) = 3
```

因此这不是只完成了编译，而是真正执行了 KVM guest。

### 可复现步骤

先确认编译工具和 KVM 可用：

```sh
clang --version
ls -l /dev/kvm
```

仓库根目录的 `rust-toolchain.toml` 已声明 Rust 版本和 guest target：

```toml
[toolchain]
channel = "1.94"
targets = ["x86_64-unknown-none", "x86_64-unknown-linux-musl"]
```

使用 rustup 时，进入仓库后会自动安装缺少的 toolchain/target，不必再手工执行
`rustup target add x86_64-unknown-none`。可以这样确认：

```sh
rustc --version
rustup target list --installed
```

本次输出为：

```text
x86_64-unknown-linux-gnu
x86_64-unknown-linux-musl
x86_64-unknown-none
```

本机没有全局安装 `just`，所以用临时 Nix 环境提供它：

```sh
nix-shell -p just --run 'just rg'
```

`just rg` 当前等价于 `build-and-move-rust-guests`，会：

1. 检查并按需安装仓库固定的 `cargo-hyperlight 0.1.12`；
2. 使用 `cargo hyperlight build --workspace` 构建 debug/release guest；
3. 把 `simpleguest`、`dummyguest`、`witguest` 复制到
   `src/tests/rust_guests/bin/{debug,release}/`。

最后运行一个最小 KVM smoke test：

```sh
cargo run --example hello-world
```

冷启动需要下载和编译 host workspace 依赖；编译缓存完成后，直接运行
`target/debug/examples/hello-world`，本机三次完整进程耗时为 `0.04`、`0.04`、`0.05`
秒，最大 RSS 约 21 MiB。这个数字包含 host 进程启动、装载和退出，只能说明本机的
smoke-test 体感，不能当作 Hyperlight micro-VM 创建或函数调用的严格 benchmark。

### Rust target 如何 Nix 化

`rustup target add` 是修改用户 toolchain 的命令式操作；Nix 中应当把 targets 声明成
Rust derivation 的组成部分。Hyperlight 当前 `flake.nix` 已采用这种做法：它为固定日期的
stable/nightly/MSRV toolchain 设置 `targets`，其中包括：

```nix
targets = [
  "x86_64-unknown-linux-gnu"
  "x86_64-pc-windows-msvc"
  "x86_64-unknown-none"
  "wasm32-wasip1"
  "wasm32-wasip2"
  "wasm32-unknown-unknown"
  "aarch64-unknown-none"
];
```

也就是说，Nix 方案不是在 `shellHook` 中调用下面这些命令：

```text
rustup target add x86_64-unknown-none
rustup target add x86_64-pc-windows-msvc
```

而是让 `nix develop` 得到的 Rust 工具链从构建时就包含对应的 `rust-std`。这样 target
版本随 `flake.lock` 和 Rust toolchain 一起固定，不会污染 `~/.rustup`。

需要注意，安装 `x86_64-pc-windows-msvc` 的 `rust-std` 只解决 Rust 标准库 target；
如果真要在 Linux 上链接 Windows MSVC 程序，仍可能需要匹配的 linker/Windows SDK。
当前 Linux guest 构建已经走 `cargo-hyperlight` 的 `x86_64-hyperlight-none` 自定义 target，
不需要再像 2024 年的旧 Justfile 那样构建和复制一份 Windows guest，也不需要注释
Justfile 中的 MSVC 命令。

### 当前 flake 的实测问题

本次执行下面的命令时，仓库当前完整 Nix dev shell **没有跑通**：

```sh
nix develop --no-update-lock-file
```

失败发生在 `cargo-hyperlight-0.1.5-vendor-staging` 下载
`clap_lex/1.0.0` 时，crates.io 返回 HTTP 403。除此之外，当前 `flake.nix` 固定的是
`cargo-hyperlight 0.1.5`，而 `Justfile` 已要求 `0.1.12`，存在明显的版本漂移。

所以本次验证结论要分开看：

- Rust target 的 Nix 声明方式是正确的，构建日志也确认生成了
  `x86_64-unknown-none` 和 `x86_64-pc-windows-msvc` 的 `rust-std`；
- 当前 commit 的整个 `nix develop` 环境仍需同步 `cargo-hyperlight` 版本并修复/绕过
  crates.io 403 后才能称为完整跑通；
- rustup toolchain + `nix-shell -p just` 的混合路径已经完成 guest 构建和真实 KVM
  执行验证。

### 作为 KVM 测试入口的效果

这个项目很适合做 KVM 的快速 smoke test：

- 产物是无 guest kernel/OS 的静态 PIE，启动链路比完整 VM 短；
- 测试确实经过 `/dev/kvm`，并覆盖 vCPU、guest memory、退出处理以及 host/guest 调用；
- 热运行是几十毫秒级的完整进程体验，适合快速确认 KVM 基本功能和做高频回归。

它不能替代完整 Linux VM 的内核启动、virtio 设备、迁移、NUMA 等测试，但非常适合验证
KVM ioctl/vCPU 基础路径，或者作为内核 KVM patch 的第一层快速回归入口。

## 可以允许调用 open() 吗?
默认不可以直接 open()。

Hyperlight guest 没有 Linux 内核、文件系统和 syscall，因此下面这种代码不能工作：

std::fs::File::open("/etc/config")

可通过两种受控方式访问文件内容：

1. Host function

由 host 打开文件，并把内容传给 guest：

guest: ReadConfig("app.toml")
             ↓
host: 校验文件名 → File::open → 读取内容
             ↓
guest: 收到 Vec<u8> 或 String

这样可以精确限制 guest 能读取哪些文件，避免把整个宿主机文件系统暴露进去。

2. map_file_cow

当前 Hyperlight 提供：

sandbox.map_file_cow(path, guest_address)?;

host 打开文件，并把内容映射到指定的 guest 内存地址。不过它：

- 不是 POSIX open()；
- 不提供文件描述符；
- 当前映射对 guest 是只读的；
- guest 通过内存地址读取内容；
- 不能用它直接修改宿主机文件。

因此可以概括为：

 需求                        Hyperlight core
━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 guest 直接 open("/path")    不支持
──────────────────────────  ────────────────────────────────────────────
 读取 host 允许的文件        用 host function
──────────────────────────  ────────────────────────────────────────────
 把大文件只读映射进 guest    用 map_file_cow
──────────────────────────  ────────────────────────────────────────────
 完整 POSIX 文件系统         不适合，考虑完整 VM 或 hyperlight-unikraft

这种限制正是 Hyperlight 隔离模型的一部分：guest 默认没有文件、网络等环境权限，host 必须显式授予每一种能力。

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
