# usb 分析记录记录

## 4k 随机读 3.5M
```txt
🤒  sudo fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=128
fio-3.33
Starting 1 process
^Cbs: 1 (f=1): [r(1)][70.0%][r=3480KiB/s][r=870 IOPS][eta 00m:03s]
fio: terminating on signal 2

trash: (groupid=0, jobs=1): err= 0: pid=1047476: Sat Jan  7 17:35:36 2023
  read: IOPS=869, BW=3477KiB/s (3560kB/s)(26.6MiB/7834msec)
    slat (usec): min=2, max=8856, avg=1149.11, stdev=572.75
    clat (msec): min=3, max=167, avg=144.74, stdev=12.06
     lat (msec): min=4, max=168, avg=145.89, stdev=12.06
    clat percentiles (msec):
     |  1.00th=[   91],  5.00th=[  138], 10.00th=[  140], 20.00th=[  142],
     | 30.00th=[  142], 40.00th=[  144], 50.00th=[  146], 60.00th=[  146],
     | 70.00th=[  148], 80.00th=[  150], 90.00th=[  155], 95.00th=[  157],
     | 99.00th=[  161], 99.50th=[  165], 99.90th=[  167], 99.95th=[  167],
     | 99.99th=[  167]
   bw (  KiB/s): min= 2472, max= 3608, per=98.08%, avg=3410.67, stdev=269.63, samples=15
   iops        : min=  618, max=  902, avg=852.67, stdev=67.41, samples=15
  lat (msec)   : 4=0.01%, 10=0.07%, 20=0.12%, 50=0.35%, 100=0.57%
  lat (msec)   : 250=98.87%
  cpu          : usr=0.04%, sys=0.28%, ctx=13228, majf=0, minf=138
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.2%, 32=0.5%, >=64=99.1%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.1%
     issued rwts: total=6809,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=128

Run status group 0 (all jobs):
   READ: bw=3477KiB/s (3560kB/s), 3477KiB/s-3477KiB/s (3560kB/s-3560kB/s), io=26.6MiB (27.9MB), run=7834-7834msec
```

## 256K 30M
```txt
Disk stats (read/write):
  sdb: ios=6653/1, merge=0/0, ticks=15273/1, in_queue=15274, util=98.73%
vn on  master [!+] took 8s
🤒  sudo fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio
trash: (g=0): rw=randread, bs=(R) 256KiB-256KiB, (W) 256KiB-256KiB, (T) 256KiB-256KiB, ioengine=libaio, iodepth=128
fio-3.33
Starting 1 process
^Cbs: 1 (f=1): [r(1)][60.0%][r=27.2MiB/s][r=109 IOPS][eta 00m:04s]
fio: terminating on signal 2

trash: (groupid=0, jobs=1): err= 0: pid=1047689: Sat Jan  7 17:35:50 2023
  read: IOPS=116, BW=29.1MiB/s (30.5MB/s)(178MiB/6107msec)
    slat (usec): min=3710, max=38756, avg=8589.63, stdev=3785.08
    clat (msec): min=5, max=1188, avg=997.41, stdev=244.21
     lat (msec): min=16, max=1197, avg=1006.00, stdev=244.09
    clat percentiles (msec):
     |  1.00th=[   64],  5.00th=[  326], 10.00th=[  634], 20.00th=[ 1028],
     | 30.00th=[ 1045], 40.00th=[ 1053], 50.00th=[ 1070], 60.00th=[ 1083],
     | 70.00th=[ 1116], 80.00th=[ 1133], 90.00th=[ 1150], 95.00th=[ 1167],
     | 99.00th=[ 1183], 99.50th=[ 1183], 99.90th=[ 1183], 99.95th=[ 1183],
     | 99.99th=[ 1183]
   bw (  KiB/s): min=25088, max=31744, per=98.40%, avg=29286.40, stdev=1972.66, samples=10
   iops        : min=   98, max=  124, avg=114.40, stdev= 7.71, samples=10
  lat (msec)   : 10=0.14%, 20=0.14%, 50=0.56%, 100=0.70%, 250=2.25%
  lat (msec)   : 500=3.66%, 750=4.37%, 1000=4.23%, 2000=83.94%
  cpu          : usr=0.02%, sys=0.23%, ctx=3262, majf=0, minf=8202
  IO depths    : 1=0.1%, 2=0.3%, 4=0.6%, 8=1.1%, 16=2.3%, 32=4.5%, >=64=91.1%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=99.8%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.2%
     issued rwts: total=710,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=128

Run status group 0 (all jobs):
   READ: bw=29.1MiB/s (30.5MB/s), 29.1MiB/s-29.1MiB/s (30.5MB/s-30.5MB/s), io=178MiB (186MB), run=6107-6107msec

Disk stats (read/write):
  sdb: ios=2059/1, merge=0/0, ticks=11758/0, in_queue=11759, util=98.38%
```

# xhci ohci 之类都是什么东西

https://en.wikipedia.org/wiki/Host_controller_interface_(USB,_Firewire)

只是看看 linux/drivers/usb/ 下的内容吧:

linux/drivers/usb/host/ 下罗列各种 hci ，目前重要的 xhci

linux/drivers/usb/host/core 显然是核心

其他的 storage 和 serial 也是可以看看的。

## ohci
- usb 键盘中断流程
```txt
[   75.597619] [<900000000020866c>] show_stack+0x2c/0x100
[   75.597621] [<9000000000ec39c8>] dump_stack+0x90/0xc0
[   75.597624] [<9000000000c4b1b0>] input_event+0x30/0xc8
[   75.597626] [<9000000000ca3ee4>] hidinput_report_event+0x44/0x68
[   75.597628] [<9000000000ca1e30>] hid_report_raw_event+0x230/0x470
[   75.597631] [<9000000000ca21a4>] hid_input_report+0x134/0x1b0
[   75.597632] [<9000000000cb07ac>] hid_irq_in+0x9c/0x280
[   75.597634] [<9000000000be9cf0>] __usb_hcd_giveback_urb+0xa0/0x120
[   75.597636] [<9000000000c23a7c>] finish_urb+0xac/0x1c0
[   75.597638] [<9000000000c24b50>] ohci_work.part.8+0x218/0x550 [   75.597640] [<9000000000c27f98>] ohci_irq+0x108/0x320
[   75.597642] [<9000000000be96e8>] usb_hcd_irq+0x28/0x40
[   75.597644] [<9000000000296430>] __handle_irq_event_percpu+0x70/0x1b8
[   75.597645] [<9000000000296598>] handle_irq_event_percpu+0x20/0x88
[   75.597647] [<9000000000296644>] handle_irq_event+0x44/0xa8
[   75.597648] [<900000000029abfc>] handle_level_irq+0xdc/0x188
[   75.597651] [<90000000002952a4>] generic_handle_irq+0x24/0x40
[   75.597652] [<900000000081dc50>] extioi_irq_dispatch+0x178/0x210
[   75.597654] [<90000000002952a4>] generic_handle_irq+0x24/0x40
[   75.597656] [<9000000000ee4eb8>] do_IRQ+0x18/0x28
[   75.597658] [<9000000000203ffc>] except_vec_vi_end+0x94/0xb8
[   75.597660] [<9000000000203e80>] __cpu_wait+0x20/0x24
[   75.597662] [<900000000020fa90>] calculate_cpu_foreign_map+0x148/0x180
```

## xhci

对于 U 盘 fio 就是这个结果:
```txt
@[
    xhci_irq+1
    __handle_irq_event_percpu+70
    handle_irq_event+58
    handle_edge_irq+177
    __common_interrupt+105
    common_interrupt+179
    asm_common_interrupt+34
    cpuidle_enter_state+222
    cpuidle_enter+41
    do_idle+492
    cpu_startup_entry+25
    start_secondary+271
    secondary_startup_64_no_verify+224
]: 1759
```

## usb controller
https://www.cs.cmu.edu/~412/lectures/L05_xHCI.pdf

当从将 USB controller 去掉的时候:
```txt
09:00.0 USB controller: Advanced Micro Devices, Inc. [AMD] Device 15b8
```
```sh
echo 0000:09:00.0 | sudo tee /sys/bus/pci/devices/0000:09:00.0/driver/unbind
```
```txt
[ 3105.455424] xhci_hcd 0000:09:00.0: remove, state 4 [ 3105.455429] usb usb6: USB disconnect, device number 1
[ 3105.455528] xhci_hcd 0000:09:00.0: USB bus 6 deregistered
[ 3105.455538] xhci_hcd 0000:09:00.0: remove, state 1
[ 3105.455540] usb usb5: USB disconnect, device number 1
[ 3105.455541] usb 5-1: USB disconnect, device number 2
[ 3105.455541] usb 5-1.1: USB disconnect, device number 3
[ 3105.457767] usb 5-1.2: USB disconnect, device number 4
[ 3105.651330] xhci_hcd 0000:09:00.0: USB bus 5 deregistered
```

https://unix.stackexchange.com/questions/338026/centos-how-to-emulate-a-usb-flashdrive


## [ ]  从 scsi 到 usb 是如何进行的

使用 perf 无法观测到 usb 相关的控制面，iops 不到 1k ，实在是太难了:
```txt
🧀  t
sudo bpftrace -e "kprobe:scsi_end_request { @[kstack] = count(); }"
Attaching 1 probe...
^C

@[
    scsi_end_request+1
    scsi_io_completion+90
    usb_stor_control_thread+568
    kthread+229
    ret_from_fork+49
    ret_from_fork_asm+27
]: 1291
```
采用 docs/kernel/perf/list.md 中

```sh
sudo taskset -ac 1 fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio
```

可见，usb 是总线协议，但是 scsi 是存储协议。


简单对比下 sata 和 usb 的差别:
```txt
lrwxrwxrwx - root  4 Jan 15:26  sda -> ../devices/pci0000:00/0000:00:17.0/ata8/host7/target7:0:0/7:0:0:0/block/sda
lrwxrwxrwx - root  4 Jan 15:26  sdd -> ../devices/pci0000:00/0000:00:14.0/usb1/1-8/1-8:1.0/host9/target9:0:0/9:0:0:0/block/sdd
```
和存储相关的代码在 drivers/usb/storage/scsiglue.c 中

```c
static const struct scsi_host_template usb_stor_host_template = {
	/* basic userland interface stuff */
	.name =				"usb-storage",
	.proc_name =			"usb-storage",
```

- [ ] 我猜测这里是将 scsi 命令进行装换，然后发送给具体的硬件 ?

- [ ] 无论如何，如果想要继续 hacking ，那么搭建一下 QEMU 的环境吧

## USB A 和 USB C 分别指的是什么东西?

bilibili 视频: [USB 接口科普:看了就懂 Type-A、Type-B、Type-C、雷电 3、雷电 4、USB2.0、USB3.0、USB4 都是什么](https://www.bilibili.com/video/BV1qV411j7Jv)


## USB 的 eh 机制
```txt
[162444.186682] sd 9:0:0:0: [sdd] Attached SCSI disk
[162530.883024] sd 9:0:0:0: [sdd] tag#24 uas_eh_abort_handler 0 uas-tag 1 inflight: OUT
[162530.883027] sd 9:0:0:0: [sdd] tag#24 CDB: ATA command pass through(12)/Blank a1 80 00 00 02 00 00 00 00 00 00 00
[162530.889011] scsi host9: uas_eh_device_reset_handler start
[162530.981337] usb 2-8.4: reset SuperSpeed USB device number 5 using xhci_hcd
[162531.009174] scsi host9: uas_eh_device_reset_handler success
```

## 制作你自己的 USB 设备
- https://popovicu.com/posts/making-usb-devices/
  - https://news.ycombinator.com/item?id=40560300


## 在 mac 上分别连接扩展坞和没有连接扩展坞的场景

```txt
lrwxrwxrwx 0 root  5 Jun 08:12  sda -> ../../devices/platform/soc/502280000.usb/xhci-hcd.1.auto/usb4/4-1/4-1.2/4-1.2:1.0/host0/target0:0:0/0:0:0:0/block/sda
lrwxrwxrwx 0 root  5 Jun 08:13  sda -> ../../devices/platform/soc/502280000.usb/xhci-hcd.1.auto/usb4/4-1/4-1:1.0/host0/target0:0:0/0:0:0:0/block/sda
```

## 用用 cyme 吧，太好看了
```txt
🧀  cyme
  1   4  0x174c 0x2074 ASM107x              -            480.0 Mb/s
  1   3  0x2717 0x003b MI Wireless Mouse    -             12.0 Mb/s
  1   2  0x0b05 0x19af AURA LED Controller  9876543210    12.0 Mb/s
  1   6  0x8087 0x0026 AX201 Bluetooth      -             12.0 Mb/s
  1   1  0x1d6b 0x0002 xHCI Host Controller 0000:00:14.0 480.0 Mb/s
  2   2  0x174c 0x3074 ASM107x              -              5.0 Gb/s
  2   1  0x1d6b 0x0003 xHCI Host Controller 0000:00:14.0          -
```

## 在 13900k 上找到了华硕的 LED 灯
```txt
🧀  cyme
  1   2  0x0b05 0x19af AURA LED Controller 9876543210  12.0 Mb/s
  1   4  0x174c 0x2074 ASM107x             -          480.0 Mb/s
  1   5  0x8087 0x0026 AX201 Bluetooth     -           12.0 Mb/s
  1   7  0x0000 0x3825 USB OPTICAL MOUSE   -            1.5 Mb/s
  2   2  0x174c 0x3074 ASM107x             -            5.0 Gb/s
```
那么可以找到这个东西的驱动吗?


## qemu hmp 有这两个
info usb  -- show guest USB devices
info usbhost  -- show host USB devices

## 很好
https://www.youtube.com/playlist?list=PLATP7rOKo3E82tBnMp90B4zejpWeAKlxn

https://news.ycombinator.com/item?id=44318588

## 似乎有趣
https://news.ycombinator.com/item?id=44345681

## 原来内核中就支持远程 usb 设备啊
drivers/usb/usbip/

```txt
config USBIP_CORE
	tristate "USB/IP support"
	depends on NET
	select USB_COMMON
	select SGL_ALLOC
	help
	  This enables pushing USB packets over IP to allow remote
	  machines direct access to USB devices. It provides the
	  USB/IP core that is required by both drivers.

	  For more details, and to get the userspace utility
	  programs, please see <http://usbip.sourceforge.net/>.

	  To compile this as a module, choose M here: the module will
	  be called usbip-core.

	  If unsure, say N.
```

## 收集一些材料

kbd_event :

```txt
kbd_event
=> input_to_handler
=> input_pass_values.part.6
=> input_handle_event
=> input_event
=> hidinput_report_event
=> hid_report_raw_event
=> hid_input_report
=> hid_irq_in
=> __usb_hcd_giveback_urb
=> xhci_giveback_urb_in_irq.isra.38
=> xhci_td_cleanup
=> handle_tx_event
=> xhci_irq
=> __handle_irq_event_percpu
=> handle_irq_event_percpu
=> handle_irq_event
=> handle_edge_irq
=> handle_irq
=> do_IRQ
=> ret_from_intr
=> cpuidle_enter_state
=> do_idle
=> cpu_startup_entry
=> start_secondary
=> secondary_startup_64
```

```txt
 => ebtable_filter_init
 => kbd_event
 => input_to_handler
 => input_pass_values.part.5
 => input_handle_event
 => input_event
 => hidinput_report_event
 => hid_report_raw_event
 => hid_input_report
 => hid_irq_in
 => __usb_hcd_giveback_urb
 => usb_giveback_urb_bh
 => tasklet_action_common.isra.14
 => __do_softirq
 => irq_exit
 => do_IRQ
 => ret_from_intr
 => cpuidle_enter_state
 => cpuidle_enter
 => do_idle
 => cpu_startup_entry
 => start_secondary
 => secondary_startup_64
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
