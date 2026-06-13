## 从 qemu 的角度分析 Option ROM
https://en.wikipedia.org/wiki/Option_ROM

```txt
pci_update_mappings pci febe0000 vga.rom
pci_update_mappings pci feb80000 e1000.rom
```

- [x] 更新了，为什么在地址空间中间看不到 pci 的 rom 啊
这个地址之后被隐藏了
```txt
Option rom sizing returned febe0000 ffff0000
map_pcirom 0xfebe0000
```
最开始的时候，将 ROM 映射到 PCI 空间中，然后拷贝到 ROM 中，然后更新 PCI 空间, 这个 ROM 被隐藏起来了。

```txt
pci_add_option_rom /home/maritns3/core/kvmqemu/build/pc-bios/vgabios-stdvga.bin
ram_block_add vga.rom
```
vga 的源代码应该是在 : https://github.com/qemu/vgabios


## 直通 megaraid 来测试

如果直通了 megaraid 卡之后，观察参数:

```txt
Scan for option roms
Running option rom at ca00:0003
pmm call arg1=1
pmm call arg1=0
pmm call arg1=1
pmm call arg1=0
Running option rom at cb00:0003
pmm call arg1=0
pmm call arg1=0
pmm call arg1=2
pmm call arg1=2
Searching bootorder for: /pci@i0cf8/*@6
Searching bootorder for: /pci@i0cf8/*@8
Searching bootorder for: /rom@genroms/kvmvapic.bin

Press ESC for boot menu.

Searching bootorder for: HALT
drive 0x000e9570: PCHS=16383/16/63 translation=lba LCHS=1024/255/63 s=734003200
drive 0x000e9520: PCHS=0/0/0 translation=lba LCHS=1024/255/63 s=20971520
Running option rom at cb00:00cc
Running option rom at d380:0003
Space available for UMB: d6000-e9000, f4da0-f5160
Returned 16613376 bytes of ZoneHigh
e820 map has 8 items:
  0: 0000000000000000 - 000000000009ec00 = 1 RAM
  1: 000000000009ec00 - 00000000000a0000 = 2 RESERVED
  2: 00000000000f0000 - 0000000000100000 = 2 RESERVED
  3: 0000000000100000 - 00000000bffd8000 = 1 RAM
  4: 00000000bffd8000 - 00000000c0000000 = 2 RESERVED
  5: 00000000feffc000 - 00000000ff000000 = 2 RESERVED
  6: 00000000fffc0000 - 0000000100000000 = 2 RESERVED
  7: 0000000100000000 - 0000000240000000 = 1 RAM
enter handle_19:
  NULL
Booting from Hard Disk...
Booting from 0000:7c00
```

如果没有直通 megaraid 卡:
```txt
Scan for option roms
Running option rom at ca00:0003
pmm call arg1=1
pmm call arg1=0
pmm call arg1=1
pmm call arg1=0
Searching bootorder for: /pci@i0cf8/*@6
Searching bootorder for: /rom@genroms/kvmvapic.bin

Press ESC for boot menu.

Searching bootorder for: HALT
drive 0x000e9570: PCHS=16383/16/63 translation=lba LCHS=1024/255/63 s=734003200
drive 0x000e9520: PCHS=0/0/0 translation=lba LCHS=1024/255/63 s=20971520
Running option rom at cb00:0003
Space available for UMB: cd800-e9000, f4da0-f5170
Returned 16613376 bytes of ZoneHigh
e820 map has 8 items:
  0: 0000000000000000 - 000000000009fc00 = 1 RAM
  1: 000000000009fc00 - 00000000000a0000 = 2 RESERVED
  2: 00000000000f0000 - 0000000000100000 = 2 RESERVED
  3: 0000000000100000 - 00000000bffd8000 = 1 RAM
  4: 00000000bffd8000 - 00000000c0000000 = 2 RESERVED
  5: 00000000feffc000 - 00000000ff000000 = 2 RESERVED
  6: 00000000fffc0000 - 0000000100000000 = 2 RESERVED
  7: 0000000100000000 - 0000000240000000 = 1 RAM
enter handle_19:
  NULL
Booting from Hard Disk...
Booting from 0000:7c00
```

对比可以看到，megaraid 的 option rom 显然被执行了，而且似乎 virtio-blk 加载启动也是会有 option rom 的

开机过程为

![](./111.png)
![](./222.png)
![](./333.png)

## 在 linux 中观测 option rom 的方法
```txt
🤒  ls -l /sys/bus/pci/devices/*/rom
.rw------- 131k root  2 Nov 20:08 /sys/bus/pci/devices/0000:00:02.0/rom
.rw------- 524k root  2 Nov 20:08 /sys/bus/pci/devices/0000:01:00.0/rom
.rw------- 131k root  2 Nov 20:08 /sys/bus/pci/devices/0000:02:00.0/rom
```

参考 http://etherboot.org/wiki/romdumping

0000:04:00.0 看看 megaraid 卡的操作:

```txt
echo 1 | sudo tee /sys/bus/pci/devices/0000:04:00.0/rom
sudo cp /sys/bus/pci/devices/0000:04:00.0/rom hba.rom
file hba.rom
```
可以看到
```txt
gpu.rom: BIOS (ia32) ROM Ext. (96*512) jmp 0x1e60; at 0x70 PNP storage controller, length 36866*16, CRC 0x7, at 0xf0 "BROADCOM Inc. ", at 0xff "(Bus Dev )PCI RAID Adapter", IPL, bootable, cacheable, shadowable, boot vector offset 0xcc; at 0x1c PCI Broadcom device=0x10e2 storage controller, revision 3, code revision 0x3, 3rd reserved 0x6000
```
由于，这个卡就只能在 BIOS (ia32) ROM Ext 使用 x86 CPU 来执行才可以

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
