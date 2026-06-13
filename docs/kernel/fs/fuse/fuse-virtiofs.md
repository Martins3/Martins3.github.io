# virtio-fs 简单尝试
## 如果后端死掉了，那么会出现一直卡死在这里

https://www.reddit.com/r/NixOS/comments/149pvwd/virtiofsd_stopped_working/

```txt
➜  ~ cat /proc/1460/stack
[<0>] fuse_get_req+0xf8/0x280
[<0>] fuse_simple_request+0x48/0x5e0
[<0>] fuse_do_getattr+0x1f2/0x3c0
[<0>] fuse_permission+0x165/0x3a0
[<0>] inode_permission+0x8c/0x160
[<0>] __se_sys_chdir+0x6a/0x110
[<0>] do_syscall_64+0xef/0x210
[<0>] entry_SYSCALL_64_after_hwframe+0x77/0x7f
```

## virtiofs
- [ ] https://virtio-fs.gitlab.io/ : /home/maritns3/core/54-linux/fs/fuse/virtio_fs.c : 只有 1000 多行，9pfs 也很小，这些东西有什么特殊的地方吗 ?

首先将 virtiofs 用起来: https://www.tauceti.blog/post/qemu-kvm-share-host-directory-with-vm-with-virtio/
> virtio-fs on the other side is designed to offer local file system semantics and performance. virtio-fs takes advantage of the virtual machine’s co-location with the hypervisor to avoid overheads associated with network file systems. virtio-fs uses FUSE as the foundation. Unlike traditional FUSE where the file system daemon runs in userspace, the virtio-fs daemon runs on the host. A VIRTIO device carries FUSE messages and provides extensions for advanced features not available in traditional FUSE.

## virtio fs

重点参考:
https://vmsplice.net/~stefan/virtio-fs_%20A%20Shared%20File%20System%20for%20Virtual%20Machines.pdf

- [ ] 没有太搞清楚整个执行流程啊
  - [ ] 为什么 guest 是和 virtiofd 沟通的，应该是首先和 QEMU 沟通才对啊


支持 windows 需要什么?
岂不是说，windows 也是需要实现 fuse

https://www.heiko-sieger.info/sharing-files-between-the-linux-host-and-a-windows-vm-using-virtiofs/

## 从后端断开之后
```txt
[ 1571.428298][   T39] Call trace:
[ 1571.430669][   T39]  __switch_to+0x130/0x1e8
[ 1571.433604][   T39]  __schedule+0x648/0xce0
[ 1571.436777][   T39]  schedule+0x58/0xf0
[ 1571.439458][   T39]  request_wait_answer+0x168/0x2d0 [fuse]
[ 1571.442674][   T39]  __fuse_request_send+0xcc/0x100 [fuse]
[ 1571.445899][   T39]  fuse_simple_request+0xa4/0x230 [fuse]
[ 1571.449164][   T39]  fuse_do_getattr+0xcc/0x1f8 [fuse]
[ 1571.452526][   T39]  fuse_permission+0x134/0x240 [fuse]
[ 1571.454562][   T39]  inode_permission+0xe4/0x1e0
[ 1571.456470][   T39]  link_path_walk.part.0.constprop.0+0x2a8/0x390
[ 1571.458797][   T39]  path_openat+0xac/0x268
[ 1571.460625][   T39]  do_filp_open+0x88/0x140
[ 1571.462365][   T39]  do_sys_openat2+0x204/0x268
[ 1571.464271][   T39]  __arm64_sys_openat+0x6c/0xb8
[ 1571.466205][   T39]  invoke_syscall+0x50/0x128
[ 1571.468299][   T39]  el0_svc_common.constprop.0+0xc8/0xf0
[ 1571.470130][   T39]  do_el0_svc+0x24/0x38
[ 1571.471479][   T39]  el0_svc+0x4c/0x1f8
[ 1571.472719][   T39]  el0t_64_sync_handler+0x100/0x130
[ 1571.474091][   T39]  el0t_64_sync+0x188/0x190
```

如果 qemu 没有配置 shared 参数
```txt
➜  ~ cat /proc/1869/stack
[<0>] fuse_get_req+0xf8/0x2a0
[<0>] fuse_simple_request+0x48/0x5e0
[<0>] fuse_do_getattr+0x1ef/0x3d0
[<0>] fuse_permission+0x173/0x3a0
[<0>] inode_permission+0x8c/0x160
[<0>] __se_sys_chdir+0x6a/0x110
[<0>] do_syscall_64+0xed/0x210
[<0>] entry_SYSCALL_64_after_hwframe+0x77/0x7f
```
## guest 日志
```txt
[   47.949744] SELinux: (dev virtiofs, type virtiofs) getxattr errno 4
```

```txt
[  112.566875] SELinux: (dev virtiofs, type virtiofs) has no security xattr handler
[  112.566881] SELinux: (dev virtiofs, type virtiofs) falling back to genfs
```

## 基本流程

```txt
🧀  l
Permissions Size User     Date Modified Name
lr-x------     - martins3 26 Sep 10:37   0 -> pipe:[21296251]
l-wx------     - martins3 26 Sep 10:37   1 -> /home/martins3/.local/share/pueue/task_logs/68.log
l-wx------     - martins3 26 Sep 10:37   2 -> /home/martins3/.local/share/pueue/task_logs/68.log
l-wx------     - martins3 26 Sep 10:37   3 -> /home/martins3/hack/vm/oe2403/vfsd.sock.pid
lrwx------     - martins3 26 Sep 10:37   4 -> socket:[21299373]
l---------     - martins3 26 Sep 10:37   5 -> /
lrwx------     - martins3 26 Sep 10:37   6 -> anon_inode:[eventfd]
lrwx------     - martins3 26 Sep 10:37   7 -> anon_inode:[eventpoll]
l---------     - martins3 26 Sep 10:37   8 -> /
lr-x------     - martins3 26 Sep 10:37   9 -> /proc/3684703/mountinfo
lrwx------     - martins3 26 Sep 10:37   10 -> anon_inode:[eventfd]
lrwx------     - martins3 26 Sep 10:37   11 -> socket:[21301333]
l---------     - martins3 26 Sep 10:37   12 -> /
lrwx------     - martins3 26 Sep 10:37   13 -> anon_inode:[eventfd]
lrwx------     - martins3 26 Sep 10:37   14 -> /dev/shm/88
lrwx------     - martins3 26 Sep 10:37   15 -> anon_inode:[eventfd]
lrwx------     - martins3 26 Sep 10:37   16 -> anon_inode:[eventfd]
lrwx------     - martins3 26 Sep 10:37   17 -> /dev/shm/88
lrwx------     - martins3 26 Sep 10:37   18 -> /dev/shm/88
lrwx------     - martins3 26 Sep 10:37   19 -> anon_inode:[eventfd]
lrwx------     - martins3 26 Sep 10:37   20 -> anon_inode:[eventfd]
lrwx------     - martins3 26 Sep 10:37   21 -> anon_inode:[eventfd]
lrwx------     - martins3 26 Sep 10:37   22 -> /a
l---------     - martins3 26 Sep 10:37   23 -> /mlnx-iproute2-5.11.0-1.54310.x86_64.rpm
l---------     - martins3 26 Sep 10:37   24 -> /a
```

## guest os 中的观测

guest os 中主要是这个:
```txt
-   10.25%     0.13%  fio              [kernel.kallsyms]  [k] __se_sys_io_submit                                                                                                               ▒
   - 10.13% __se_sys_io_submit                                                                                                                                                                 ▒
      - 9.77% io_submit_one                                                                                                                                                                    ▒
         - 8.50% aio_write                                                                                                                                                                     ▒
            - 8.21% fuse_file_write_iter                                                                                                                                                       ▒
               - 7.49% generic_file_direct_write                                                                                                                                               ▒
                  - 7.42% fuse_direct_IO                                                                                                                                                       ▒
                     - 6.86% fuse_direct_io                                                                                                                                                    ▒
                        - 4.01% fuse_async_req_send                                                                                                                                            ▒
                           - 3.81% fuse_simple_background                                                                                                                                      ▒
                              - 3.18% flush_bg_queue                                                                                                                                           ▒
                                 - 3.09% virtio_fs_wake_pending_and_unlock                                                                                                                     ▒
                                    - 2.94% virtio_fs_enqueue_req                                                                                                                              ▒
                                         1.14% _raw_spin_lock                                                                                                                                  ▒
                                       + 0.90% virtqueue_add_sgs                                                                                                                               ▒
                        - 0.97% iov_iter_extract_pages                                                                                                                                         ▒
                             0.92% gup_fast_fallback
```
实在是有趣，原来是直接将 virtiofs 挂载的识别为 fuse ，然后通过 virtio 将数据发给 host ，
qemu 将消息转发给 daemon ，daemon 无需 fuse ，直接操作文件。

## TODO
- virtiofsd 自身的参数很多，也是可以关注一下的
- 猜测 virtiofd 是通过 uds 来接受中断的
  - 所以 dpdk 的代码的实现也是需要非 polling 的模式吗?
- fuse 是

获取 mount 的地方:
```txt
grep virtiofs /proc/mounts | awk '{ print $2 }'
```

## virtiofsd
- 之前 https://gitlab.com/virtio-fs/virtiofsd ，似乎之前是在 qemu 中的，后来单独实现的
  - 这个项目还是可以看看的

不知道这里说明的 src/passthrough 指的是什么


## 想法 : 其实 guest host 之间的共享可以直接使用 nfs 的
所以，也可以使用 fuse 来实现 nfs 的效果，也就是在机器 A 上对于文件系统的操作，
所以操作全部都通过网络发送给机器 B ，机器 B 的用户态的类似 virtiofsd 的来把这些操作
重放到 B 的文件系统上。

当然，其实意义不大，因为 nfs 上的操作直接进入到 nfs 中，只是用了 rpc 而已，
而 virtio 适用于 guest 和 host 直接:

```txt
[<0>] __switch_to+0xf0/0x158
[<0>] rpc_wait_bit_killable+0x2c/0xc0 [sunrpc]
[<0>] __rpc_execute+0x134/0x4f0 [sunrpc]
[<0>] rpc_execute+0x68/0xe8 [sunrpc]
[<0>] rpc_run_task+0x138/0x190 [sunrpc]
[<0>] rpc_call_sync+0x70/0xc8 [sunrpc]
[<0>] nlmclnt_call+0xb8/0x2c8 [lockd]
[<0>] nlmclnt_proc+0x4d4/0x770 [lockd]
[<0>] nfs3_proc_lock+0x4c/0xe8 [nfsv3]
[<0>] nfs_lock+0x1ec/0x2c0 [nfs]
[<0>] vfs_test_lock+0x38/0x60
[<0>] fcntl_getlk+0xac/0x158
[<0>] do_fcntl+0x384/0x950
[<0>] __arm64_sys_fcntl+0x94/0xc0
[<0>] el0_svc_common+0x78/0x178
[<0>] el0_svc_handler+0x38/0x78
[<0>] el0_svc+0x8/0x640
[<0>] 0xffffffffffffffff
```

## 两个问题
### 强制重启之后，系统无法启动
当虚拟机被强制关机之后，发现 ssh 最后会卡住，然后 ssh root ，可以进去
通过 strace -f su martins3 ，才发现是加载了共享的文件:

```txt
[root@bogon ~]# cat /proc/3492/stack
[<0>] fuse_get_req+0xf8/0x2a0
[<0>] fuse_simple_request+0x48/0x5e0
[<0>] fuse_do_getattr+0x1ef/0x3d0
[<0>] fuse_permission+0x173/0x3a0
[<0>] inode_permission+0x8c/0x160
[<0>] link_path_walk+0x2e4/0x3d0
[<0>] path_lookupat+0x5a/0x100
[<0>] filename_lookup+0xd5/0x1b0
[<0>] vfs_statx+0x83/0x210
[<0>] __se_sys_statx+0xf0/0x140
[<0>] do_syscall_64+0xed/0x200
[<0>] entry_SYSCALL_64_after_hwframe+0x77/0x7f
```
这就很奇怪了，qemu 和 virtiofsd 都重启了多次，不知道为什么还是会 crash 。

### 无法创建新文件

2024-10-28 发现的，qemu 和 kernel 都是当时最新的
可以修改文件，但是无法创建新的文件:
```txt
openat(AT_FDCWD, "aaaaaaaaaaaaaaaaaaaaaaaaaaa", O_WRONLY|O_CREAT|O_NOCTTY|O_NONBLOCK, 0666) = -1 EINVAL (Invalid argument)
utimensat(AT_FDCWD, "aaaaaaaaaaaaaaaaaaaaaaaaaaa", NULL, 0) = -1 ENOENT (No such file or directory)
newfstatat(AT_FDCWD, "aaaaaaaaaaaaaaaaaaaaaaaaaaa", 0x7ffce8009650, 0) = -1 ENOENT (No such file or directory)
```

### 看看 virtme-ng 是如何解决的吧
- https://github.com/arighi/virtme-ng


## 真的就不能开箱即用吗?

无法 changeowner
```txt
 sudo chown martins3 tmp
chown: changing ownership of 'tmp': Invalid argument

# 尝试 umount ，然后卡主
sudo umount tmp
```

```txt
[sudo] password for martins3:
[<0>] virtio_fs_drain_all_queues_locked+0x39/0xb0 [virtiofs]
[<0>] virtio_kill_sb+0x82/0x150 [virtiofs]
[<0>] deactivate_locked_super+0x30/0xa0
[<0>] cleanup_mnt+0xba/0x150
[<0>] task_work_run+0x59/0x90
[<0>] syscall_exit_to_user_mode+0x1f7/0x200
[<0>] do_syscall_64+0x7e/0x180
[<0>] entry_SYSCALL_64_after_hwframe+0x76/0x7e
```

同时可以观察到日志，不知道和 chown 有没有关系:
```txt
[2025-07-10T12:04:13Z WARN  virtiofsd::limits] Failure when trying to set the limit to 1000000, the hard limit (524288) of open file descriptors is used instead.
[2025-07-10T12:04:13Z WARN  virtiofsd::sandbox] Couldn't set the process uid as root: -1
[2025-07-10T12:04:13Z WARN  virtiofsd::sandbox] Couldn't set the process gid as root: -1
[2025-07-10T12:04:13Z INFO  virtiofsd] Waiting for vhost-user socket connection...
[2025-07-10T12:04:14Z INFO  virtiofsd] Client connected, servicing requests
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
