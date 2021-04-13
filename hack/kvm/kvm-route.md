## 问题
- [ ] 关于 vcpu 和 CPU 的关系:
  - [ ] 一个虚拟机可能创建多个 vcpu 出来，如何保证这个虚拟机的 thread 可以在多个 vcpu 之间切换
  - [ ] 不同虚拟机的 vcpu 如何实现隔离

## 项目
- [ ] https://github.com/openvswitch/ovs : open vSwitch 和 virtio 是什么关系？
- [ ] https://github.com/intel/haxm : 对于 Windows 和 Mac 存在良好的支持，但是 Linux 上根本无法编译的，感觉类似于 KVM 的东西
  - https://www.qemu.org/2017/11/22/haxm-usage-windows/ : 在 qemu 上利用这个东西实现加速

https://github.com/wbenny/hvpp 但是指向了一堆项目
https://github.com/Fmstrat/winapps

https://github.com/steren/awesome-cloudrun : Google 的产品介绍，不知道能不能白嫖

https://github.com/rootsongjc/awesome-cloud-native#api-gateway
> 完全不知道云原生在干什么 !

dockerpi : docker 作为一个容器，为什么可以实现跨架构, 因为里面还安装了一个 qemu !
  - https://github.com/dhruvvyas90/qemu-rpi-kernel
https://www.vagrantup.com/intro/getting-started/up : 基于虚拟化的环境开发，那么，所以和 docker 的关系是什么 ?
https://github.com/weaveworks/footloose : 让 docker 类似虚拟机，看来虚拟机和 containers 的区别不仅仅如此啊
https://github.com/kholia/OSX-KVM

- 各种轻量级虚拟化
1. https://github.com/cloud-hypervisor/cloud-hypervisor
3. Chrome OS Virtual Machine Monitor : https://news.ycombinator.com/item?id=15346269
4. firecracker
5. kata
https://www.ibm.com/developerworks/cn/linux/l-cn-kvm-mem/index.html

https://github.com/fireeye/speakeasy : Speakeasy is a portable, modular, binary emulator designed to emulate Windows kernel and user mode malware.
https://github.com/Kelvinhack/kHypervisor : window 的 ept hypervisor

[nodejs](https://en.wikipedia.org/wiki/Node.js) 是 js 的 runtime environment. running scripts server-side to produce dynamic web page content before the page is sent to the user's web browser

kvm 虚拟化相关教学:
- https://luohao-brian.gitbooks.io/interrupt-virtualization/content/qemuzhi-network-device-quan-xu-ni-fang-an-4e8c3a-xu-ni-wang-qia.html
  - kernelgo.org
- https://rayanfam.com/topics/hypervisor-from-scratch-part-4/ 
- https://github.com/changeofpace/VivienneVMM


## kubernetes
https://github.com/rootsongjc/kubernetes-handbook
  - 这本书太难懂了，使用过多的高深词汇, 比如 Borg : Large-scale cluster management at Google with Borg
> 云原⽣系统须具备下⾯这些属性：
> 1. 应⽤容器化：将软件容器中的应⽤程序和进程作为独⽴的应⽤程序部署单元运⾏，并作为实现⾼级别资源隔离的机 制。从总体上改进开发者的体验、促进代码和组件重⽤，⽽且要为云原⽣应⽤简化运维⼯作。
> 2. 动态管理：由中⼼化的编排来进⾏活跃的调度和频繁的管理，从根本上提⾼机器效率和资源利⽤率，同时降低与运 维相关的成本。
> 3. ⾯向微服务：与显式描述的依赖性松散耦合（例如通过服务端点），可以提⾼应⽤程序的整体敏捷性和可维护性。 CNCF 将塑造技术的发展，推动应⽤管理的先进技术发展，并通过可靠的接⼝使技术⽆处不在，并且易于使⽤。

- http://www.dockone.io/article/932
- https://github.com/ramitsurana/awesome-kubernetes
- https://github.com/kubernetes/examples/tree/master/guestbook : 四个例子
- https://github.com/lensapp/lens : 图形界面
- https://github.com/aind-containers/aind
- https://github.com/TNK-Studio/lazykube
- https://github.com/AliyunContainerService/minikube : 在自己电脑上装 kubernate, 所以和 kind 的关系是什么 ?

- https://github.com/ysicing/kubernetes-handbook
- https://github.com/ramitsurana/awesome-kubernetes
- https://blog.quickbird.uk/domesticating-kubernetes-d49c178ebc41

- 上手尝试才是最好的学习方法
  - [几种快速上手的方式](https://mp.weixin.qq.com/s/vsicQ6Qn2YDzuD4q2EUK7w)

## 文摘
[ali 云计算 的自我推广](https://mp.weixin.qq.com/s/5WKDZfzIQE3QB-Io1lmG-w)

> 对于service mesh，我们采用基于eBPF的sockmap进行加速。sockmap允许将TCP连接之间的数据转发过程卸载到内核中，从而绕过复杂的Linux网络协议栈直接在内核完成socket之间的数据转发操作，减少了上下文切换以及用户态和内核态之间的数据拷贝操作，优化TCP连接之间socket数据转发的性能。

> 首先对于9pfs作为安全容器rootfs的问题，我们建议采用virtio-fs替换9pfs，

https://github.com/alibaba/inclavare-containers : alibaba 果然感受到别人对于自己的不信任
