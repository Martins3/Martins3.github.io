## nbd

将 `blk_mq_ops` 替换成为网络的接口就可以了

```c
static const struct blk_mq_ops nbd_mq_ops = {
    .queue_rq   = nbd_queue_rq,
    .complete   = nbd_complete_rq,
    .init_request   = nbd_init_request,
    .timeout    = nbd_xmit_timeout,
};
```
相对于 nfs 5 万多行，而 nbd 只要 2500 行就结束了。

## 市场现状
- https://github.com/NetworkBlockDevice/nbd
- https://github.com/NetworkBlockDevice/nbd/blob/master/doc/proto.md

- https://www.qemu.org/2021/08/22/fuse-blkexport/

- https://manpages.ubuntu.com/manpages/bionic/man8/nbd-client.8.html
- https://manpages.debian.org/testing/qemu-utils/qemu-nbd.8.en.html

qemu 是完全不需要 nbd kernel module 的

nbd kernel module 是为了实现为 /dev/ndb 文件的，所以 qemu 其实不需要 nbd 的

## qemu 快速操作
```sh
  qemu-nbd  --export-name=xueshi --port 6000 --persistent --shared=2 --format=qcow2 1.qcow2
  # 展示
  nbd-client -l 10.0.0.5 6000
  # 链接到本地的网络中
  sudo nbd-client -name xueshi 10.0.0.5 6000 /dev/nbd2
```

似乎这么配置就可以了:
```txt
	-device scsi-hd,bus=scsi4.0,channel=0,scsi-id=0,lun=10,drive=root1,id=root1 \
	-blockdev driver=nbd,server.type=inet,server.host=10.0.0.5,server.port=6000,export=xueshi,node-name=root1 \
```

## nbd 的配置
使用参考 qemu 的: docs/tools/qemu-nbd.rst ，也就是 man qemu-nbd 而已。

如何理解 export name ?
```txt
🧀  nbd-client -l 10.0.0.5 6000
Negotiation: ..
xueshi
```

1. 解决端口问题，希望一个 server 代理所有的公用一个?

哦，还可以使用这个方法调试
```txt
  -c, --connect=DEV         connect FILE to the local NBD device DEV
```

```txt
qemu-img create -f qcow2 1.qcow2 180G
sudo qemu-nbd -c /dev/nbd1 -f qcow2 2.qcow2
sudo qemu-nbd -d /dev/nbd0
```
添加之后，通过 pstree 可以看到
```txt
🧀  pstree
systemd─┬─NetworkManager─┬─dhclient
        │                └─3*[{NetworkManager}]
        ├─3*[agetty]
        ├─chronyd
        ├─crond
        ├─dbus-daemon
        ├─gssproxy───5*[{gssproxy}]
        ├─irqbalance───{irqbalance}
        ├─iscsid
        ├─ovs-vswitchd───5*[{ovs-vswitchd}]
        ├─ovsdb-server
        ├─polkitd───3*[{polkitd}]
        ├─pueued───6*[{pueued}]
        ├─qemu-nbd───2*[{qemu-nbd}]
        ├─qemu-nbd───3*[{qemu-nbd}]
        ├─rngd───4*[{rngd}]
        ├─rpcbind
        ├─rsyslogd───2*[{rsyslogd}]
        ├─sshd─┬─sshd───sshd───zsh
        │      └─sshd───sshd───zsh───pstree
        ├─systemd-journal
        ├─systemd-logind
        ├─systemd-udevd───(udev-worker)
        ├─tailscaled───9*[{tailscaled}]
        └─tuned───3*[{tuned}]
```

大致可以猜到实现原理，
例如，如果网络是通过 uds 的，那么结果为:
```txt
-   14.17%     0.33%  fio              [kernel.kallsyms]                [k] do_syscall_64
   - 13.83% do_syscall_64
      - 13.33% __x64_sys_io_uring_enter
         - 8.28% io_submit_sqes
            - 8.13% io_issue_sqe
               - 7.98% io_read
                  - __io_read
                     - 7.89% blkdev_read_iter
                        - 7.78% blkdev_direct_IO
                           - 6.60% submit_bio_noacct_nocheck
                              - 6.41% __submit_bio
                                 - 5.32% __blk_flush_plug
                                    - blk_mq_flush_plug_list
                                       - 4.81% blk_mq_run_hw_queue
                                          - 4.56% blk_mq_sched_dispatch_requests
                                             - __blk_mq_sched_dispatch_requests
                                                - 3.58% blk_mq_dispatch_rq_list
                                                   - nbd_queue_rq
                                                      - 3.07% nbd_send_cmd
                                                         - 3.03% __sock_xmit
                                                            - sock_sendmsg
                                                               - unix_stream_sendmsg
                                                                  - 1.76% sock_def_readable
                                                                       1.67% _raw_spin_unlock_irqrestore
                                   1.05% blk_mq_submit_bio
                             0.57% bio_iov_iter_get_pages
         + 4.50% io_cqring_wait
```

## modprobe nbd 会立刻获取到 16 这设备

```txt
lrwxrwxrwx - root  6 Mar 15:40  nbd0 -> ../../devices/virtual/block/nbd0
lrwxrwxrwx - root  6 Mar 15:40  nbd15 -> ../../devices/virtual/block/nbd15
```

## 如何自动化?
https://gist.github.com/derekp7/9978986 : 似乎这个已经很简单了

## 原来 libndb 有了 ublk/ 的支持啊

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
