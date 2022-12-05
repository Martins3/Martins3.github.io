# kvm

## 问题
- [ ] 关于 vcpu 和 CPU 的关系:
  - [ ] 一个虚拟机可能创建多个 vcpu 出来，如何保证这个虚拟机的 thread 可以在多个 vcpu 之间切换
  - [ ] 不同虚拟机的 vcpu 如何实现隔离
- [ ] 切换 CPU 的时候切换 TLB
- [ ] vcpu 在不同的 CPU 的切换
- [ ] 为什么 kernel 中存在那么多代码来处理 lapic 的虚拟化，比 ioapic 多多了

## 项目
- https://github.com/Fmstrat/winapps/issues/269 : 利用 kvm 直接拉起来 windows 应用

## virtualization
- [LC-3 虚拟机](https://justinmeiners.github.io/) : 只有几百行
- [dockerpi](https://github.com/lukechilds/dockerpi) : 其实是一百行左右的 Dockerfile，在其中运行 qemu 模拟 raspberrypi 的硬件环境法
- [OSX-KVM](https://github.com/kholia/OSX-KVM) : 利用 kvm 实现运行 OSX 的虚拟机
- [Docker-OSX](https://github.com/sickcodes/Docker-OSX) : 类似 dockerpi, 提供安装 [OSX-KVM](https://github.com/kholia/OSX-KVM) 的自动安装
- [macos virtualbox](https://github.com/myspaghetti/macos-virtualbox) : 提供一个脚本，在 virtualbox 中间运行 macos
- [v86](https://github.com/copy/v86/) : 使用 js 写的 x86 硬件虚拟化，可以在网页上运行机器 [windows95 in electron](https://github.com/felixrieseberg/windows95) : 利用 v86 实现运行 windows95 在 electron 中间

## 项目
- [ ] https://github.com/intel/haxm : 对于 Windows 和 Mac 存在良好的支持，但是 Linux 上根本无法编译的，感觉类似于 KVM 的东西
  - https://www.qemu.org/2017/11/22/haxm-usage-windows/ : 在 qemu 上利用这个东西实现加速

https://github.com/steren/awesome-cloudrun : Google 的产品介绍，不知道能不能白嫖

https://github.com/rootsongjc/awesome-cloud-native#api-gateway
> 完全不知道云原生在干什么 !

dockerpi : docker 作为一个容器，为什么可以实现跨架构, 因为里面还安装了一个 qemu !
  - https://github.com/dhruvvyas90/qemu-rpi-kernel
https://www.vagrantup.com/intro/getting-started/up : 基于虚拟化的环境开发，那么，所以和 docker 的关系是什么 ?
https://github.com/weaveworks/footloose : 让 docker 类似虚拟机，看来虚拟机和 containers 的区别不仅仅如此啊
https://github.com/kholia/OSX-KVM

- 各种轻量级虚拟化
3. Chrome OS Virtual Machine Monitor : https://news.ycombinator.com/item?id=15346269
5. kata

https://github.com/fireeye/speakeasy : Speakeasy is a portable, modular, binary emulator designed to emulate Windows kernel and user mode malware.
https://github.com/Kelvinhack/kHypervisor : window 的 ept hypervisor

https://github.com/Friz-zy/awesome-linux-containers

- [rust-vmm](https://github.com/rust-vmm/kvm-bindings)
- https://github.com/canonical/multipass : cpp 的项目，可以编排 ubuntu 虚拟机

## 文摘
[ali 云计算 的自我推广](https://mp.weixin.qq.com/s/5WKDZfzIQE3QB-Io1lmG-w)

> 对于 service mesh，我们采用基于 eBPF 的 sockmap 进行加速。sockmap 允许将 TCP 连接之间的数据转发过程卸载到内核中，从而绕过复杂的 Linux 网络协议栈直接在内核完成 socket 之间的数据转发操作，减少了上下文切换以及用户态和内核态之间的数据拷贝操作，优化 TCP 连接之间 socket 数据转发的性能。

> 首先对于 9pfs 作为安全容器 rootfs 的问题，我们建议采用 virtio-fs 替换 9pfs，

https://github.com/alibaba/inclavare-containers : alibaba 果然感受到别人对于自己的不信任

## TODO
- [ ] https://github.com/Martins3/Martins3.github.io/issues/22

- [ ] [An Introduction to Clear Containers](https://lwn.net/Articles/644675/)
  - https://github.com/clearcontainers 已经死掉了
