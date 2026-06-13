# Rust os 实现

似乎那些 are suitable for use in no_std environments 的 crate 就是可以关注的

- https://github.com/zesterer/spin-rs
- https://github.com/smoltcp-rs/smoltcp

- https://www.redox-os.org/
  - https://gitlab.redox-os.org/redox-os

哦，原来就是这个项目:
https://github.com/rust-osdev
- https://github.com/rust-osdev/bootloader : 真的值得一试

嵌入式 os ，很好
https://github.com/tock/tock

chengyu 老师的，似乎也很类似啊

https://github.com/rcore-os/virtio-drivers

https://github.com/ariel-os/ariel-os

## rust vmm 类似，用于写操作系统的类似的库叫做什么?
```txt
 ● 用于在 Rust 中写操作系统的库/框架主要有：

   通用类 OS 框架
   • Theseus — 用 Rust 编写的实验型操作系统，强调语言级安全与单地址空间设计。
   • Redox — 用 Rust 编写的类 Unix 微内核操作系统。
   • Tock — 面向嵌入式/IoT 的 Rust 操作系统，组件化设计。

   OS 开发辅助 crate
   • bootloader — 编写 Rust OS 的引导加载器。
   • x86_64 — x86_64 架构下寄存器、中断、分页等底层抽象。
   • uefi-rs — UEFI 引导与服务接口的 Rust 绑定。
   • linked_list_allocator / buddy_system_allocator — OS 内简单的堆分配器。
   • spin — 自旋锁实现（no_std 环境可用）。
   • volatile — MMIO 易失性读写抽象。
   • pic8259 / x2apic — 中断控制器相关库。

   如果是想找像 rust-vmm 一样“偏组件化、可拼装”的 Rust OS 基础设施，Theseus 和 bootloader + x86_64 + uefi-rs 这套组合最接近。
```

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
