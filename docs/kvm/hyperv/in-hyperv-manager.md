# hyperv 中运行 Linux

## linux 运行在 hyperv 的时候，会加载的驱动
```txt
hv_netvsc             135168  0
hv_utils               57344  0
hv_balloon             49152  0
hv_sock                20480  0
vsock                  65536  6 vmw_vsock_virtio_transport_common,vsock_loopback,hv_sock,vmw_vsock_vmci_transport
hv_storvsc             32768  3
scsi_transport_fc     126976  1 hv_storvsc
hv_vmbus              196608  8 hv_balloon,hv_utils,hv_netvsc,hid_hyperv,hv_storvsc,hyperv_keyboard,hyperv_drm,hv_sock
```

## 也是如此，不使用任何 pci 相关的东西
```txt
🧀  ls -la /sys/block
lrwxrwxrwx@ - root 22 Sep 02:11 dm-0 -> ../devices/virtual/block/dm-0
lrwxrwxrwx@ - root 22 Sep 02:11 sda -> ../devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/VMBUS:00/964ec074-f0fb-4420-8762-1ecff2524a1a/host0/target0:0:0/0:0:0:0/block/sda
lrwxrwxrwx@ - root 22 Sep 02:11 sr0 -> ../devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/VMBUS:00/964ec074-f0fb-4420-8762-1ecff2524a1a/host0/target0:0:0/0:0:0:1/block/sr0
lrwxrwxrwx@ - root 22 Sep 02:11 zram0 -> ../devices/virtual/block/zram0
~ 🪟
🧀  lspci
```

不过如何理解?
LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00

用这个看看一个 scsi 控制器，有多个盘的操作吧。

## 网络


```txt
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host noprefixroute
       valid_lft forever preferred_lft forever
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000
    link/ether 00:15:5d:00:08:02 brd ff:ff:ff:ff:ff:ff
    altname enx00155d000802
    inet 172.23.69.168/20 brd 172.23.79.255 scope global dynamic noprefixroute eth0
       valid_lft 73128sec preferred_lft 73128sec
    inet6 fe80::215:5dff:fe00:802/64 scope link noprefixroute
       valid_lft forever preferred_lft forever
```

172.23.69.168 物理机中可以 Ping 通，虚拟机中也可以 ping 通

虚拟机中可以直接 ping 通另外一个物理机
```txt
 🪟
🧀  ping 10.0.0.8
PING 10.0.0.8 (10.0.0.8): 56 data bytes
64 bytes from 10.0.0.8: icmp_seq=0 ttl=51 time=0.314 ms
64 bytes from 10.0.0.8: icmp_seq=1 ttl=52 time=0.485 ms
^C--- 10.0.0.8 ping statistics ---
2 packets transmitted, 2 packets received, 0% packet loss
round-trip min/avg/max/stddev = 0.314/0.400/0.485/0.086 ms
```

但是另外的物理机无法 ping 通这个虚拟机。

也就是这个网络配置介于内核

## hyper-v manager 默认 page cache 为总内存使用量的 20%

这个配置太保守了，导致了几个问题

### 如果长期不用， 总内存太小了
```txt
[ 4070.558779] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 4370.564748] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 4670.562097] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 4970.564466] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 5270.560955] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 5570.563192] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 5870.552621] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 6170.564015] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 6470.552463] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 6770.552681] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 7070.554138] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 7370.567150] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 7670.566734] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 7970.566297] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 8270.554093] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 8570.558998] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 8870.565398] hv_balloon: Balloon request will be partially fulfilled. Balloon floor reached.
[ 9170.564023] hv_balloon: Balloon request will be partially fulfilled. Not enough memory.
[ 9267.575916] __vm_enough_memory: pid: 3972, comm: git, bytes: 881713152 not enough memory for the allocation
[ 9267.575922] __vm_enough_memory: pid: 3972, comm: git, bytes: 881750016 not enough memory for the allocation
[ 9267.575924] __vm_enough_memory: pid: 3972, comm: git, bytes: 881844224 not enough memory for the allocation
[ 9353.520343] TCP: eth0: Driver has suspect GRO implementation, TCP performance may be compromised.
```

### page cache 有时候是非常重要的
当时只是 git pull 一下，然后 git 分配内存直接被干掉了
可能是 git 直接 mmap 了一个几百兆的内存，这个虚拟机分配的是 16G ，但是好长时间
没用，结果当时内存总量就只有 600M 了，300M 被使用，300M 是 page cache 。

把 memory overcommit 关掉，结果 git pull 下载完成之后，最后的解析工作花费了 40 分钟

io 都是 D 状态:
```txt
+ sudo bpftrace -e 'kprobe:filemap_fault { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
Attaching 2 probes...
^C

@[
        filemap_fault+5
        __do_fault+48
        do_read_fault+293
        do_fault+306
        __handle_mm_fault+940
        handle_mm_fault+256
        do_user_addr_fault+533
        exc_page_fault+126
        asm_exc_page_fault+38
]: 5207
```

```txt
+ sudo bpftrace -e 'kprobe:filemap_fault { @[curtask->comm] = count() } interval:s:1000 { exit(); }'
Attaching 2 probes...
^C

@[sshd-session]: 1
@[crond]: 1
@[nvim]: 2
@[abrt-dump-journ]: 4
@[bpftrace]: 4
@[zsh]: 5
@[htop]: 7
@[sqlx-sqlite-wor]: 31
@[systemd-journal]: 32
@[git-remote-http]: 45
@[node]: 70
@[direnv]: 123
@[atuin]: 460
@[starship]: 770
@[git]: 7398
```

## 打开嵌套虚拟化

Set-VMProcessor -VMName <VMName> -ExposeVirtualizationExtensions $true

这个名称居然就是 hyper v manager 上的名称，例如 fedora-2

## 测试一下存储性能

```txt
fio docs/kernel/blk/fio/4k-read.ini
trash: (g=0): rw=randwrite, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=io_uring, iodepth=1
...
fio-3.39
Starting 2 processes
trash: Laying out IO file (1 file / 10240MiB)
Jobs: 2 (f=2): [w(2)][0.9%][w=195MiB/s][w=50.0k IOPS][eta 16m:31s]
```
randwrite iouring 2 jobs ，对于 ext4 上的 10G 文件来 io
应该是存在什么限制，最高大约 50.2K

还行吧

## 网络测试
哪里网络配置的有点不对

windows 物理机使用 10.0.0.8 ，使用 windows 版本的 iperf3
```txt
iperf3 -c 10.0.0.8
Connecting to host 10.0.0.8, port 5201
[  5] local 172.23.195.66 port 35608 connected to 10.0.0.8 port 5201
[ ID] Interval           Transfer     Bitrate         Retr  Cwnd
[  5]   0.00-1.00   sec  19.6 MBytes   164 Mbits/sec  698   9.90 KBytes
[  5]   1.00-2.00   sec  9.12 MBytes  76.5 Mbits/sec  308   28.3 KBytes
[  5]   2.00-3.00   sec  9.50 MBytes  79.7 Mbits/sec  409   26.9 KBytes
[  5]   3.00-4.00   sec  18.0 MBytes   151 Mbits/sec  503   35.4 KBytes
[  5]   4.00-5.00   sec  9.75 MBytes  81.8 Mbits/sec  349   60.8 KBytes
[  5]   5.00-6.00   sec  13.9 MBytes   116 Mbits/sec  802   12.7 KBytes
[  5]   6.00-7.00   sec  24.4 MBytes   204 Mbits/sec  685   8.48 KBytes
^C[  5]   7.00-7.71   sec  7.38 MBytes  87.6 Mbits/sec  267   11.3 KBytes
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-7.71   sec   112 MBytes   121 Mbits/sec  4021            sender
[  5]   0.00-7.71   sec  0.00 Bytes  0.00 bits/sec                  receiver
iperf3: interrupt - the client has terminated by signal Interrupt(2)
```
不知道咋配置的，感觉像是 windows 的 iperf3
如果配置交换机和物理网卡绑定，和 mac 做测试，发现可以很容易达到 交换机的 上限的。

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
