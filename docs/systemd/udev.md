# udev 机制

## 使用的经典案例

### 网卡重命名

```txt
for r in `rpm -qa |grep mlnx`;do rpm -ql $r |grep udev ;done

/lib/udev/mlnx_bf_udev
/lib/udev/auxdev-sf-netdev-rename
/lib/udev/rules.d/83-mlnx-sf-name.rules
/lib/udev/rules.d/90-ib.rules
/lib/udev/sf-rep-netdev-rename
/usr/lib/udev/rdma_rename
/usr/lib/udev/rules.d
/usr/lib/udev/rules.d/60-rdma-ndd.rules
/usr/lib/udev/rules.d/60-rdma-persistent-naming.rules
/usr/lib/udev/rules.d/75-rdma-description.rules
/usr/lib/udev/rules.d/90-rdma-umad.rules
/usr/share/doc/rdma-core/udev.md
```

中可以找到类似这样的写法:

```txt
ACTION=="add", SUBSYSTEM=="infiniband", PROGRAM="rdma_rename %k NAME_GUID"
```

### 权限修改
例如将 /dev/kvm 放到 kvm group 中去

### kernel module 自动添加

原因在 /usr/lib/udev/rules.d/80-drivers.rules 中的这个通用规则中:
```txt
ENV{MODALIAS}=="?*", RUN{builtin}+="kmod load"
```

当系统检测到一个新设备，且该设备携带了非空的 MODALIAS 时，udev 会自动执行：
```bash
  kmod load
```
kmod 会拿这个 MODALIAS 去 /lib/modules/$(uname -r)/modules.alias 里查找匹配项，然后调用 modprobe 加载对应的驱动模块。

所以，我们再回到那个问题， kvm 为什么总是可以自动加载?
执行 modinfo kvm_intel 显然可以看到其 alias 关联的内容:
```txt
$ modinfo kvm_intel

filename:       /lib/modules/7.0.8-200.fc44.x86_64/kernel/arch/x86/kvm/kvm-intel.ko.xz
license:        GPL
description:    KVM support for VMX (Intel VT-x) extensions
author:         Qumranet
alias:          cpu:type:x86,ven*fam*mod*:feature:*0085*
depends:        kvm
```
同时，是可以观察一个设备的 alias 的:
```sh
find /sys -name "modalias"
```


### /dev 目录的构建

### udev 会监控盘的读写
这个问题比较细节:

打开关闭 /dev/sda (HDD 盘) 的操作都会让 udevd 来进行 重新获取 disk capacity 。

为什么要做这个机制:
如果盘是有问题的，那么 udev 会来 这里的 blk_execute_rq
会执行失败，它会将盘的容量设置为 0 。接下来，所有的 process 都可以 open ，但是
write 立刻触发容量为 0 的错误并且导致 disk 关闭

实现原理:

1. 通过 inotify 来监听文件是否被打开

通过打开额外的日志来验证
```txt
sudo udevadm control --log-level=debug
sudo journalctl -u systemd-udevd -f
```
```txt
Jun 06 10:44:25 localhost.localdomain systemd-udevd[1159]: sda: Received inotify event of watch handle 154.
Jun 06 10:44:25 localhost.localdomain systemd-udevd[1159]: Successfully forked off '(udev-synth)' as PID 1023046.
```

2. 触发 uevent + netlink 机制的
```txt
+ sudo bpftrace -e 'kprobe:netlink_broadcast { @[curtask->comm] = count() } interval:s:1000 { exit(); }'
@[udev-synth]: 16

sudo bpftrace -e 'kprobe:netlink_broadcast { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
@[
        netlink_broadcast+5
        kobject_uevent_net_broadcast+191
        kobject_uevent_env+906
        kobject_synth_uevent+316
        uevent_store+32
        kernfs_fop_write_iter+369
        vfs_write+641
        ksys_write+123
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 14
```

3. 执行容量检测
```txt
[<0>] blk_execute_rq+0x147/0x220
[<0>] scsi_execute_cmd+0x15c/0x2d0
[<0>] read_capacity_10+0x104/0x230
[<0>] sd_revalidate_disk.isra.0+0x1441/0x2860
[<0>] sd_open+0x13d/0x1d0
[<0>] blkdev_get_whole+0x23/0x90
[<0>] blkdev_get_by_dev.part.0+0x174/0x320
[<0>] disk_scan_partitions+0x69/0xe0
[<0>] blkdev_ioctl+0x10a/0x270
[<0>] __x64_sys_ioctl+0x94/0xd0
[<0>] do_syscall_64+0x3b/0x90
[<0>] entry_SYSCALL_64_after_hwframe+0x6e/0xd8
```

也就是:
```txt
进程打开 /dev/sda 然后关闭
           |
           v
   内核 inotify 子系统检测到 CLOSE
           |
           v
   systemd-udevd 读取 inotify 事件（read fd）
           |
           v
   systemd-udevd fork 出 udev-synth
           |
           v
   udev-synth 写 /sys/.../sda/uevent (change)
           |
           v
   内核广播 uevent
           |
           v
   udev-worker 处理规则 -> 调用 scsi_id/blkid -> blk_execute_rq
```

## 参考文档
- https://unix.stackexchange.com/questions/550037/udev-and-uevent-question
- https://events19.linuxfoundation.org/wp-content/uploads/2017/12/Introduction-to-Linux-Kernel-Driver-Programming-Michael-Opdenacker-Bootlin-.pdf
- https://lwn.net/Articles/645810/
- https://lwn.net/Articles/646514/
- https://askubuntu.com/questions/297412/how-do-i-make-udev-rules-work
- https://opensource.com/article/18/11/udev
- http://www.wowotech.net/device_model/uevent.html
- https://documentation.suse.com/sles/12-SP5/html/SLES-all/cha-udev.html
- https://www.reactivated.net/writing_udev_rules.html
- https://unix.stackexchange.com/questions/97676/how-to-find-the-driver-module-associated-with-a-device-on-linux
- https://unix.stackexchange.com/questions/248494/how-to-find-the-driver-module-associated-with-sata-device-on-linux
- https://lwn.net/Articles/740455/
- https://duasynt.com/blog/linux-kernel-module-autoloading
- https://unix.stackexchange.com/questions/532086/how-can-i-write-a-udev-rule-that-will-apply-to-a-pci-device-at-boot-time

## 其他问题
https://wiki.archlinux.org/title/Modalias

## 👷 devtmpfs 的作用 
derivers/base/devtmpfs.c

```txt
devtmpfs on /dev type devtmpfs (rw,nosuid,size=65474912k,nr_inodes=16368728,mode=755,inode64)
tmpfs on /dev/shm type tmpfs (rw,nosuid,nodev,inode64,usrquota)
```

- https://unix.stackexchange.com/questions/77933/using-devtmpfs-for-dev
- https://mp.weixin.qq.com/s/SEUGeRtAFH5LrFPbloN9LA : /dev/by-uuid
  目录如何构建的

devtmpfs_create_node 是什么时候调用

## 经典命令

1. udevadm monitor --property
2. udevadm test /dev/kvm
3. udevadm info 查询 udev 数据库和 sysfs 中的设备信息
4. udevadm control --reload-rules

## 通知的链路 : uevent + netlink

### 测试实验

sudo modprobe dummy sudo modprobe -r dummy

```txt
sudo bpftrace -e 'kprobe:netlink_broadcast { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
@[
        netlink_broadcast+5
        kobject_uevent_net_broadcast+191
        kobject_uevent_env+906
        do_init_module+188
        init_module_from_file+200
        idempotent_init_module+574
        __x64_sys_finit_module+127
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 8
@[
        netlink_broadcast+5
        kobject_uevent_net_broadcast+191
        kobject_uevent_env+906
        __kobject_del+96
        kobject_cleanup+177
        mod_sysfs_teardown+375
        free_module+27
        __do_sys_delete_module.isra.0+672
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 8
```

```txt
udevadm monitor --property
monitor will print the received events for:
UDEV - the event which udev sends out after rule processing
KERNEL - the kernel uevent

KERNEL[832099.357555] add      /module/dummy (module)
ACTION=add
DEVPATH=/module/dummy
SUBSYSTEM=module
SEQNUM=69035

UDEV  [832099.360212] add      /module/dummy (module)
ACTION=add
DEVPATH=/module/dummy
SUBSYSTEM=module
SEQNUM=69035
USEC_INITIALIZED=832099357572
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
