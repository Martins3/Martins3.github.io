# SPDK 基础介绍

## Introduction
spdk 提供
- a user space, polled-mode, asynchronous, lockless NVMe driver.
- a block stack
- NVMe-oF, iSCSI, and vhost servers

- [Message Passing and Concurrency](https://spdk.io/doc/concurrency.html)
- [Submitting I/O to an NVMe Device](https://spdk.io/doc/nvme_spec.html)

## 构建问题

```txt
🤒  ./configure
Using default SPDK env in /home/martins3/core/spdk/lib/env_dpdk
Using default DPDK in /home/martins3/core/spdk/dpdk/build
Configuring ISA-L (logfile: /home/martins3/core/spdk/isa-l/spdk-isal.log)...

Configuration failed
```

cd spdk/isa-l
./configure 得到的结果为:
```txt
configure: error: No modern nasm or yasm found as required. Nasm should be v2.11.01 or later (v2.13 for AVX512) and yasm should be 1.2.0 or later.
```
问题是，我的 nasm 版本是正常的啊

执行
```txt
=====================
All unit tests passed
=====================
WARN: lcov not installed or SPDK built without coverage!
WARN: neither valgrind nor ASAN is enabled!
```

最后在 codex 的协助下解决，default.nix 在 .dotfiles 中，会自动提示如何使用。

## 三种使用场景

提前准备:
```txt
cd /home/martins3/data/spdk
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
```

### ublk
https://spdk.io/doc/ublk.html
```bash
sudo modprobe ublk_drv

# 使用 0 1 两个 core
sudo ./build/bin/spdk_tgt -m 0x3 -L ublk

sudo ./build/bin/spdk_tgt  ublk

sudo ./scripts/rpc.py ublk_create_target
sudo ./scripts/rpc.py bdev_malloc_create -b MallocUblk0 64 4096
sudo ./scripts/rpc.py ublk_start_disk MallocUblk0 0 -q 1 -d 32
sudo ./scripts/rpc.py ublk_get_disks
```

### 构建 nvmf 后端

```bash
sudo ./build/bin/spdk_tgt -m 0x3
sudo ./scripts/rpc.py bdev_malloc_create -b MallocNvmf0 64 4096
sudo ./scripts/rpc.py nvmf_create_transport -t TCP
sudo ./scripts/rpc.py nvmf_create_subsystem nqn.2026-06.io.spdk:alpine -a -s SPDKALPINE0001
sudo ./scripts/rpc.py nvmf_subsystem_add_ns nqn.2026-06.io.spdk:alpine MallocNvmf0
sudo ./scripts/rpc.py nvmf_subsystem_add_listener nqn.2026-06.io.spdk:alpine -t TCP -a 10.0.0.2 -s 4420
sudo ./scripts/rpc.py nvmf_get_subsystems
```

### 配合 qemu 使用

```sh
# 创建 SPDK vhost controller
sudo ./build/bin/spdk_tgt

sudo ./scripts/rpc.py bdev_malloc_create -b MallocVhost0 64 4096
sudo ./scripts/rpc.py vhost_create_blk_controller vhost.0 MallocVhost0
sudo ./scripts/rpc.py vhost_get_controllers
sudo chown martins3:martins3 /home/martins3/data/spdk/vhost.0       


printf '%s\n%s\n%s\n' \
  '{"execute":"qmp_capabilities"}' \
  '{"execute":"chardev-add","arguments":{"id":"spdk_vhost0","backend":{"type":"socket","data":{"addr":{"type":"unix","data":{"path":"/home/martins3/data/spdk/vhost.0"}},"server":false}}}}' \
  '{"execute":"device_add","arguments":{"driver":"vhost-user-blk-pci","id":"spdk_vhost0","chardev":"spdk_vhost0"}}' |
  socat - UNIX-CONNECT:/home/martins3/data/hack/vm/yyds-fs/s/qmp
```

然后到虚拟机中，可以看到:

```txt
🧀  lsblk -o NAME,SIZE,TYPE,MODEL,SERIAL
NAME             SIZE TYPE MODEL         SERIAL
sda               10G disk QEMU HARDDISK virtio-scsi_1
sdb               10G disk QEMU HARDDISK virtio-scsi_2
zram0            7.4G disk
vda               10G disk
vdb              350G disk
├─vdb1             1M part
├─vdb2             1G part
└─vdb3           349G part
  └─fedora-root  349G lvm
vdc               64M disk
```

## 小记
1. 单元测试

./test/unit/unittest.sh
sysctl -a | grep "vm.nr_hugepages"

2. spdk 可以支持小页吗?

2026-06-05 测试，不需要任何特殊操作，就是可以的。

## TODO

2. 为什么必须使用 root 啊
```txt
 ./build/bin/spdk_tgt -m 0x3

[2026-06-05 21:36:51.963149] Starting SPDK v26.05-pre git sha1 4dc62ec60ff8 / DPDK 25.11.0 initialization...
[2026-06-05 21:36:51.963189] [ DPDK EAL parameters: spdk_tgt --no-shconf -l 0-1 --huge-unlink --no-telemetry --log-level=lib.eal:6 --log-level=lib.cryptodev:5 --log-level=lib.power:5 --log-level=user1:6 --iova-mode=pa --base-virtaddr=0x200000000000 --match-allocations --file-prefix=spdk_pid909845 ]
EAL: Cannot use IOVA as 'PA' since physical addresses are not available
[2026-06-05 21:36:51.968099] init.c: 861:spdk_env_init: *ERROR*: Failed to initialize DPDK
[2026-06-05 21:36:51.968110] app.c: 562:app_setup_env: *ERROR*: Unable to initialize SPDK env
[2026-06-05 21:36:51.968116] app.c: 564:app_setup_env: *ERROR*: You may need to run as root
```

1. spdk 正好是 polling 在一个核的两个 hyper thread 上，会存在多少性能退化

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
