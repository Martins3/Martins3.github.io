# vhost.c

- `vhost_vq_reset`
- `vhost_dev_set_owner`
- `vhost_dev_ioctl`

## vsock
- 如何使用上: https://gist.github.com/mcastelino/9a57d00ccf245b98de2129f0efe39857
- 看上去是用于改善: https://vmsplice.net/~stefan/stefanha-kvm-forum-2015.pdf

## vDPA
阅读一下 `driver/vdpa/vdpa_sim` 中的内容，应该可以便于理解一点吧！

- [ ] vdpa-blk : https://www.youtube.com/watch?v=zyDSUU0TLB4

## net.c 和 vsock.c 是两个对称的模块, 但是从 net.c 分析
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
