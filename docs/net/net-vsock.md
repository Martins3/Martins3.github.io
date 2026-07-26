# vsock

## 为什么需要 vsock
https://gist.github.com/mcastelino/9a57d00ccf245b98de2129f0efe39857
才意识到，vsock 也是非常适合放到内核态的，因为物理机和虚拟机沟通，还是需要走物理机的 sock
虚拟机也是在内核中，这样就少了上下文切换了

- 如何使用上: https://gist.github.com/mcastelino/9a57d00ccf245b98de2129f0efe39857
- 看上去是用于改善: https://vmsplice.net/~stefan/stefanha-kvm-forum-2015.pdf
https://github.com/rust-vmm/vhost-device : 哦，原来 vhost 存在这么多的设备啊

- https://static.sched.com/hosted_files/kvmforum2019/50/KVMForum_2019_virtio_vsock_Andra_Paraschiv_Stefano_Garzarella_v1.3.pdf
- https://stefano-garzarella.github.io/posts/2019-08-22-vsock-iperf3/
  - 作者提供的测试 vsock 的性能
- https://gist.github.com/nrdmn/7971be650919b112343b1cb2757a3fe6
  - 参考，这个可以

- https://kubevirt.io/user-guide/virtual_machines/vsock/ : 分析了 vsock 相对于 guest 的好处


个人感受就是，现在不需要配置网卡就可以 ssh 到虚拟机了，对于轻量级虚拟机是很好的。

## 基本代码分析
看代码实现的位置内容其实不少
net/vmw_vsock/

看看这些配置的左右吧
```txt
CONFIG_VSOCKETS=y
CONFIG_VSOCKETS_DIAG=y
CONFIG_VSOCKETS_LOOPBACK=y
CONFIG_VIRTIO_VSOCKETS=y
CONFIG_VIRTIO_VSOCKETS_COMMON=y
# CONFIG_VSOCKMON is not set
```

### qemu 实现
qemu-system-x86_64 -device help

根据 vsock_transport::cancel_pkt 的注册位置，可以找到
- vhost_transport_cancel_pkt : 这个应该是 host 的
- virtio_transport_cancel_pkt : 这个应该是 guest 的
- vsock_loopback_cancel_pkt

也就是存在 vsock 的三个实现，

- [ ] https://man7.org/linux/man-pages/man7/vsock.7.html

### 内核实现
- net.c 和 vsock.c 是两个对称的模块, 但是从 net.c 分析

- [ ] ioctl 提供了很多让用户态访问的接口，难道不是让 kernel 处理这些事情不就可以了吗 ?

```c
static const struct file_operations vhost_net_fops = {
    .owner          = THIS_MODULE,
    .release        = vhost_net_release,
    .read_iter      = vhost_net_chr_read_iter,
    .write_iter     = vhost_net_chr_write_iter,
    .poll           = vhost_net_chr_poll,
    .unlocked_ioctl = vhost_net_ioctl,
    .compat_ioctl   = compat_ptr_ioctl,
    .open           = vhost_net_open,
    .llseek     = noop_llseek,
};

static const struct file_operations vhost_vsock_fops = {
    .owner          = THIS_MODULE,
    .open           = vhost_vsock_dev_open,
    .release        = vhost_vsock_dev_release,
    .llseek     = noop_llseek,
    .unlocked_ioctl = vhost_vsock_dev_ioctl,
    .compat_ioctl   = compat_ptr_ioctl,
    .read_iter      = vhost_vsock_chr_read_iter,
    .write_iter     = vhost_vsock_chr_write_iter,
    .poll           = vhost_vsock_chr_poll,
};
```

- `vhost_net_open` : 注册各种 handler
  - 创建两个 `struct vhost_virtqueue` 队列，分别用于收发
  - `vhost_dev_init`
  - `vhost_poll_init(n->poll + VHOST_NET_VQ_TX, handle_tx_net, EPOLLOUT, dev);`
    - `vhost_poll_init(n->poll + VHOST_NET_VQ_RX, handle_rx_net, EPOLLIN, dev);`

看看 ioctl 的实现:
- `vhost_net_ioctl`
  - [ ] `VHOST_NET_SET_BACKEND`
  - [x] `VHOST_SET_OWNER` : 为了让一个打开的 vhost-net fd 和一个进程关联起来
    - `vhost_dev_set_owner` 将当前进程的 `mm_struct` 赋值到 `vhost_dev` 的 mm 成员中(`dev->mm`)，然后创建一个内核线程 vhost_worker
      - `kthread_use_mm(dev->mm);` : 刷新了对于内核线程和 mm 的理解
      - `node = llist_del_all(&dev->work_list);` : 将 `vhost_dev->work_list` 所有的 work 取下来工作
  - `vhost_dev_ioctl`
    - [x] `VHOST_SET_MEM_TABLE` : 告诉虚拟机的物理地址布局信息
      - [x] virtio 规定 virtqueue 是虚拟机分配的，这些地址是通过 PCI 传递给用户层的，同时用户层掌握了 memory region 是映射规则, 所以内核为了获取到 vring 的一些地址需要这些映射信息
  - `vhost_vring_ioctl`
    - [x] `VHOST_SET_VRING_KICK`


- [ ] 为什么需要 vhost-net fd ?

## 问题
1. vhost-vsock 似乎是支持热迁移的，这就有点难以想到如何实现了。
2. vhost-vsock 之外，还有普通的 vsock 吗? 现在的参数重视这么配置的
```txt
-device vhost-vsock-pci,id=vhost-vsock-pci0,guest-cid=1096
```

## 基本的配置和使用

虚拟机至少需要打开这个模块，其他什么模块还需要打开不需要添加

CONFIG_VSOCKETS=m
```txt
vsock_diag             12288  0
vmw_vsock_virtio_transport    20480  0
vmw_vsock_virtio_transport_common    57344  1 vmw_vsock_virtio_transport
vsock                  69632  5 vmw_vsock_virtio_transport_common,vmw_vsock_virtio_transport,vsock_diag
```

物理机中需要一下模块:
```txt
vhost_vsock            28672  4
vsock_loopback         12288  0
vmw_vsock_virtio_transport_common    61440  2 vhost_vsock,vsock_loopback
vmw_vsock_vmci_transport    57344  0
vsock                  77824  4 vmw_vsock_virtio_transport_common,vhost_vsock,vsock_loopback,vmw_vsock_vmci_transport
vmw_vmci              122880  1 vmw_vsock_vmci_transport
vhost                  77824  2 vhost_vsock,vhost_net
```

## 问题

### systemd 如何支持 vsock

这相当于实现了一个不用网卡的 vsock 配置方法:
```txt
🧀  ssh vsock%1100
Warning: Permanently added 'vsock%1100' (ED25519) to the list of known hosts.
Web console: https://fedora44-server:9090/ or https://10.0.2.15:9090/

Last login: Sat Jul 11 09:01:52 2026 from 10.0.2.2
```

关键不是 Fedora 装了某个额外 SSH 服务，而是 systemd 259 自带的 SSH/vsock 集成：

- host 侧 ssh vsock%1100 命中这个 SSH 配置：
```txt
/usr/lib/systemd/ssh_config.d/20-systemd-ssh-proxy.conf
```
其中内容为:
```txt
  Host unix/* unix%* vsock/* vsock%* vsock-mux/* vsock-mux%* machine/* machine%*
          ProxyCommand /usr/lib/systemd/systemd-ssh-proxy %h %p
          ProxyUseFdpass yes
```

所以 ssh vsock%1100 实际是 OpenSSH 调用：

/usr/lib/systemd/systemd-ssh-proxy vsock%1100 22
由 systemd-ssh-proxy 建立 AF_VSOCK 连接，再把 fd 交给 ssh。

guest 侧 Fedora 44 自动生成了这个 socket：

```txt
# /run/systemd/generator/sshd-vsock.socket
[Socket]
ListenStream=vsock::22
Accept=yes
```

```txt
/usr/lib/systemd/system-generators/systemd-ssh-generator
```
工作方式是：guest 里 systemd 监听 vsock::22，每个连接 socket-activate 一个 sshd@.service，里面跑：

```txt
/usr/sbin/sshd -i
```

所以结论是：
1. ssh vsock%1100 是 systemd 的 SSH proxy 语法，不是 OpenSSH 原生直接懂 vsock。
2. Fedora 44 guest 里 systemd 自动生成 sshd-vsock.socket，所以不需要 ncat。
3. 这个方案比我们的 ncat --vsock -l -k ... -c "ncat 127.0.0.1 22" 更正统，也更干净。

我们后面可以把 collei 的 setup_vsock_ssh 改成优先使用 systemd-ssh-generator：如果 guest 有 /usr/lib/systemd/
system-generators/systemd-ssh-generator，就只需要 systemctl daemon-reload && systemctl start sshd-vsock.socket；
没有这个能力的旧系统再 fallback 到 ncat service。

#### 简单模拟

ncat --vsock -l -k 2222 -c "ncat 127.0.0.1 22 这个命令如何理解?
┌────────────────────────┬────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ 参数                   │ 含义                                                                                               │
├────────────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ ncat                   │ nmap 出品的网络工具（升级版 netcat）                                                               │
├────────────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ --vsock                │ 使用 VM Socket 协议，而不是 TCP/UDP                                                                │
├────────────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ -l                     │ 监听模式（listen）                                                                                 │
├────────────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ -k                     │ keep-open，一个连接断开后继续监听，不退出                                                          │
├────────────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ $port                  │ 要监听的 vsock 端口号（CID 由上下文决定）                                                          │
├────────────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ -c "ncat 127.0.0.1 22" │ 每收到一个 vsock 连接，就 fork 执行这条子命令，并把该连接的 stdin/stdout 接到子命令的 stdin/stdout │
└────────────────────────┴────────────────────────────────────────────────────────────────────────────────────────────────────┘

里面的 ncat 127.0.0.1 22 就是再去连接本机的 TCP 22 端口，也就是 SSH 服务。

所以整体流程是：

1. 某个 VM（或 host）通过 vsock 连到这个监听端
2. ncat 收到连接后启动子进程 ncat 127.0.0.1 22
3. 子进程连到本机 SSH
4. 两个连接的数据被互相转发

也就是，在 vsock 端口上搭了一个“桥”，把 vsock 流量桥接到本地 SSH，方便 VM 免网络配置就能 ssh 到 host。

### 为什么不可以使用 0 1 2 作为 guest-cid=

guest-cid=1100 可以，不能用的是 0/1/2 这些保留 CID。

内核头文件 /usr/include/linux/vm_sockets.h 里定义了：

#define VMADDR_CID_HYPERVISOR 0
#define VMADDR_CID_LOCAL      1
#define VMADDR_CID_HOST       2

含义是：

- 0: hypervisor
- 1: local / loopback，本地 vsock 通信
- 2: host，也就是 guest 里连接宿主机时常用的目标 CID

所以 QEMU 的：

-device vhost-vsock-pci,guest-cid=...

这里的 guest-cid 是给“这个 guest 自己”的 CID，不能占用这些 well-known CID。实际 guest CID 应该从 3 开始，并且同一 host 上运行的 guest 之间要唯一。



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
