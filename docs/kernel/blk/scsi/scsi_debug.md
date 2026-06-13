## scsi_debug
https://sg.danny.cz/sg/scsi_debug.html

### scsi debug 来模拟 raid1 必有问题，为什么
modprobe scsi_debug max_luns=2 dev_size_mb=10000
mdadm --create --failfast --verbose /dev/md0 --level=1 --raid-devices=2 /dev/sdb /dev/sda

### 创建 scsi_debug 的时候指定大小
sudo rmmod scsi_debug
sudo modprobe scsi_debug dev_size_mb=300

### 添加更多的盘
```txt
cd /sys/bus/pseudo/drivers/scsi_debug
cat dev_size_mb # 输出 8

echo 2 | sudo tee max_luns
echo 1 | sudo tee add_host
```

然后 lsblk 可以看到
```txt
sdc               8:32   0     8M  0 disk
sdd               8:48   0     8M  0 disk
sde               8:64   0     8M  0 disk
```

lsscsi
```txt
[4:0:0:0]    disk    Linux    scsi_debug       0191  /dev/sdc
[5:0:0:0]    disk    Linux    scsi_debug       0191  /dev/sdd
[5:0:0:1]    disk    Linux    scsi_debug       0191  /dev/sde
```

### 构建一个无限延迟的

```txt
cd /sys/module/scsi_debug/parameters
echo 10000000 | sudo tee delay
```

使用 src/c/fs/aio-minimal.c 测试，效果完全相同:
```txt
 sudo  cat /proc/9388/stack

[sudo] password for martins3:
[<0>] exit_aio+0x122/0x140
[<0>] __mmput+0x12/0x110
[<0>] exit_mm+0xb1/0x110
[<0>] do_exit+0x20b/0x4a0
[<0>] do_group_exit+0x30/0x80
[<0>] get_signal+0x7b3/0x7d0
[<0>] arch_do_signal_or_restart+0x35/0x100
[<0>] syscall_exit_to_user_mode+0x126/0x210
[<0>] do_syscall_64+0x8c/0x170
[<0>] entry_SYSCALL_64_after_hwframe+0x76/0x7e
```

但是很快就会被 offline 掉:
```txt
[139547.581734] sd 4:0:0:0: Power-on or device reset occurred
[139557.802249] sd 4:0:0:0: Power-on or device reset occurred
[139568.041740] sd 4:0:0:0: Power-on or device reset occurred
[139578.281910] sd 4:0:0:0: Power-on or device reset occurred
[139588.521684] sd 4:0:0:0: Device offlined - not ready after error recovery
[139588.523287] sd 4:0:0:0: [sdc] tag#10 FAILED Result: hostbyte=DID_TIME_OUT driverbyte=DRIVER_OK cmd_age=101s
[139588.525147] sd 4:0:0:0: [sdc] tag#10 CDB: Read(10) 28 00 00 00 00 00 00 00 08 00
[139588.527762] I/O error, dev sdc, sector 0 op 0x0:(READ) flags 0x0 phys_seg 1 prio class 0
```

可以看到 /sys/block/sdd/inflight 结果为
```txt
 cat inflight
       2        0
```


## dm

### dm-dust
- https://blogs.oracle.com/linux/post/error-injection-using-dm-dust
  - dm-dust is a Linux kernel module which can be used to simulate the bad  blocks behavior on a physical disk.


### dm-delay
https://serverfault.com/questions/523509/linux-how-to-simulate-hard-disk-latency-i-want-to-increase-iowait-value-withou


```sh
echo "0 `sudo blockdev --getsz /dev/nvme1n1` delay /dev/nvme1n1 0 5000" | sudo dmsetup create delayed
# 不知道为什么，这个执行的特别慢

# 可以动态修改吗? 想要注入一个超长时间的延迟，结果 mkfs 都要超时
sudo mkfs.ext4 /dev/mapper/delayed
```

https://github.com/kawamuray/ddi


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
