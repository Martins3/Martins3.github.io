# qemu 封装
## 通用虚拟机管理

- [collei](../../collei/)：本仓库自己的 QEMU 启动、调试和迁移包装。
- [libvirt](https://libvirt.org/drvqemu.html)：QEMU 最常见的管理层；图形前端可以使用 [virt-manager](https://virt-manager.org/)。
- [Lima](https://github.com/lima-vm/lima)：在 macOS 和 Linux 上创建 Linux 虚拟机，Linux host 上可以使用 QEMU/KVM。仓库中记录过它生成的完整 QEMU 命令行，见 [basic.md 的 lima 章节](./basic.md#lima)。
- [Multipass](https://github.com/canonical/multipass)：用于快速创建 Ubuntu 虚拟机；仓库中记录的 Linux 实例由 QEMU/KVM 启动，见 [collei/todo.md 的“吸收一下 multipass 的”章节](../collei/todo.md#吸收一下-multipass-的)。
- [Packer](https://github.com/hashicorp/packer)：通过 QEMU builder 自动构建虚拟机镜像。仓库中还记录了 [packer-kvm](https://github.com/goffinet/packer-kvm) 和 [virt-scripts](https://github.com/goffinet/virt-scripts)。
- [Proxmox qemu-server](https://github.com/proxmox/qemu-server)：Proxmox VE 的 QEMU/KVM 管理层；仓库中保存了它生成的 QEMU 参数，见 [proxmox.md](../kernel/tutorial/proxmox.md)。
- [kvmrun](https://github.com/0xef53/kvmrun)：轻量的 QEMU/KVM 虚拟机管理工具。

- https://rkiselenko.dev/blog/development-on-mac-with-utm/development-on-mac-with-lima/
- https://blog.getutm.app/2026/introducing-triton-directx-11-driver-for-qemu/
utm 感觉还是有技术含量的

## windows 相关
- https://github.com/quickemu-project/quickemu
- https://github.com/winapps-org/winapps
- https://github.com/dockur/windows
    - https://github.com/TibixDev/winboat

## 内核开发和自动化测试

- [virtme-ng](https://github.com/arighi/virtme-ng)：直接用当前文件系统启动待测试内核，避免制作完整 rootfs；仓库中还分析过它的 initramfs 和 virtiofs 用法。
- [danobi/vmtest](https://github.com/danobi/vmtest)：面向内核开发的 QEMU 包装，仓库中有单独的 [vmtest 集成记录](../collei/vmtest.md)。
- [hugelgupf/vmtest](https://github.com/hugelgupf/vmtest)：Rust 的 QEMU 测试虚拟机库；仓库原有备注认为它已经不再更新。
- [syzkaller 的 QEMU backend](https://github.com/google/syzkaller/blob/master/vm/qemu/qemu.go)：由 syzkaller 自动创建、启动和回收 QEMU 虚拟机。
- [xfstests-bld](https://github.com/tytso/xfstests-bld)：为 xfstests 构建并管理 QEMU 测试环境。
- [kdevops](https://github.com/linux-kdevops/kdevops)：可用 QEMU/libvirt 自动部署内核开发和测试环境。
- [mptcp-upstream-virtme-docker](https://github.com/multipath-tcp/mptcp-upstream-virtme-docker)：把 virtme 内核测试环境进一步封装进容器。

## 特定系统和容器化场景

- [OSX-KVM](https://github.com/kholia/OSX-KVM)：用 QEMU/KVM 运行 macOS。
- [Docker-OSX](https://github.com/sickcodes/Docker-OSX)：在容器中封装 OSX-KVM 的安装和运行流程。
- [OSX-PROXMOX](https://github.com/luchina-gabriel/OSX-PROXMOX)：在 Proxmox VE 上自动准备 macOS 虚拟机。
- [dockerpi](https://github.com/lukechilds/dockerpi)：在容器中调用 QEMU 模拟 Raspberry Pi。
- [qemu-user-static](https://github.com/multiarch/qemu-user-static)：把 QEMU user-mode 和 binfmt_misc 注册流程容器化，用于运行跨架构容器。
- [qemus/qemu](https://github.com/qemus/qemu)：容器化的 QEMU 环境。
- https://github.com/qemus/qemu-docker

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
