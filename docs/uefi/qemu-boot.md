## 使用 qemu 来理解 boot 机器

可以很容易配置多个 iso ，然后就看那个启动了
```txt
root@localhost:~# lsblk
NAME            MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
sda               8:0    0    10G  0 disk
sr0              11:0    1   3.2G  0 rom
sr1              11:1    1 698.4M  0 rom
zram0           251:0    0   7.7G  0 disk [SWAP]
vda             253:0    0   350G  0 disk
├─vda1          253:1    0     1M  0 part
├─vda2          253:2    0     2G  0 part /boot
└─vda3          253:3    0   348G  0 part
  └─fedora-root 252:0    0    15G  0 lvm  /
```

如果不去配置 bootindex
```txt
UEFI Interactive Shell v2.2
EDK II
UEFI v2.70 (EDK II, 0x00010000)
Mapping table
      FS0: Alias(s):CD0c1:;BLK2:
          PciRoot(0x0)/Pci(0x1,0x1)/Ata(0x0)/CDROM(0x1)
     BLK3: Alias(s):
          PciRoot(0x0)/Pci(0x2,0x0)/Scsi(0x1,0x0)
     BLK0: Alias(s):
          PciRoot(0x0)/Pci(0x1,0x1)/Ata(0x0)
     BLK1: Alias(s):
          PciRoot(0x0)/Pci(0x1,0x1)/Ata(0x0)/CDROM(0x0)
     BLK4: Alias(s):
          PciRoot(0x0)/Pci(0x3,0x0)/NVMe(0x1,00-00-00-00-00-00-00-00)
Press ESC in 1 seconds to skip startup.nsh or any other key to continue.
Shell>
```

## 一些已知问题
### nvme 作为启动盘，无法识别问题
https://gitlab.com/qemu-project/qemu/-/issues/665
似乎 seabios 原来是 virtio-scsi 启动，换成 nvme 启动之后，连 grub 都进不去了，真的服了
哦，原来是 nvme 不能作为启动盘，否则无法找到启动的

需要使用 ovmf 启动，然后进入 fs0: ，手动启动 nvme 盘才可以
但是之后就可以自动进入了，不需要使用在 uefi 中操作
```sh
arg_boot_img+=" -device nvme,drive=$id,serial=$(uuidgen)"
```

- [ ] 我还有点好奇，操作系统是如何修改 pflash 的

### windows 启动，默认进入到这个界面，而不是直接启动

```txt
UEFI Interactive Shell v2.2
EDK II
UEFI v2.70 (EDK II, 0x00010000)
Mapping table
      FS0: Alias(s):CD0a65535a1:;BLK2:
          PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0xFFFF,0x0)/CDROM(0x1)
      FS1: Alias(s):F0a65535a:;BLK3:
          PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0xFFFF,0x0)/VenMedia(C5BD4D42-1A76-4996-8956-73CDA326CD0A)
     BLK4: Alias(s):
          PciRoot(0x0)/Pci(0x2,0x0)/Scsi(0x1,0x0)
     BLK5: Alias(s):
          PciRoot(0x0)/Pci(0x3,0x0)
     BLK0: Alias(s):
          PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0xFFFF,0x0)
     BLK1: Alias(s):
          PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0xFFFF,0x0)/CDROM(0x0)

Press ESC in 4 seconds to skip startup.nsh or any other key to continue.
```

即便当时的配置是
```txt
	-device virtio-blk-pci,drive=boot1,bootindex=1 \
		-device virtio-blk-pci,drive=boot1,bootindex=1 \
```
这个容易 workaround ，就是手动启动一下就可以了

### uefi 的 bug ?
不然 遇到这个问题: https://www.reddit.com/r/openstack/comments/1g0ilp5/cant_boot_instance_uefi_pf_pagefault/
```txt
BdsDxe: loading Boot0001 "UEFI QEMU NVMe Ctrl 95aa0623-d0a4-438e-b 1" from PciRoot(0x0)/Pci(0x2,0x0)/NVMe(0x1,00-00-00-00-00-00-00-00)
BdsDxe: starting Boot0001 "UEFI QEMU NVMe Ctrl 95aa0623-d0a4-438e-b 1" from PciRoot(0x0)/Pci(0x2,0x0)/NVMe(0x1,00-00-00-00-00-00-00-00)
!!!! X64 Exception Type - 0E(#PF - Page-Fault)  CPU Apic ID - 00000000 !!!!
ExceptionData - 0000000000000009  I:0 R:1 U:0 W:0 P:1 PK:0 SS:0 SGX:0
RIP  - 00000000BEC447B0, CS  - 0000000000000038, RFLAGS - 0000000000010246
RAX  - 00000000BF8E2040, RCX - 0000000000000001, RDX - 0000000001040001
RBX  - 00000000001A6B28, RSP - 00000000001A6AF0, RBP - 00000000001A6D70
RSI  - 00000000BEC49AB0, RDI - 0000000000000000
R8   - 0000000000000000, R9  - 00000000BEC49AA0, R10 - 0000FFFFFFFFF000
R11  - 00000000001A6E90, R12 - 0000000000000001, R13 - 0000000000000001
R14  - 00000000BEA48898, R15 - 0000000001040001
DS   - 0000000000000030, ES  - 0000000000000030, FS  - 0000000000000030
GS   - 0000000000000030, SS  - 0000000000000030
CR0  - 0000000080010033, CR2 - 00000000BF8E2040, CR3 - 00000000BFA01000
CR4  - 0000000000040668, CR8 - 0000000000000000
DR0  - 0000000000000000, DR1 - 0000000000000000, DR2 - 0000000000000000
DR3  - 0000000000000000, DR6 - 00000000FFFF0FF0, DR7 - 0000000000000400
GDTR - 00000000BF7DC000 0000000000000047, LDTR - 0000000000000000
IDTR - 00000000BF1D2018 0000000000000FFF,   TR - 0000000000000000
FXSAVE_STATE - 00000000001A6750
!!!! Find image based on IP(0xBEC447B0) /builddir/build/BUILD/edk2-3e722403cd/Build/OvmfX64/DEBUG_GCC5/X64/MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe/DEBUG/TerminalDxe.dll (ImageBase=00000000BEC42000, EntryPoint=00000000BEC47BFB) !!!!
QEMU: Terminated
```
才发现，之前安装的时候，一直用的都是 -machine q35,accel=kvm,smm=on
之后，只要调换过 -machine 的参数之后，就无法启动了，调换回来也没用。

有趣，如果是非 secure 的 ovmf 启动嗯，然后使用 secure ovmf 启动，会有这个问题
```txt
!!!! X64 Exception Type - 0E(#PF - Page-Fault)  CPU Apic ID - 00000000 !!!!
ExceptionData - 0000000000000009  I:0 R:1 U:0 W:0 P:1 PK:0 SS:0 SGX:0
RIP  - 000000007ECFF586, CS  - 0000000000000038, RFLAGS - 0000000000010006
RAX  - 000000007DF84AA0, RCX - 000000007E7E00E0, RDX - 00000000001A68EF
RBX  - 0000000000000010, RSP - 00000000001A68C0, RBP - 00000000001A68F0
RSI  - 000000000038A000, RDI - 0000000000000001
R8   - ABD7F02E4201DB9E, R9  - 000000000038EFFF, R10 - 0000000000000000
R11  - 0000000000000005, R12 - 000000007ED17B74, R13 - 0000000000000001
R14  - 000000000002600F, R15 - 000000007CDCACC0
DS   - 0000000000000030, ES  - 0000000000000030, FS  - 0000000000000030
GS   - 0000000000000030, SS  - 0000000000000030
CR0  - 0000000080010033, CR2 - 000000007E7E00E0, CR3 - 000000007EA01000
CR4  - 0000000000040668, CR8 - 0000000000000000
DR0  - 0000000000000000, DR1 - 0000000000000000, DR2 - 0000000000000000
DR3  - 0000000000000000, DR6 - 00000000FFFF0FF0, DR7 - 0000000000000400
GDTR - 000000007E6F6000 0000000000000047, LDTR - 0000000000000000
IDTR - 000000007DFB4018 0000000000000FFF,   TR - 0000000000000000
FXSAVE_STATE - 00000000001A6520
!!!! Find image based on IP(0x7ECFF586) /builddir/build/BUILD/edk2-3e722403cd/Build/OvmfX64/DEBUG_GCC5/X64/MdeModulePkg/Core/Dxe/DxeMain/DEBUG/DxeCore.dll (ImageBase=000000007ECF7000, EntryPoint=000000007ED0C894) !!!!
```

### 启动的时候，出现 cdrom 加载 timeout ，无论是否配置 secure boot

```txt
BdsDxe: loading Boot0001 "UEFI QEMU QEMU CD-ROM " from PciRoot(0x0)/Pci(0x7,0x0)/Scsi(0x14,0x1)
BdsDxe: failed to start Boot0001 "UEFI QEMU QEMU CD-ROM " from PciRoot(0x0)/Pci(0x7,0x0)/Scsi(0x14,0x1): Time out
```
应该不是 secure boot 的问题，而是使用

## 如果用 seabios 安装的，就不能用 uefi 启动了

用 seabios 安装的环境，没有 efi 分区:

这是典型的 uefi 的启动盘:
```txt
nvme0n1     259:0    0   1.5T  0 disk
├─nvme0n1p1 259:1    0   512M  0 part /boot/efi
├─nvme0n1p2 259:2    0     5G  0 part /boot
├─nvme0n1p3 259:3    0   185G  0 part /
```
```txt
root@localhost:~# lsblk
NAME            MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
sda               8:0    0    10G  0 disk
sdb               8:16   0    10G  0 disk
zram0           251:0    0   7.7G  0 disk [SWAP]
vda             253:0    0   350G  0 disk
├─vda1          253:1    0   600M  0 part /boot/efi
├─vda2          253:2    0     2G  0 part /boot
└─vda3          253:3    0 347.4G  0 part
  └─fedora-root 252:0    0    15G  0 lvm  /
```

这是 bios 的启动的环境
```txt
vda    253:0    0  350G  0 disk
├─vda1 253:1    0    5G  0 part /boot
└─vda2 253:2    0  185G  0 part /
```
所以，总体来说，这是预期的，为什么会存在一个印象可以
实现一个虚拟机两种状态都可以切换的了，那是用的 -kernel 吧，
其实那个时候，根本就没有 bios 什么事情了。

使用 fdisk -l /dev/nvme0n1 来检查
```txt
🧀  sudo fdisk -l /dev/nvme0n1
[sudo] password for martins3:
Disk /dev/nvme0n1: 3.64 TiB, 4000787030016 bytes, 7814037168 sectors
Disk model: Fanxiang S790 4TB
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: gpt
Disk identifier: 06C70350-563A-4C0D-B496-7E7AF2BB248C

Device           Start        End    Sectors  Size Type
/dev/nvme0n1p1    2048    1230847    1228800  600M EFI System
/dev/nvme0n1p2 1230848    3327999    2097152    1G Linux extended boot
/dev/nvme0n1p3 3328000 7814035455 7810707456  3.6T Linux LVM
```

```txt
🤒  sudo fdisk -l /dev/vda
[sudo] password for martins3:
Disk /dev/vda: 350 GiB, 375809638400 bytes, 734003200 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: gpt
Disk identifier: 94765132-A540-452B-9A1C-0903B1A786F3

Device       Start       End   Sectors  Size Type
/dev/vda1     2048      4095      2048    1M BIOS boot
/dev/vda2     4096   2101247   2097152    1G Linux extended boot
/dev/vda3  2101248 734001151 731899904  349G Linux LVM
```

## 关于 qemu 的问题

这个目录 qemu roms/ 看看

似乎其中的内容都是 qemu 支持的 bios 的
```txt
 openbios
 opensbi
 qboot
 qemu-palcode
 QemuMacDrivers
 seabios
 seabios-hppa
 skiboot
 SLOF
 u-boot
 u-boot-sam460ex
 vbootrom
```
而且这里已经有 roms/edk2-build.config 的构建方法。

此外，在 pc-bios 有现成编译好的内容

-boot once=d 这个参数什么意思

arg_vfio+="-device vfio-pci,host=$dx,bootindex=1" 看看是如何解析的，他都不知道这个 pci 是一个盘

```txt
🧀  qemu-system-x86_64 -device vfio-pci,help
vfio-pci options:
  acpi-index=<uint32>    -  (default: 0)
  addr=<int32>           - Slot and optional function number, example: 06.
0 or 06 (default: -1)
  bootindex=<int32>
```

看看 man qemu(1) -boot 参数在 bios 中是如何实现的

这里提示说，不可以让 bootindex 和 disk 的同时配置，如何理解?

> Specify boot order drives as a  string  of  drive  letters.
> Valid  drive letters depend on the target architecture. The
> x86 PC uses: a, b (floppy 1 and 2), c (first hard disk),  d
> (first  CD-ROM),  n-p (Etherboot from network adapter 1-4),
> hard disk boot is the default.  To apply a particular  boot
> order  only on the first startup, specify it via once. Note
> that the order or once parameter should  not  be  used  to‐
> gether  with  the  bootindex property of devices, since the
> firmware implementations normally do not  support  both  at
> the same time.


> Note: The legacy format '-boot drives' is still supported but its use is discouraged as it may be removed from future versions.

所以 bootindex 其实按道理是记录到 rom 中的吧

这两个写法的展开都是什么意思:
```txt
arg_boot_img+="-cdrom $iso"
hda 到底是什么，现在一直有一个迷信，有些地方只能使用 -hda ，但是应该是 bootindex 导致的吧
```

### -boot menu=on,strict=on 作用是什么

> [!NOTE]
> 参考神奇海螺的意见，有待验证

(这么看，UEFI 原来是没用的)

以下参数属于 QEMU 的 `-boot` 子选项，用于控制 BIOS/固件启动流程的行为。具体含义如下：

1. `-boot menu=on`

启用 QEMU 的 **BIOS 启动菜单**（Boot Menu）。
不同固件的表现略有差异：

* **SeaBIOS**：在启动阶段显示一个 “Boot Menu” 界面（通常按 F12），列出可用的启动设备，允许手动选择。
* **OVMF (UEFI)**：该参数对 UEFI 并不影响启动菜单（UEFI 自带 Boot Manager），但仍被解析，不会报错。

作用：
在启动时让用户能够手动选择从哪个设备启动，便于调试或在多个镜像之间切换。

2. `-boot strict=on`

开启更严格的启动设备检查模式。

其功能是：
如果 QEMU 找不到你指定的启动设备（例如 `bootorder=c`），**则直接报错退出**，而不是尝试自动从其他设备启动。

举例说明：

```
-boot order=c,strict=on
```

如果没有任何可启动的 disk 被 QEMU 识别，那么它会立刻报错，而不是 fallback 到 PXE/其他设备。

作用是确保启动行为完全可控，在自动化环境（CI、测试框架、固定依赖链）很常见。

| 参数        | 作用                                      |
| ----------- | ----------------------------              |
| `menu=on`   | 显示 BIOS 启动菜单（主要对 SeaBIOS 有效） |
| `strict=on` | 强制严格匹配启动设备；缺失即报错退出      |

### if-none 是什么意思

```sh
arg_sata="-device virtio-scsi-pci,id=scsi -device scsi-hd,drive=jj,bootindex=10 -drive file=${workstation}/img4,format=raw,id=jj"
```

```txt
(qemu) qemu-system-x86_64: -device scsi-hd,drive=jj,bootindex=10: Drive 'jj' is already in use because it has been automatically connected to another
device (did you need 'if=none' in the drive options?)
```
这里的 if=none 是什么意思？

### 那么 drive 上的 index 和 device 上的 index 是什么关系?
arg_boot_img+=" -drive file=$disk_path,format=$format,id=$id,if=none$aio,media=disk,index=3"

### 最后这两个看看
- docs/qemu/cdrom.md
- https://en.wikipedia.org/wiki/Multiboot_specification


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
