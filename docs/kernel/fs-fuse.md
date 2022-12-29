# virtio fs

QEMU 中共享 Guest 和 Host 的目录以前可以使用 [virtio-9p](https://www.linux-kvm.org/page/9p_virtio)，最近支持了 virtio fs ，在这里对于
virtio fs 一探究竟。

## 背景知识

### fuse
原理参考
- [知乎专栏: 5 分钟搞懂用户空间文件系统 FUSE 工作原理](https://zhuanlan.zhihu.com/p/106719192)  差不多真的就 5 分钟就可以看懂
- [兰新宇: 用户态文件系统 - FUSE](https://zhuanlan.zhihu.com/p/143256077) 可以更加深入的理解

用户态的 daemon 的实现:
- [libfuse](https://github.com/libfuse/libfuse)
- [github : Rust FUSE library for server, virtio-fs and vhost-user-fs](https://github.com/cloud-hypervisor/fuse-backend-rs)

基于 fuse 开发的有意思的项目:
- [sshfs](https://github.com/libfuse/sshfs)
- [etcdfs : 文件浏览器查看 etcd 的内容](https://github.com/polyrabbit/etcdfs)

### virtio

## virtio fs

重点参考:
https://vmsplice.net/~stefan/virtio-fs_%20A%20Shared%20File%20System%20for%20Virtual%20Machines.pdf

- [ ] 没有太搞清楚整个执行流程啊
  - [ ] 为什么 guest 是和 virtiofd 沟通的，应该是首先和 QEMU 沟通才对啊
