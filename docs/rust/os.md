# Rust 实现 Hobby OS

一个初步的关注思路：标注为 “are suitable for use in `no_std` environments” 的
crate，通常值得在 Hobby OS 开发中关注。

## Rust OS 生态入口

- [rust-osdev](https://github.com/rust-osdev)：Rust OS 开发生态的相关项目集合。
- [rust-osdev/bootloader](https://github.com/rust-osdev/bootloader)：用于 Rust Hobby OS 的引导加载器，真的值得一试。

## OS 项目与框架

### 通用类 OS 框架

- [Theseus](https://github.com/theseus-os/Theseus)：用 Rust 编写的实验型操作系统，强调语言级安全与单地址空间设计。
- Redox：用 Rust 编写的类 Unix 微内核操作系统。
  - [官方网站](https://www.redox-os.org/)
  - [GitLab 源码仓库](https://gitlab.redox-os.org/redox-os)
  - [GitHub 镜像](https://github.com/redox-os/redox)
- Tock：面向嵌入式/IoT 的 Rust 操作系统，采用组件化设计。这个嵌入式 OS 很好。
  - [源码仓库](https://github.com/tock/tock)
- [Ariel OS](https://github.com/ariel-os/ariel-os)

## OS 开发组件与辅助 crate

- [bootloader](https://github.com/rust-osdev/bootloader)：编写 Rust OS 的引导加载器。
- [`x86_64`](https://github.com/rust-osdev/x86_64)：x86_64 架构下寄存器、中断、分页等底层抽象。
- [`uefi-rs`](https://github.com/rust-osdev/uefi-rs)：UEFI 引导与服务接口的 Rust 绑定。
- [`linked_list_allocator`](https://github.com/rust-osdev/linked-list-allocator) / [`buddy_system_allocator`](https://github.com/rcore-os/buddy_system_allocator)：OS 内简单的堆分配器。
- [`spin`](https://github.com/zesterer/spin-rs)：自旋锁实现，可用于 `no_std` 环境。
- [`smoltcp`](https://github.com/smoltcp-rs/smoltcp)：值得关注的 `no_std` crate。
- [`volatile`](https://github.com/rust-osdev/volatile)：MMIO 易失性读写抽象。
- [`pic8259`](https://github.com/rust-osdev/pic8259) / [`x2apic`](https://github.com/kwzhao/x2apic-rs)：中断控制器相关库。
- [rcore-os/virtio-drivers](https://github.com/rcore-os/virtio-drivers)：chengyu 老师的项目，看起来也很类似。

## 组件化方向的选择

如果想找像 [`rust-vmm`](https://github.com/rust-vmm) 一样“偏组件化、可拼装”的
Rust OS 基础设施，
- [Theseus](https://github.com/theseus-os/Theseus)，以及
- [`bootloader`](https://github.com/rust-osdev/bootloader) +
- [`x86_64`](https://github.com/rust-osdev/x86_64) +
- [`uefi-rs`](https://github.com/rust-osdev/uefi-rs) 这套组合最接近。

## 如果是实现 Hypervisor

那么就是 [`rust-vmm`](https://github.com/rust-vmm) 了。

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
