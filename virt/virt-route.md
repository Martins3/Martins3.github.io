## Plans
1. 看一下licun 的文章
2. [^1]
3. kvmsample
4. byte : virtio
5. libvirt 是什么 ? https://github.com/libvirt/libvirt


## qemu boches
https://news.ycombinator.com/item?id=22826296 : boches 的原理
https://github.com/aind-containers/aind

## 小项目
https://github.com/wbenny/hvpp 但是指向了一堆项目

https://github.com/steren/awesome-cloudrun : Google 的产品介绍，不知道能不能白嫖

https://github.com/rootsongjc/awesome-cloud-native#api-gateway
> 完全不知道云原生在干什么 !


dockerpi : docker 作为一个容器，为什么可以实现跨架构, 因为里面还安装了一个 qemu !


## 不知道干什么的东西
https://www.vagrantup.com/intro/getting-started/up : 基于虚拟化的环境开发，那么，所以和 docker 的关系是什么 ?
https://github.com/weaveworks/footloose : 让 docker 类似虚拟机，看来虚拟机和 containers 的区别不仅仅如此啊

## KVM


## 文摘
[ali 云计算 的自我推广](https://mp.weixin.qq.com/s/5WKDZfzIQE3QB-Io1lmG-w)

> 对于service mesh，我们采用基于eBPF的sockmap进行加速。sockmap允许将TCP连接之间的数据转发过程卸载到内核中，从而绕过复杂的Linux网络协议栈直接在内核完成socket之间的数据转发操作，减少了上下文切换以及用户态和内核态之间的数据拷贝操作，优化TCP连接之间socket数据转发的性能。

> 首先对于9pfs作为安全容器rootfs的问题，我们建议采用virtio-fs替换9pfs，

https://virtio-fs.gitlab.io/ : /home/maritns3/core/54-linux/fs/fuse/virtio_fs.c : 只有 1000 多行，9pfs 也很小，这些东西有什么特殊的地方吗 ?

## IOS
我记得是存在好几个程序的:
https://github.com/kholia/OSX-KVM


## qemu
1. qemu 模拟 syscall 的策略是什么 ?


## 内核技术总结

## 轻量级虚拟化
1. https://github.com/cloud-hypervisor/cloud-hypervisor
2. rust vmm : https://github.com/rust-vmm
3. https://news.ycombinator.com/item?id=15346269
4. firecracker
5. kata

## TODO
1. eBPF 是不是可以用来写内核模块 ? 我想是不能，插入的程序什么时候时候触发和执行呀 ?
2. paravirtualization 是啥 ?
3. GPU 存在虚拟化吗 ?

4. https://github.com/LordNoteworthy/cpu-internals#chapter-6-interrupt-and-exception-handling
仔细研读一下，然后分析 intel 的 kvm 的实现

https://www.ibm.com/developerworks/cn/linux/l-cn-kvm-mem/index.html

5. https:/news.ycombinator.com/item?id=24166572/ : write you own vm

https://github.com/fireeye/speakeasy

[^1] : https://www.cnblogs.com/arnoldlu/category/1118000.html

