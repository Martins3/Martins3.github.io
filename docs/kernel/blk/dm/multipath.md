# multipath

## device mapper multipath
multipath 聚合的前提是：这些路径指向同一个存储对象。 Linux 通过 WWID（World Wide Identifier）来识别：

### 双路径测试盘
两个独立 virtio-scsi 控制器 (各带 iothread `mp_io1`/`mp_io2`), scsi-hd 用相同 `serial=MULTIPATH`,
guest 内 WWID 相同,multipathd 聚合为一个 mpath 设备

guest 内使用:

```bash
sudo mpathconf --enable
sudo systemctl start multipathd
sudo multipath        # 首次需要手动触发建图
sudo multipath -ll    # 验证 mpatha 聚合了两个 virtio-scsi 的两条路径
```

```txt
🧀  sudo multipath -ll
mpatha (0QEMU_QEMU_HARDDISK_MULTIPATH) dm-1 QEMU,QEMU HARDDISK
size=25G features='0' hwhandler='0' wp=rw
|-+- policy='service-time 0' prio=1 status=active
| `- 2:0:0:0 sdd 8:48 active ready running
`-+- policy='service-time 0' prio=1 status=enabled
  `- 3:0:0:0 sde 8:64 active ready running

🧀  sudo multipathd show paths format "%d %D %t %T %s"
dev dev_t dm_st  chk_st vend/prod/rev
sdb 8:16  undef  undef  QEMU,QEMU HARDDISK,2.5+
sdc 8:32  undef  undef  QEMU,QEMU HARDDISK,2.5+
sdd 8:48  active ready  QEMU,QEMU HARDDISK,2.5+
sde 8:64  active ready  QEMU,QEMU HARDDISK,2.5+
sda 8:0   undef  undef  Linux,scsi_debug,0191
```

故障注入(host 侧):`SIGSTOP`/`SIGCONT` 对应路径的 qemu-nbd 进程即可单独卡死/恢复该路径:

```bash
kill -STOP $(pgrep -f 'qemu-nbd.*mpath1.nbd')   # 卡死路径 1
kill -CONT $(pgrep -f 'qemu-nbd.*mpath1.nbd')   # 恢复

kill -STOP $(pgrep -f 'qemu-nbd.*mpath2.nbd')   # 卡死路径 2
kill -CONT $(pgrep -f 'qemu-nbd.*mpath2.nbd')   # 恢复
```

```sh
# guest 内运行：持续对 mpatha 做 direct 写，每个 dd 256MiB，记录每轮耗时
#!/usr/bin/env bash
set -u
i=0
while true; do
	i=$((i + 1))
	start=$(date +%s)
	dd if=/dev/zero of=/dev/mapper/mpatha bs=1M count=256 oflag=direct status=none 2>/dev/null
	rc=$?
	end=$(date +%s)
	echo "$(date '+%T') round=$i rc=$rc elapsed=$((end - start))s"
	sleep 1
done
```

然后就会发现 io 是一直都卡在这里:
```txt
# sudo cat /proc/5708/stack
[sudo] password for martins3:
[<0>] bio_await+0x94/0xe0
[<0>] submit_bio_wait+0x1a/0x30
[<0>] __blkdev_direct_IO_simple+0x154/0x280
[<0>] blkdev_write_iter+0x228/0x350
[<0>] vfs_write+0x24f/0x5c0
[<0>] ksys_write+0x7b/0xf0
[<0>] do_syscall_64+0xcf/0x630
[<0>] entry_SYSCALL_64_after_hwframe+0x76/0x7e
```

执行 sudo multipath -ll 同样卡在这里了:
```txt
# sudo cat /proc/10128/stack
[sudo] password for martins3:
[<0>] do_read_cache_folio+0x29f/0x3d0
[<0>] scsi_bios_ptable+0x28/0xa0
[<0>] scsi_partsize+0x1f/0x140
[<0>] scsicam_bios_param+0x1b/0x170
[<0>] sd_getgeo+0xc3/0xd0
[<0>] blkdev_ioctl+0x186/0x1320
[<0>] __x64_sys_ioctl+0x4f7/0xa10
[<0>] do_syscall_64+0xcf/0x630
[<0>] entry_SYSCALL_64_after_hwframe+0x76/0x7e
```

不知道为什么 device mapper 关于 io 没有报错的。


## nvme multipath

NVMe 支持 multipath，核心原因是：同一个存储 namespace 可能通过多条独立路径访问。
Multipath 用来把这些路径合并成一个可靠的块设备，并负责故障切换或负载均衡。

- https://spdk.io/doc/nvme_multipath.html : 这个讲的很深入
- https://www.ibm.com/docs/en/flashsystem-7x00/8.4.x?topic=nhtrlos-multipath-configuration-fc-nvme-hosts-1

### 实验

qemu 启动参数:
```txt
	-drive file=/home/martins3/data/hack/vm/yyds-collei/img/nvme1,format=qcow2,if=none,id=nvme_mpath,discard=unmap \
	-device nvme-subsys,id=nvme-subsys-0,nqn=subsys0 \
	-device nvme,serial=deadbeef,subsys=nvme-subsys-0,id=nc1 \
	-device nvme,serial=deadbeef,subsys=nvme-subsys-0,id=nc2 \
	-device nvme-ns,drive=nvme_mpath,bus=nc1,nsid=1,shared=on \
```

```txt
lrwxrwxrwx - root 19 Jul 15:17 nvme0c0n1 -> ../devices/pci0000:00/0000:00:04.0/nvme/nvme0/nvme0c0n1
lrwxrwxrwx - root 19 Jul 15:17 nvme0c1n1 -> ../devices/pci0000:00/0000:00:05.0/nvme/nvme1/nvme0c1n1
lrwxrwxrwx - root 19 Jul 15:17 nvme0n1 -> ../devices/virtual/nvme-subsystem/nvme-subsys0/nvme0n1
```
所以，也就是 qemu 内部模拟了多个 channel ，最后 io 还是落入到一个盘上的。

io 策略
```txt
🧀  cat /sys/class/nvme-subsystem/nvme-subsys0/iopolicy
numa
```

```txt
@[
    nvme_ns_head_submit_bio+5
    __submit_bio+132
    submit_bio_noacct_nocheck+345
    blkdev_direct_IO.part.0+575
    blkdev_write_iter+427
    io_write+290
    io_issue_sqe+96
    io_submit_sqes+507
    __do_sys_io_uring_enter+1471
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 1365299
```


```txt
  nvme_subsystem
    ├── ctrl A
    │    └── nvme_ns A ──┐
    ├── ctrl B           │
    │    └── nvme_ns B ──┼── nvme_ns_head
    └── ctrl C           │       └── 聚合 gendisk: nvmeXnY
         └── nvme_ns C ──┘
```

- nvme_subsystem：同一个 Subsystem NQN。
- nvme_ns：某个 controller 看到的一条 namespace path。
- nvme_ns_head：同一 namespace 在 subsystem 内的聚合对象。
- head->list：这个 namespace 的所有 path。
- head->disk：用户实际使用的聚合块设备。
- head->current_path[node]：按 NUMA node 缓存的当前路径。


 策略           行为                                      关键特点
━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 numa           选择距离当前 CPU NUMA node 最近的 path    默认策略；按 node 缓存路径
─────────────  ────────────────────────────────────────  ───────────────────────────────────────────────
 round-robin    在同一 ANA 优先级的路径间轮转             不会为了轮转而从 optimized 切到 non-optimized
─────────────  ────────────────────────────────────────  ───────────────────────────────────────────────
 queue-depth    选择活动请求数最少的 controller           统计的是 controller 级 in-flight 数量

## 其他类型的 multipath

1. mptcp : 多个网络路径的 tcp 来加速带宽
2. multifd qemu migration : 多个 fd 来加速传输

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
