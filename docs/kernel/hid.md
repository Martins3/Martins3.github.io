# hid

/sys/bus/hid/devices

```txt
@[
    usbhid_probe+5
    usb_probe_interface+229
    really_probe+211
    __driver_probe_device+120
    driver_probe_device+31
    __device_attach_driver+137
    bus_for_each_drv+146
    __device_attach+178
    bus_probe_device+141
    device_add+1736
    usb_set_configuration+1594
    usb_generic_driver_probe+80
    usb_probe_device+66
    really_probe+211
    __driver_probe_device+120
    driver_probe_device+31
    __device_attach_driver+137
    bus_for_each_drv+146
    __device_attach+178
    bus_probe_device+141
    device_add+1736
    usb_new_device+1052
    hub_event+4686
    process_one_work+395
    worker_thread+581
    kthread+205
    ret_from_fork+49
    ret_from_fork_asm+26
]: 1
```

```c
static const struct hid_ll_driver usb_hid_driver = {
	.parse = usbhid_parse,
	.start = usbhid_start,
	.stop = usbhid_stop,
	.open = usbhid_open,
	.close = usbhid_close,
	.power = usbhid_power,
	.request = usbhid_request,
	.wait = usbhid_wait_io,
	.raw_request = usbhid_raw_request,
	.output_report = usbhid_output_report,
	.idle = usbhid_idle,
	.may_wakeup = usbhid_may_wakeup,
};
```
这个只是介入到的 usb 的 hid 定义的，其他的类型还有，但是不只有这一个。

## 这么说，键盘的每一个按键都可以检测到

drivers/tty/vt/keyboard.c 中的 kbd_bh ，就是现在键盘的输入的

```txt
@[
    kbd_bh+5
    tasklet_action_common.isra.0+189
    handle_softirqs+228
    __irq_exit_rcu+152
    common_interrupt+135
    asm_common_interrupt+38
    _raw_spin_unlock_irqrestore+29
    hidinput_report_event+55
    hid_report_raw_event+207
    hid_input_report+265
    uhid_char_write+1131
    vfs_writev+633
    do_writev+236
    do_syscall_64+184
    entry_SYSCALL_64_after_hwframe+119
]: 1

@[
    kbd_bh+5
    tasklet_action_common.isra.0+189
    handle_softirqs+228
    run_ksoftirqd+47
    smpboot_thread_fn+218
    kthread+205
    ret_from_fork+49
    ret_from_fork_asm+26
]: 11
```

但是这个 hid 居然是从用户态过来的。

而 i8042_interrupt 这个 13900k 上没有感觉

## 其他的用户态工具
https://github.com/libusb/hidapi

## 先观察下基本情况
13900K
```txt
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 001 Device 002: ID 0b05:19af ASUSTek Computer, Inc. AURA LED Controller
Bus 001 Device 003: ID 3554:f503 Compx VGN Mouse 2.4G Receiver
Bus 001 Device 004: ID 320f:5088 Telink VGN S99 2.4G Dongle
Bus 001 Device 005: ID 174c:2074 ASMedia Technology Inc. ASM1074 High-Speed hub
Bus 001 Device 006: ID 8087:0026 Intel Corp. AX201 Bluetooth
Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
Bus 002 Device 002: ID 174c:3074 ASMedia Technology Inc. ASM1074 SuperSpeed hub
```
```txt
lrwxrwxrwx - root  6 Jul 12:10  0003:0B05:19AF.0001 -> ../../../devices/pci0000:00/0000:00:14.0/usb1/1-2/1-2:1.2/0003:0B05:19AF.0001
lrwxrwxrwx - root  6 Jul 12:10  0003:320F:5088.0005 -> ../../../devices/pci0000:00/0000:00:14.0/usb1/1-8/1-8:1.0/0003:320F:5088.0005
lrwxrwxrwx - root  6 Jul 12:10  0003:320F:5088.0006 -> ../../../devices/pci0000:00/0000:00:14.0/usb1/1-8/1-8:1.1/0003:320F:5088.0006
lrwxrwxrwx - root  6 Jul 12:10  0003:3554:F503.0002 -> ../../../devices/pci0000:00/0000:00:14.0/usb1/1-3/1-3:1.0/0003:3554:F503.0002
lrwxrwxrwx - root  6 Jul 12:10  0003:3554:F503.0003 -> ../../../devices/pci0000:00/0000:00:14.0/usb1/1-3/1-3:1.1/0003:3554:F503.0003
lrwxrwxrwx - root  6 Jul 12:10  0003:3554:F503.0004 -> ../../../devices/pci0000:00/0000:00:14.0/usb1/1-3/1-3:1.2/0003:3554:F503.0004
lrwxrwxrwx - root  6 Jul 12:10  0005:054C:0DF0.0008 -> ../../../devices/virtual/misc/uhid/0005:054C:0DF0.0008
lrwxrwxrwx - root  6 Jul 12:10  0005:320F:5055.0007 -> ../../../devices/virtual/misc/uhid/0005:320F:5055.0007
```

ubuntu qemu
```txt
$ lsusb
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 001 Device 002: ID 0627:0001 Adomax Technology Co., Ltd QEMU Tablet
Bus 001 Device 003: ID 0627:0001 Adomax Technology Co., Ltd QEMU Tablet
Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
```
```txt
$ ls -la /sys/bus/hid/devices
total 0
drwxr-xr-x 2 root root 0 Jul  6 12:24 .
drwxr-xr-x 4 root root 0 Jul  6 11:56 ..
lrwxrwxrwx 1 root root 0 Jul  6 11:56 0003:0627:0001.0001 -> ../../../devices/pci0000:00/0000:00:11.0/usb1/1-2/1-2:1.0/0003:0627:0001.0001
lrwxrwxrwx 1 root root 0 Jul  6 11:56 0003:0627:0001.0002 -> ../../../devices/pci0000:00/0000:00:11.0/usb1/1-3/1-3:1.0/0003:0627:0001.0002
```

apple
```txt
🤒  lsusb
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
Bus 002 Device 002: ID 0b95:1790 ASIX Electronics Corp. AX88179 Gigabit Ethernet

🧀  ls -la /sys/bus/hid/devices
lrwxrwxrwx 0 root  6 Jul 12:26  0019:0000:0000.0001 -> ../../../devices/platform/soc/24eb14000.fifo/24eb30000.input/0019:0000:0000.0001
lrwxrwxrwx 0 root  6 Jul 12:26  0019:05AC:0354.0002 -> ../../../devices/platform/soc/24eb14000.fifo/24eb30000.input/0019:05AC:0354.0002
lrwxrwxrwx 0 root  6 Jul 12:26  0019:05AC:0354.0003 -> ../../../devices/platform/soc/24eb14000.fifo/24eb30000.input/0019:05AC:0354.0003
lrwxrwxrwx 0 root  6 Jul 12:26  0019:05AC:0354.0004 -> ../../../devices/platform/soc/24eb14000.fifo/24eb30000.input/0019:05AC:0354.0004
lrwxrwxrwx 0 root  6 Jul 12:26  0019:05AC:0354.0005 -> ../../../devices/platform/soc/24eb14000.fifo/24eb30000.input/0019:05AC:0354.0005
```

似乎 hid + usb 就是来处理键盘鼠标的了 ?

## 和 bpf 的结合，这才是有趣的问题
- https://www.kernel.org/doc/html/next/hid/hid-bpf.html
  - https://lwn.net/Articles/909109/
  - https://news.ycombinator.com/item?id=33625475

虽然，但是，还是不太懂 hid 为什么可以使用 bpf 来处理，
和 nvme 的 ebpf 有关系吗?

samples/hid/

## 如果是蓝牙键盘，那么是不是 hid 就不会 usb 了，而且可以 trace 到 Bluetooth 的调用栈

也许是的:
```txt
[    5.858691] input: VGN S99-1 Keyboard as /devices/virtual/misc/uhid/0005:320F:5055.0007/input/input28
[    5.858801] input: VGN S99-1 Mouse as /devices/virtual/misc/uhid/0005:320F:5055.0007/input/input29
[    5.858853] hid-generic 0005:320F:5055.0007: input,hidraw6: BLUETOOTH HID v1.04 Keyboard [VGN S99-1] on 70:a8:d3:66:73:c0
```
而且这里接入一个有趣的 sysfs 结构。

### 似乎键盘用 Bluetooth 的功耗很高，使用 2.4G 的接受器，那么可以看到对应
的 io 路径有什么不同吗?

## oe2 虚拟机的 kernel 启动之后，需要这两个选项

不然没有办法在 terminal 中输入字母的:
```txt
CONFIG_USB_SUPPORT is not set
CONFIG_HID_SUPPORT is not set
```
但是 -serial mon:stdio 就可以完全不依赖这个:

### 此外，如果 arm 的环境中，如果没有 VIRTIO_GPU (依赖 DRM)

## 可以让 x86 也不要用 VGA 了，用 virtio-gpu


## 素材
https://static.vgnclub.com/vgn/driver/S99/s99%E8%AF%B4%E6%98%8E%E4%B9%A6.pdf

```txt
[431374.184045] Bluetooth: hci0: ACL packet for unknown connection handle 2048
```

## 看看这个内核日志
```txt
[  817.695140] input: WH-1000XM5 (AVRCP) as /devices/virtual/input/input35
```

## BMC 提供的 USB，这也何尝不是一个 hid
```txt
[  168.656121] usb 1-14.2: USB disconnect, device number 4
[  456.060165] usb 1-14.2: new high-speed USB device number 6 using xhci_hcd
[  456.158212] usb 1-14.2: New USB device found, idVendor=413c, idProduct=0000, bcdDevice= 0.00
[  456.167350] usb 1-14.2: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[  456.175447] usb 1-14.2: Product: DRAC 5 Virtual Keyboard and Mouse
[  456.182328] usb 1-14.2: Manufacturer: DELLEMC
[  456.187389] usb 1-14.2: SerialNumber: DELL413C
[  456.204725] input: DELLEMC DRAC 5 Virtual Keyboard and Mouse as /devices/pci0000:00/0000:00:14.0/usb1/1-14/1-14.2/1-14.2:1.0/0003:413C:0000.0003/input/input4
[  456.220677] hid-generic 0003:413C:0000.0003: input,hidraw0: USB HID v1.01 Mouse [DELLEMC DRAC 5 Virtual Keyboard and Mouse] on usb-0000:00:14.0-14.2/input0
[  456.241474] input: DELLEMC DRAC 5 Virtual Keyboard and Mouse as /devices/pci0000:00/0000:00:14.0/usb1/1-14/1-14.2/1-14.2:1.1/0003:413C:0000.0004/input/input5
[  456.308286] hid-generic 0003:413C:0000.0004: input,hidraw1: USB HID v1.01 Keyboard [DELLEMC DRAC 5 Virtual Keyboard and Mouse] on usb-0000:00:14.0-14.2/input1
[  473.040248] usb 1-14.2: USB disconnect, device number 6
```

## 看看这个吧
https://news.ycombinator.com/item?id=41295390

## 和 CONFIG_HID  是什么关系？

## 不知道为什么 n100 自动加载了 evbug ，而且还有展示
drivers/input/evbug.c

```txt
module evbug did not have configs CONFIG_INPUT_EVBUG
```

一般是没有加载的，如果加载的时候，会有这个现象:

这个是 13900k 的:
```txt
[591277.127479] evbug: Connected device: input1 (Sleep Button at PNP0C0E/button/input0)
[591277.127482] evbug: Connected device: input2 (Power Button at PNP0C0C/button/input0)
[591277.127483] evbug: Connected device: input3 (Power Button at LNXPWRBN/button/input0)
[591277.127484] evbug: Connected device: input4 (Eee PC WMI hotkeys at eeepc-wmi/input0)
[591277.127485] evbug: Connected device: input5 (Asus WMI hotkeys at asus-nb-wmi/input0)
[591277.127486] evbug: Connected device: input6 (HDA NVidia HDMI/DP,pcm=3 at ALSA)
[591277.127486] evbug: Connected device: input7 (HDA NVidia HDMI/DP,pcm=7 at ALSA)
[591277.127487] evbug: Connected device: input8 (HDA NVidia HDMI/DP,pcm=8 at ALSA)
[591277.127488] evbug: Connected device: input9 (HDA NVidia HDMI/DP,pcm=9 at ALSA)
[591277.127489] evbug: Connected device: input10 (Video Bus at LNXVIDEO/video/input0)
[591277.127490] evbug: Connected device: input11 (HDA Intel PCH Rear Mic at ALSA)
[591277.127491] evbug: Connected device: input12 (HDA Intel PCH Front Mic at ALSA)
[591277.127491] evbug: Connected device: input13 (HDA Intel PCH Line at ALSA)
[591277.127492] evbug: Connected device: input14 (HDA Intel PCH Line Out Front at ALSA)
[591277.127492] evbug: Connected device: input15 (HDA Intel PCH Line Out Surround at ALSA)
[591277.127493] evbug: Connected device: input16 (HDA Intel PCH Line Out CLFE at ALSA)
[591277.127494] evbug: Connected device: input17 (HDA Intel PCH Front Headphone at ALSA)
[591277.127494] evbug: Connected device: input18 (HDA Intel PCH HDMI/DP,pcm=3 at ALSA)
[591277.127495] evbug: Connected device: input19 (HDA Intel PCH HDMI/DP,pcm=7 at ALSA)
[591277.127495] evbug: Connected device: input20 (HDA Intel PCH HDMI/DP,pcm=8 at ALSA)
[591277.127496] evbug: Connected device: input21 (HDA Intel PCH HDMI/DP,pcm=9 at ALSA)
[591277.127497] evbug: Connected device: input22 (VGN S99-1 Keyboard at 70:a8:d3:66:73:c0)
[591277.127498] evbug: Connected device: input23 (VGN S99-1 Mouse at 70:a8:d3:66:73:c0)
[591277.127499] evbug: Connected device: input27 ( USB OPTICAL MOUSE at usb-0000:00:14.0-8/input0)
```
这个是 n100 的:
```txt
[77797.707143] evbug: Connected device: input0 (Sleep Button at PNP0C0E/button/input0)
[77797.707151] evbug: Connected device: input1 (Power Button at PNP0C0C/button/input0)
[77797.707152] evbug: Connected device: input2 (Power Button at LNXPWRBN/button/input0)
[77797.707154] evbug: Connected device: input3 (Video Bus at LNXVIDEO/video/input0)
[77797.707156] evbug: Connected device: input7 (HDA Intel PCH Mic at ALSA)
[77797.707158] evbug: Connected device: input8 (HDA Intel PCH Headphone at ALSA)
[77797.707160] evbug: Connected device: input9 (HDA Intel PCH HDMI/DP,pcm=3 at ALSA)
[77797.707162] evbug: Connected device: input10 (HDA Intel PCH HDMI/DP,pcm=7 at ALSA)
[77797.707163] evbug: Connected device: input11 (HDA Intel PCH HDMI/DP,pcm=8 at ALSA)
[77797.707164] evbug: Connected device: input12 (HDA Intel PCH HDMI/DP,pcm=9 at ALSA)
[77797.707165] evbug: Connected device: input13 (Microsoft Microsoft Ergonomic Keyboard at usb-0000:00:14.0-3/input0)
[77797.707167] evbug: Connected device: input14 (Microsoft Microsoft Ergonomic Keyboard Mouse at usb-0000:00:14.0-3/input0)
```

所以 drivers/input/ 和 drivers/hid 有什么关系?

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
