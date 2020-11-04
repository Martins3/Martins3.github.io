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
https://github.com/alibaba/inclavare-containers : alibaba 果然感受到别人对于自己的不信任

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


# 看看 backend 到底在干什么 ?
https://github.com/elliotreborn/github-stars



[nodejs](https://en.wikipedia.org/wiki/Node.js) 是 js 的 runtime environment.
running scripts server-side to produce dynamic web page content before the page is sent to the user's web browser

## kubernetes
# [](https://github.com/ramitsurana/awesome-kubernetes)

> 这本书太难懂了，使用过多的高深词汇

> Borg : Large-scale cluster management at Google with Borg

云原⽣系统须具备下⾯这些属性：
1. 应⽤容器化：将软件容器中的应⽤程序和进程作为独⽴的应⽤程序部署单元运⾏，并作为实现⾼级别资源隔离的机 制。从总体上改进开发者的体验、促进代码和组件重⽤，⽽且要为云原⽣应⽤简化运维⼯作。
2. 动态管理：由中⼼化的编排来进⾏活跃的调度和频繁的管理，从根本上提⾼机器效率和资源利⽤率，同时降低与运 维相关的成本。
3. ⾯向微服务：与显式描述的依赖性松散耦合（例如通过服务端点），可以提⾼应⽤程序的整体敏捷性和可维护性。 CNCF 将塑造技术的发展，推动应⽤管理的先进技术发展，并通过可靠的接⼝使技术⽆处不在，并且易于使⽤。

# 资源
https://github.com/ramitsurana/awesome-kubernetes

https://github.com/kubernetes/examples/tree/master/guestbook : 四个例子
https://github.com/lensapp/lens : 图形界面

# So, how it works !

# 利用 kind 赶快上手吧 !
1. 忽然发现，go lang 还是赶快安排吧


## 入门
http://www.dockone.io/article/932
