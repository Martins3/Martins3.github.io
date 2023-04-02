# libvirt

- https://wiki.libvirt.org/page/Main_Page
- https://libvirt.org/drvqemu.html
- https://libvirt.org/manpages/virsh.html

## 常见使用路径
1. /var/run/libvirt/qemu
  - 存储 uuid.xml 和 uuid.pid
2. /etc/libvirt/qemu/
  - 配置 uuid.xml

## 使用 virsh 安装系统
- https://unix.stackexchange.com/questions/309788/how-to-create-a-vm-from-scratch-with-virsh

## Ctrl+] 从 virsh console 中退出
参考 https://superuser.com/questions/637669/how-to-exit-a-virsh-console-connection

## 环境搭建: https://wiki.libvirt.org/page/UbuntuKVMWalkthrough

- [ ] python-virtinst : 是做什么的?

## 正式操作: https://wiki.libvirt.org/page/QEMUSwitchToLibvirt


## what's the domiain in the libvirt

## patch
- https://libvirt.org/intro.html : not found

## TODO

> 将 source code 的 reading 放到之后

- 使用 Python 开发的吗?

## 使用 libvirt
两个配合使用:
- https://wiki.libvirt.org/page/UbuntuKVMWalkthrough
- https://www.technicalsourcery.net/posts/nixos-in-libvirt/

nixos 专属内容:
- Check if `/dev/kvm` exists, and check the contents of the file opened with `virsh edit <your vm name>`.
This should list /run/libvirt/nix-emulators/qemu-kvm in the <emulator> tag. If both are the case, the VM should be KVM accelerated.

## 重新创建虚拟机的
```sh
kvm : no hardware support
```

## 使用 virsh 来操作 hmp
- https://gist.github.com/orimanabu/815fc2453966f50f5d5281ea58b0058e

## 使用 virsh 操作 qmp

## 使用 virsh 安装系统
```sh
virt-install  \
  --name martins3 \
  --memory 1024             \
  --vcpus=2,maxvcpus=4      \
  --cpu host                \
  --disk size=2,format=qcow2  \
  --network user            \
  --virt-type kvm \
  --cdrom $HOME/Downloads/arch-linux_install.iso
```

## 问题
- 什么叫做 storage pool ?

## 研究下，如何让 libvirt 来替代现在的操作 qemu 的管理
- 打开 memory reporting 机制；
- 开启多个虚拟机。
- 使用 Rust 管理代码。
- 理解其中的网络部署。
- 如何让 virsh 构建一个网络，让本地构建虚拟机迁移。

- 在 ubuntu 上测试吧?


## virsh 观测 memory

1. [root@node-67-81 18:02:04 ~]$virsh dommemstat edab2f44-081c-4031-b4a6-ad064789ad67

2. 使用 qmp
```txt
domain=0de88a46-9331-492e-b978-7c313f13bc6b
virsh qemu-monitor-command $domain  '{"execute": "query-status"}' --pretty
virsh qemu-monitor-command $domain --hmp 'info balloon'
virsh qemu-monitor-command $domain '{ "execute": "qom-list", "arguments": { "path": "/machine/peripheral" } }'
virsh qemu-monitor-command $domain '{ "execute": "qom-set", "arguments": { "path": "/machine/peripheral/balloon0", "property": "guest-stats-polling-interval", "value": 2 } }'
virsh qemu-monitor-command $domain '{ "execute": "qom-get", "arguments": { "path": "/machine/peripheral/balloon0", "property": "guest-stats" } }'
virsh qemu-monitor-command $domain --hmp 'info balloon'
virsh qemu-monitor-command $domain --hmp 'balloon 4000'
```


| 作用        | 说明                               |
|-------------|------------------------------------|
| actual      | QEMU 参数配置的内存                |
| swap_in     | 累加数值                           |
| swap_out    |                                    |
| major_fault |                                    |
| minor_fault |                                    |
| unused      | MemFree                            |
| available   | MemTotal                           |
| usable      | MemAvailable                       |
| last_update |                                    |
| disk_caches | Buffers + Cached + swapcache       |
| rss         | /proc/$qemu_pid/status \| grep RSS |

理解下各个字段的含义
```txt
{
  "return": {
    "stats": {
      "stat-swap-out": 0,
      "stat-available-memory": 2083450880, # 2034620
      "stat-free-memory": 999878656, # 976444
      "stat-minor-faults": 482351294,
      "stat-major-faults": 5083,
      "stat-total-memory": 3241218048,
      "stat-swap-in": 0,
      "stat-disk-caches": 1242247168 # 1213132
    },
    "last-update": 1676350443
  },
  "id": "libvirt-23424"
}

actual 4194304
swap_in 0
swap_out 0
major_fault 5083
minor_fault 482351294
unused 976444
available 3165252
usable 2034620
last_update 1676350443
disk_caches 1213132
rss 67288

# 如果没有 balloon 的数据
actual 8388608
last_update 1676945856
rss 8462616
```

```txt
MemTotal:        3165252 kB
MemFree:          980324 kB
MemAvailable:    2032100 kB
Buffers:          312624 kB
Cached:           895964 kB
SwapCached:            0 kB
Active:           559984 kB
Inactive:         981104 kB
Active(anon):       1212 kB
Inactive(anon):   332704 kB
Active(file):     558772 kB
Inactive(file):   648400 kB
Unevictable:       29272 kB
Mlocked:           27736 kB
SwapTotal:       4194300 kB
SwapFree:        4194300 kB
Dirty:               276 kB
Writeback:             0 kB
AnonPages:        361852 kB
Mapped:           338476 kB
Shmem:              2852 kB
KReclaimable:      95396 kB
Slab:             217388 kB
SReclaimable:      95396 kB
SUnreclaim:       121992 kB
KernelStack:       10912 kB
PageTables:         5840 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:     5776924 kB
Committed_AS:    3076652 kB
VmallocTotal:   34359738367 kB
VmallocUsed:       89020 kB
VmallocChunk:          0 kB
Percpu:           262080 kB
HardwareCorrupted:     0 kB
AnonHugePages:         0 kB
ShmemHugePages:        0 kB
ShmemPmdMapped:        0 kB
FileHugePages:         0 kB
FilePmdMapped:         0 kB
HugePages_Total:       0
HugePages_Free:        0
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
Hugetlb:               0 kB
DirectMap4k:     1498984 kB
DirectMap2M:     2695168 kB
DirectMap1G:     2097152 kB
```

### 从代码上确认一下

> 观察 libvirt 的代码，cmdDomMemStat 和 qemuMonitorJSONGetMemoryStats 中 VIR_DOMAIN_MEMORY_STAT_USABLE 这个宏的使用，那么就是 avaliable 的含义了

```c
    for (i = 0; i < nr_stats; i++) {
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_SWAP_IN)
            vshPrint(ctl, "swap_in %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_SWAP_OUT)
            vshPrint(ctl, "swap_out %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_MAJOR_FAULT)
            vshPrint(ctl, "major_fault %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_MINOR_FAULT)
            vshPrint(ctl, "minor_fault %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_UNUSED)
            vshPrint(ctl, "unused %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE)
            vshPrint(ctl, "available %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_USABLE)
            vshPrint(ctl, "usable %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON)
            vshPrint(ctl, "actual %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_RSS)
            vshPrint(ctl, "rss %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_LAST_UPDATE)
            vshPrint(ctl, "last_update %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_DISK_CACHES)
            vshPrint(ctl, "disk_caches %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_HUGETLB_PGALLOC)
            vshPrint(ctl, "hugetlb_pgalloc %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_HUGETLB_PGFAIL)
            vshPrint(ctl, "hugetlb_pgfail %llu\n", stats[i].val);
    }
```

```c
    GET_BALLOON_STATS(statsdata, "stat-swap-in",
                      VIR_DOMAIN_MEMORY_STAT_SWAP_IN, 1024);
    GET_BALLOON_STATS(statsdata, "stat-swap-out",
                      VIR_DOMAIN_MEMORY_STAT_SWAP_OUT, 1024);
    GET_BALLOON_STATS(statsdata, "stat-major-faults",
                      VIR_DOMAIN_MEMORY_STAT_MAJOR_FAULT, 1);
    GET_BALLOON_STATS(statsdata, "stat-minor-faults",
                      VIR_DOMAIN_MEMORY_STAT_MINOR_FAULT, 1);
    GET_BALLOON_STATS(statsdata, "stat-free-memory",
                      VIR_DOMAIN_MEMORY_STAT_UNUSED, 1024);
    GET_BALLOON_STATS(statsdata, "stat-total-memory",
                      VIR_DOMAIN_MEMORY_STAT_AVAILABLE, 1024);
    GET_BALLOON_STATS(statsdata, "stat-available-memory",
                      VIR_DOMAIN_MEMORY_STAT_USABLE, 1024);
    GET_BALLOON_STATS(data, "last-update",
                      VIR_DOMAIN_MEMORY_STAT_LAST_UPDATE, 1);
    GET_BALLOON_STATS(statsdata, "stat-disk-caches",
                      VIR_DOMAIN_MEMORY_STAT_DISK_CACHES, 1024);
    GET_BALLOON_STATS(statsdata, "stat-htlb-pgalloc",
                      VIR_DOMAIN_MEMORY_STAT_HUGETLB_PGALLOC, 1);
    GET_BALLOON_STATS(statsdata, "stat-htlb-pgfail",
                      VIR_DOMAIN_MEMORY_STAT_HUGETLB_PGFAIL, 1);
```


## vish dommemstat 是如何实现的
snoopexec 中
```txt
virtqemud        659083  2800      0 /home/martins3/core/libvirt/build/src/virtqemud --timeout=120
qemu-system-x86  659104  659083    0 /run/current-system/sw/bin/qemu-system-x86_64 -S -no-user-config -nodefaults -nographic -machine none,accel=kvm:tcg -qmp unix:/home/martins3/.config/libvirt/qemu/lib/qmp-LEY2Z1/qmp.monitor,server=on,wait=off -pidfile /home/martins3/.config/libvirt/qemu/lib/qmp-LEY2Z1/qmp.pid -daemonize
.qemu-system-x8  659104  659083    0 /nix/store/lfldwcwazg4qpnb9nwps4nvlxab6zkmk-qemu-7.1.0/bin/.qemu-system-x86_64-wrapped -S -no-user-config -nodefaults -nographic -machine none,accel=kvm:tcg -qmp unix:/home/martins3/.config/libvirt/qemu/lib/qmp-LEY2Z1/qmp.monitor,server=on,wait=off -pidfile /home/martins3/.config/libvirt/qemu/lib/qmp-LEY2Z1/qmp.pid -daemonize
qemu-system-x86  659115  659083    0 /run/current-system/sw/bin/qemu-system-x86_64 -S -no-user-config -nodefaults -nographic -machine none,accel=tcg -qmp unix:/home/martins3/.config/libvirt/qemu/lib/qmp-99VWZ1/qmp.monitor,server=on,wait=off -pidfile /home/martins3/.config/libvirt/qemu/lib/qmp-99VWZ1/qmp.pid -daemonize
.qemu-system-x8  659115  659083    0 /nix/store/lfldwcwazg4qpnb9nwps4nvlxab6zkmk-qemu-7.1.0/bin/.qemu-system-x86_64-wrapped -S -no-user-config -nodefaults -nographic -machine none,accel=tcg -qmp unix:/home/martins3/.config/libvirt/qemu/lib/qmp-99VWZ1/qmp.monitor,server=on,wait=off -pidfile /home/martins3/.config/libvirt/qemu/lib/qmp-99VWZ1/qmp.pid -daemonize
swtpm_setup      659127  659083    0 /home/martins3/.nix-profile/bin/swtpm_setup --print-capabilities
swtpm            659128  659127    0 /home/martins3/.nix-profile/bin/swtpm socket
swtpm            659129  659127    0 /home/martins3/.nix-profile/bin/swtpm socket
```
实际上调用 remoteConnectGetDomainCapabilities

## 如何理解 domain 的概念

virsh domcapabilities
virsh dom

## 代码分析

### remote 机制
本来以为 virsh domcapabilitie 调用到 virConnectGetDomainCapabilities
的时候，实际上是会去查询 qemuConnectGetDomainCapabilities 的，
但是使用 bcc 工具 execsnoop 发现，实际上，libvirt 会启动一个 QEMU ，
然后和 qemu 通信，使用 query-cpu-definitions 来查询的。

- libvirt/src/remote/remote_driver.c

- [ ] libvirt/src/remote 和 libvirt/src/qemu 是什么关系
- [ ] 为什么不去直接使用 libvirt/src/qemu
- [ ] 为什么 dom 是和 libvirt 绑定的

为什么 ccls 没有索引 remoteConnectGetDomainCapabilities

## 如何使用 numad

src/qemu/qemu_process.c:qemuProcessPrepareDomainNUMAPlacement 中确定 qemu 所在的 numa 节点

## 热迁移也可以使用的
- virsh domjobinfo domain_id
- virsh getdirtyrate domain_id
