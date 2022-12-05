## QA
3. 为什么不使用 unikernel + QEMU 的方法?
    - 现在 unikernel 的解决方案都是试图将 unikernel 放到虚拟机的 guest 态中运行，无法避免 host 的软件栈
      - 那为什么不采用的 A Linux in unikernel clothing [^2] 的方式 ?
          - 这种方法无法实现设备直通, 而且这种方案依赖于 KLM[^3] 的支持
4. 为什么不基于 captive 开发?
    - captive 自身的代码量非常大, 大约在 10 万行左右
    - captive 需要对于 x86 写对应的 asl 硬件描述语言从而生成自动翻译
    - captive 是通过构建一个 unikernel 在 kvm 中运行，其 unikernel 实现和硬件打交道的方法都是 virtio 。这和我们希望直接在虚拟上运行需求不符合。
5. 为什么不使用 Rust 开发?
    - 虽然在底层开发中间出现了类似 Theseus[^5] 以及 Bento[^4] 之类使用 Rust 开发趋势，Rust 可以带来内存安全以及其他的更多安全检查
    - 虽然存在 c2rust [^6] 等工具实现源码装换，目前来说，重写代码的难度不可预估，使用 Rust 来构建是将来的工作
    - 使用 Rust 写还有一个好处在于可以拜托 QEMU 的证书限制。

## 一些参考的 hypervisor
- https://github.com/quic/gunyah-hypervisor : 是构建了一个 micro kernel + kernel virtual machine 比较初级的项目
- https://github.com/projectacrn/acrn-hypervisor : 是用于嵌入式上的 type 1 hypervisor, 也是 kernel + hypervisor 的方式
- https://github.com/airbus-seclab/ramooflax : 这个项目目前是和我们的需求最为相似的，从 bios 启动，然后运行 hypervisor 然后将磁盘上的操作系统在 guest 中间运行, 其唯一的区别在于使用了 vt-x 的虚拟化技术
- https://github.com/udosteinberg/NOVA : NOVA 可以运行多个 unmodified 的 guest kernel
- https://github.com/matsud224/raspvisor : 在树莓派上运行的 type 1 hypervisor
- https://github.com/wbenny/hvpp/blob/master/README.md :  使用 Window 驱动, 用 C++ 写的一个 hypervisor, 可以理解为一个简单的 kvm
- https://github.com/tandasat/MiniVisorPkg : 使用 UEFI driver 来构建 hypervisor

[^1]: [Efficient Cross-architecture Hardware Virtualisation](https://era.ed.ac.uk/handle/1842/25377)
[^2]: [A Linux in unikernel clothing](https://dl.acm.org/doi/10.1145/3342195.3387526)
[^3]: [Kernel Mode Linux](http://web.yl.is.s.u-tokyo.ac.jp/~tosh/kml/)
[^4]: [High Velocity Kernel File Systems with Bento](https://www.usenix.org/conference/fast21/presentation/miller)
[^5]: [Theseus](https://github.com/theseus-os/Theseus)
[^6]: [c2rust](https://github.com/immunant/c2rust)
