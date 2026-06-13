## 先将 fabrics 也放到这里吧

https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/managing_storage_devices/configuring-nvme-over-fabrics-using-nvme-rdma_managing-storage-devices

## 参考这个搭建一下

- https://news.ycombinator.com/item?id=38280472
  - https://github.com/poettering/diskomator

我靠，似乎超级简单的
https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/9/html/managing_storage_devices/configuring-nvme-over-fabrics-using-nvme-tcp_managing-storage-devices

## 操作一波

https://futurewei-cloud.github.io/ARM-Datacenter/qemu/nvme-of-tcp-vms/

需要加载:
```sh
sudo modprobe nvmet
sudo modprobe nvmet-tcp
```

具体操作参考: vn/code/guest-setup/nvme.sh


不知道为啥，自己构建的内核无法链接上，但是 fedora 或者 anolis 都是可以正常连接的:
```txt
[ 2050.097748] nvme_fabrics: found same hostid 8df3f6ee-194e-43c0-9c76-43a8bbd82a9f but different hostnqn nqn.2014-08.org.nvmexpress:uuid:c827ae3a-59b8-4697-8ba5-1b705fd52113
```
1. 也许是 latest kernel 存在这个问题
2. 也许是自己构建内核存在这个问题

## 将 host 的 nvme0n1 使用 nvme-tcp 给 guest

在 guest 中设置为 xfs ，在其上面创建文件，然后在 host 上可以 mount 上，但是创建文件的过程中，会出现如下错误:

```txt
[170283.343362] nvmet: creating nvm controller 1 for subsystem nvme-test-target for NQN nqn.2014-08.org.nvmexpress:uuid:00000000-0000-0000-0000-000000000000.
[170333.731101] XFS (nvme0n1): Mounting V5 Filesystem 3685b57e-b230-45eb-a3c9-740ab56519b8
[170333.733779] XFS (nvme0n1): Ending clean mount
[170388.353438] XFS (nvme0n1): Internal error rec.ir_free != frec->ir_free || rec.ir_freecount != frec->ir_freecount at line 1441 of file fs/xfs/libxfs/xfs_ialloc.c.  Caller x
fs_dialloc_ag_update_inobt+0xee/0x180 [xfs]
[170388.353532] CPU: 12 PID: 284107 Comm: touch Tainted: P           O       6.8.1 #1-NixOS
[170388.353534] Hardware name: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
[170388.353535] Call Trace:
[170388.353537]  <TASK>
[170388.353538]  dump_stack_lvl+0x47/0x60
[170388.353541]  xfs_corruption_error+0x94/0xa0 [xfs]
[170388.353613]  ? xfs_dialloc_ag_update_inobt+0xee/0x180 [xfs]
[170388.353680]  ? xfs_inobt_get_rec+0xbd/0x160 [xfs]
[170388.353745]  xfs_dialloc_ag_update_inobt+0x11d/0x180 [xfs]
[170388.353811]  ? xfs_dialloc_ag_update_inobt+0xee/0x180 [xfs]
[170388.353878]  xfs_dialloc+0x459/0x780 [xfs]
[170388.353950]  xfs_create+0x254/0x5d0 [xfs]
[170388.354026]  xfs_generic_create+0x2ef/0x360 [xfs]
[170388.354100]  path_openat+0xe99/0x1180
[170388.354103]  do_filp_open+0xb3/0x160
[170388.354104]  do_sys_openat2+0xab/0xe0
[170388.354106]  __x64_sys_openat+0x6e/0xa0
[170388.354107]  do_syscall_64+0xc5/0x210
[170388.354109]  entry_SYSCALL_64_after_hwframe+0x6f/0x77
[170388.354110] RIP: 0033:0x7f28cd316a85
[170388.354133] Code: 75 53 89 f0 25 00 00 41 00 3d 00 00 41 00 74 45 80 3d ee 28 0e 00 00 74 69 89 da 48 89 ee bf 9c ff ff ff b8 01 01 00 00 0f 05 <48> 3d 00 f0 ff ff 0f 87 8
f 00 00 00 48 8b 54 24 28 64 48 2b 14 25
[170388.354134] RSP: 002b:00007ffe9e010b80 EFLAGS: 00000202 ORIG_RAX: 0000000000000101
[170388.354136] RAX: ffffffffffffffda RBX: 0000000000000941 RCX: 00007f28cd316a85
[170388.354136] RDX: 0000000000000941 RSI: 00007ffe9e01130e RDI: 00000000ffffff9c
[170388.354137] RBP: 00007ffe9e01130e R08: 0000000000000000 R09: 0000000000000000
[170388.354137] R10: 00000000000001b6 R11: 0000000000000202 R12: 00007ffe9e01130e
[170388.354138] R13: 0000000000000000 R14: 00007f28cd3f1248 R15: 00000000ffffffff
[170388.354139]  </TASK>
[170388.354142] XFS (nvme0n1): Corruption detected. Unmount and run xfs_repair
[170388.354145] XFS (nvme0n1): Internal error xfs_trans_cancel at line 1109 of file fs/xfs/xfs_trans.c.  Caller xfs_create+0x2b5/0x5d0 [xfs]
[170388.354214] CPU: 12 PID: 284107 Comm: touch Tainted: P           O       6.8.1 #1-NixOS
[170388.354215] Hardware name: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
[170388.354215] Call Trace:
[170388.354215]  <TASK>
[170388.354216]  dump_stack_lvl+0x47/0x60
[170388.354217]  xfs_trans_cancel+0x131/0x150 [xfs]
[170388.354289]  xfs_create+0x2b5/0x5d0 [xfs]
[170388.354356]  xfs_generic_create+0x2ef/0x360 [xfs]
[170388.354422]  path_openat+0xe99/0x1180
[170388.354424]  do_filp_open+0xb3/0x160
[170388.354425]  do_sys_openat2+0xab/0xe0
[170388.354426]  __x64_sys_openat+0x6e/0xa0
[170388.354427]  do_syscall_64+0xc5/0x210
[170388.354428]  entry_SYSCALL_64_after_hwframe+0x6f/0x77
[170388.354429] RIP: 0033:0x7f28cd316a85
[170388.354431] Code: 75 53 89 f0 25 00 00 41 00 3d 00 00 41 00 74 45 80 3d ee 28 0e 00 00 74 69 89 da 48 89 ee bf 9c ff ff ff b8 01 01 00 00 0f 05 <48> 3d 00 f0 ff ff 0f 87 8
f 00 00 00 48 8b 54 24 28 64 48 2b 14 25
[170388.354432] RSP: 002b:00007ffe9e010b80 EFLAGS: 00000202 ORIG_RAX: 0000000000000101
[170388.354432] RAX: ffffffffffffffda RBX: 0000000000000941 RCX: 00007f28cd316a85
[170388.354433] RDX: 0000000000000941 RSI: 00007ffe9e01130e RDI: 00000000ffffff9c
[170388.354433] RBP: 00007ffe9e01130e R08: 0000000000000000 R09: 0000000000000000
[170388.354433] R10: 00000000000001b6 R11: 0000000000000202 R12: 00007ffe9e01130e
[170388.354434] R13: 0000000000000000 R14: 00007f28cd3f1248 R15: 00000000ffffffff
[170388.354435]  </TASK>
[170388.354439] XFS (nvme0n1): Corruption of in-memory data (0x8) detected at xfs_trans_cancel+0x14a/0x150 [xfs] (fs/xfs/xfs_trans.c:1110).  Shutting down filesystem.
[170388.354506] XFS (nvme0n1): Please unmount the filesystem and rectify the problem(s)
```

## 有趣哇

- [Cloning a Laptop over NVMe TCP](https://news.ycombinator.com/item?id=39676767)

## 似乎 broadcom 也有这种，但是好小众
Documentation/scsi/bnx2fc.rst

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
