# discard / TRIM

## 基本概念

discard（也常叫 TRIM）是块设备向底层存储（SSD、虚拟化镜像、LVM thin pool 等）声明“这些逻辑块当前不再被文件系统使用”的机制。

- 对 SSD：帮助 FTL 回收空闲页，减少写放大和 GC 压力。
- 对虚拟化镜像（qcow2/raw sparse）：让宿主稀疏文件真正释放空间。
- 对 LVM thin pool：让 thin volume 不使用的 extent 回收到 data pool。

Linux 里用户态接口主要是 `fstrim(8)`；内核通过 `BLKDISCARD`、`BLKSECDISCARD`、`BLKZEROOUT` 等 ioctl 把请求发到块设备。

## sysfs 接口

每个块设备的 queue 下暴露 4 个 discard 相关参数：

```txt
/sys/block/<dev>/queue/discard_granularity
/sys/block/<dev>/queue/discard_max_bytes
/sys/block/<dev>/queue/discard_max_hw_bytes
/sys/block/<dev>/queue/discard_zeroes_data
```

含义：

| 文件 | 含义 |
|------|------|
| `discard_granularity` | discard 对齐粒度，单位字节。`0` 表示设备不支持 discard。 |
| `discard_max_bytes` | 单次 discard 请求允许的最大字节数（受软件层限制）。 |
| `discard_max_hw_bytes` | 硬件单次能处理的最大字节数。 |
| `discard_zeroes_data` | 丢弃后读回是否保证为 0。`0` 表示不保证，`1` 表示保证。 |

实现位置：`block/blk-mq-sysfs.c` / `block/blk-sysfs.c`。

## systemd 定时服务

现代发行版（Fedora、openEuler、Ubuntu 等）默认启用 `fstrim.timer`，每周触发一次 `fstrim.service`：

```sh
systemctl status fstrim.timer
systemctl status fstrim.service
```

- `fstrim.timer`：每周执行一次，由 `/usr/lib/systemd/system/fstrim.timer` 定义。
- `fstrim.service`：实际执行 `/usr/sbin/fstrim --listed-in /etc/fstab:/proc/self/mountinfo --verbose --quiet-unsupported`。

如果不需要周期性 trim，可以 `systemctl disable fstrim.timer`；也可以手动执行 `fstrim -v /`。

## 在 yyds-fs 虚拟机上的实测

### 1. 服务状态

```txt
$ systemctl status fstrim.service --no-pager
○ fstrim.service - Discard unused blocks on filesystems from /etc/fstab
     Loaded: loaded (/usr/lib/systemd/system/fstrim.service; static)
    Drop-In: /usr/lib/systemd/system/service.d
             └─10-timeout-abort.conf
     Active: inactive (dead)
TriggeredBy: ● fstrim.timer
       Docs: man:fstrim(8)

$ systemctl status fstrim.timer --no-pager
● fstrim.timer - Discard unused filesystem blocks once a week
     Loaded: loaded (/usr/lib/systemd/system/fstrim.timer; enabled; preset: enabled)
     Active: active (waiting) since Sat 2026-07-11 08:33:03 CST; 1min 8s ago
    Trigger: Mon 2026-07-13 00:06:10 CST; 1 day 15h left
   Triggers: ● fstrim.service
       Docs: man:fstrim(8)
```

说明 `fstrim.timer` 已启用，每周触发一次 `fstrim.service`。

### 2. lsblk 看 discard 能力

```txt
$ lsblk -D
NAME            DISC-ALN DISC-GRAN DISC-MAX DISC-ZERO
sda                    0        0B       0B         0
sdb                    0        4K       1G         0
sdc                    0        4K       1G         0
vda                    0      512B       2G         0
vdb                    0      512B       2G         0
|-vdb1                 0      512B       2G         0
|-vdb2                 0      512B       2G         0
`-vdb3                 0      512B       2G         0
  `-fedora-root        0      512B       2G         0
zram0                  0        4K       2T         0
nullb0                 0        0B       0B         0
```

- `sda` 和 `nullb0` 不支持 discard（`DISC-GRAN` 为 `0B`）。
- `sdb`/`sdc` 是普通 SCSI/SATA 盘，粒度 4K，单次最大 1G。
- `vda`/`vdb` 是 virtio-blk，粒度 512B，单次最大 2G。
- `zram0` 也支持 discard，粒度 4K。


## 验证 fstrim 真的释放宿主机空间

在 `yyds-fs` 虚拟机上通过 hotplug 新增一块 10G qcow2 稀疏盘，验证 discard 透传效果。

### hotplug 独立盘验证

宿主机创建稀疏 qcow2：

```sh
qemu-img create -f qcow2 /home/martins3/data/hack/vm/yyds-fs/img/test-discard.qcow2 10G
```

QEMU monitor 中热插拔（只给 `discard=unmap` 即可）：

```sh
drive_add 0 file=...,format=qcow2,if=none,id=test-discard,discard=unmap
device_add virtio-blk-pci,drive=test-discard,id=test-discard
```

guest 中格式化、写数据、删除、trim：

```sh
mkfs.ext4 /dev/vdd
mount /dev/vdd /mnt/test-discard

# 写 2G 数据
dd if=/dev/zero of=/mnt/test-discard/bigfile bs=1M count=2000

# 删除并 trim
rm /mnt/test-discard/bigfile
sync
fstrim -v /mnt/test-discard
# /mnt/test-discard: 9.7 GiB (10461900800 bytes) trimmed
```

宿主机文件变化：

```txt
写数据后：   1.7G actual / 1.8G apparent
删除+fstrim：9.1M actual / 1.8G apparent
```

说明：只配 `discard=unmap` 就能让宿主机 qcow2 真正释放空间。`aio=native,cache.direct=on` 不是 punch hole 的必要条件，但生产环境常一起用（libvirt 默认也这样配），主要是为了保证 I/O 直写路径的性能和一致性。

### 根文件系统上的验证

`/` 位于 LVM `/dev/mapper/fedora-root` → `/dev/vdb3`。直接在根目录创建大文件测试：

```sh
# 写 10G 文件
dd if=/dev/zero of=/bigfile bs=1M count=10000

# 删除并 trim
rm /bigfile
sync
fstrim -v /
# /: 349 GiB (374731702272 bytes) trimmed
```

宿主机 `boot1` qcow2 占用变化：

```txt
写文件前：  76G actual / 148G apparent
写文件后：  86G actual / 148G apparent
删除+fstrim：76G actual / 148G apparent
```

这说明即使不修改 `/etc/lvm/lvm.conf` 的 `issue_discards`，文件系统层的 `fstrim` 也会通过 device-mapper 透传到 PV，再经 `discard=unmap` 让宿主机 qcow2 真正释放空间。

已修改 `collei/scripts/collei.py`，为所有 `-drive` 后端统一加上 `discard=unmap`（boot、virtio-scsi、virtio-blk、nvme、sata）。`yyds-fs` 重启后生成的 `cmd.sh` 已包含这些参数。

## 虚拟机 / QEMU 下的注意事项

- virtio-blk：QEMU 启动参数需要 `discard=unmap`，并且磁盘格式（如 qcow2）支持 unmap。
- virtio-scsi：`discard=unmap` 同样可以开启 TRIM 透传。
- 如果 `cat /sys/block/vd*/queue/discard_granularity` 为 `0`，但宿主角色的底层盘支持 TRIM，通常是因为 QEMU 没配 discard 透传。

即使底层块设备支持 discard，如果挂载点对应的层（如 dm-crypt 没开 `allow_discards`、
LVM thin 没启用 `issue_discards`、QEMU 没开启 `discard=unmap`）， trim 也可能无法真正透传到底层。

## 常用命令

```sh
# 查看所有块设备的 discard 能力
lsblk -D

# 查看某个盘的 discard 参数
cat /sys/block/sdc/queue/discard_*

# 手动 trim 根分区
sudo fstrim -v /

#  trim 所有已挂载文件系统
sudo fstrim -av

# 查看/启用每周 trim 定时器
systemctl status fstrim.timer
systemctl enable --now fstrim.timer

# 
fstrim --listed-in /etc/fstab:/proc/self/mountinfo --verbose --quiet-unsupported
```

## 参考

- `man fstrim(8)`
- `Documentation/block/queue-sysfs.rst`
- `block/blk-mq-sysfs.c`, `block/blk-sysfs.c`
- https://askubuntu.com/questions/1492995/how-to-know-if-my-nvme-ssd-needs-trim

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
