## TODO
- [UEFI 引导与 BIOS 引导在原理上有什么区别？](https://www.zhihu.com/question/21672895/answer/774538058)

## 那么设备的固件和 option rom 是什么关系?

option rom 可以被驱动修改吗?

## 固件和 os 是如何交互的?
是 os 可以调用固件的函数，还是固件的执行 os 完全看不到

## 查看网卡的 firmware 的 版本
```txt
🤒  ethtool -i wlo1
driver: iwlwifi
version: 6.5.3
firmware-version: 81.31fc9ae6.0 so-a0-hr-b0-81.uc
expansion-rom-version:
bus-info: 0000:00:14.3
supports-statistics: yes
supports-test: no
supports-eeprom-access: no
supports-register-dump: no
supports-priv-flags: no
```
输出的具体含义需要看看内核驱动的实现，


## 服了 UEIF 了
https://news.ycombinator.com/item?id=39024228

## 是否可以测试用 QEMU 测试


## x710 的网卡

```txt
[3807026.791533] i40e 0000:3d:00.0: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[3807026.816139] i40e 0000:3d:00.2: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[3807026.885435] i40e 0000:3d:00.3: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[3807027.573635] i40e 0000:3d:00.1: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[3811177.282360] i40e 0000:3d:00.0: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[3811177.321289] i40e 0000:3d:00.2: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[3811177.370360] i40e 0000:3d:00.3: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[3811177.984385] i40e 0000:3d:00.1: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[4067914.664588] i40e 0000:3d:00.0: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[4067914.721973] i40e 0000:3d:00.2: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[4067914.792383] i40e 0000:3d:00.3: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[4067915.510916] i40e 0000:3d:00.1: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[4412725.986790] i40e 0000:3d:00.0: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[4412726.056689] i40e 0000:3d:00.2: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[4412726.118981] i40e 0000:3d:00.3: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
[4412726.830695] i40e 0000:3d:00.1: Use ethtool to disable LLDP firmware agent:"ethtool --set-priv-flags <interface> disable-fw-lldp on".
```
这个 lldp 为什么可以在固件中做，也可以在 os 做

## 为什么固件可以控制 cstate 和 pstate 的状态

## 看看这里
https://hn.algolia.com/?q=firmware
https://hn.algolia.com/?q=uefi

## Linux Boot : 虽然没维护了，但是还可以吧
- https://news.ycombinator.com/item?id=16259373

## 这个问题分析下
```txt
[    5.029135] nvme 0000:02:00.0: VPD access failed.  This is likely a firmware bug on this device.  Contact the card vendor for a firmware update
[    5.029139] nvme 0000:02:00.0: failed VPD read at offset 1
```

- [ ] nvme 也有 vpn ?
- [ ] 如何给这个 nvme 刷固件 ?


## qemu romfile
https://gist.github.com/mcastelino/7ab9dba51b0dbb230bd18c448d935312


qemu 日志:
```txt
virtio-net-pci.rom
```

- https://forum.proxmox.com/threads/problems-with-ipxe-pxe-virtio-rom-and-https.57326/

## mps3sas 可以让固件的日志放到 kernel 中
这是如何实现的

## 忽然有一个非常的想法，使用 esp32 来学习固件开发

### 理解下 micropython 是如何工作的吧
将代码发送给刚刚烧的固件，然后板子解释执行?

- [ ] 这个可以在 qemu 中测试，先用 qemu 测试吧

## 这个需要看看
https://lwn.net/Articles/738649/

## 从这里开始学习 firmware 吧
- https://doc.coreboot.org/tutorial/part1.html

## 可以 qemu 测试使用 SMM 测试吗?

SMI 中断


## 可以用 Linux 调用 smm 吗?

## 看看这个东西
https://github.com/daliansky/XiaoMi-Pro-Hackintosh

到底如何刷 bios

## 有趣
https://retrage.github.io/about/

似乎他的每一个项目和 talk 都可以看看

> Practical Rust (Hypervisor) Firmware Kernel/VM Online Part 3, July 10, 2021

## 有趣的
https://news.ycombinator.com/item?id=41186862

## 啊，这个到底实现了什么啊?
https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=de6582833db0

## 也许调查下 kvm 下的 smm 如何使用

## RISC-V 的 openSBI
<!-- e0c22e00-b195-4614-b350-27d52fb8a791 -->

https://github.com/riscv-software-src/opensbi

> [!NOTE]
> 参考神奇海螺的意见，有待验证

在 x86 世界中：它的功能类似于 BIOS/UEFI 加上 SMM（系统管理模式）。
在 ARM 世界中：它非常类似于 ATF (Arm Trusted Firmware)。

## 了解下
__ghes_print_estatus

## 新的想法
1. 了解芯片的支持吧: 例如 el3 ，smi ，mmi 驱动的支持
  - mtrr
  - CONFIG_MTRR_SANITIZER=y 的这个问题
2. 分析一下

## 内核打印这个日志是做什么
- https://serverfault.com/questions/708447/probing-edd-boot-message-stays-for-ten-minutes-on-centos-6-6

## 这个太好了
- [老狼:UEFI 和 BIOS 探秘](https://www.zhihu.com/column/UEFIBlog) :  信息密度不高

## 为什么要去 hacking 固件

1. 网卡 vfio reset 导致 kernel crash 的
3. firmware 控制的 pstate 和 cstate
4. SMM 和 SMI 的虚拟化
5. nvme 固件导致系统低噪的
6. 盘慢，也是固件的原因

## hdparm 刷固件是如何操作的
https://salsa.debian.org/debian/hdparm.git/

通过 sg 来实现的。

HDIO_DRIVE_TASKFILE

进一步可以参考这个: Documentation/userspace-api/ioctl/hdio.rst

## 终于知道我们的问题是什么了
既然有 driver 和 UEFI ，为什么还需要设备中的固件?

## 这里是怎么回事
```txt
[   46.263106] usb 1-1: new high-speed USB device number 2 using xhci_hcd
[   46.384341] SDEI NMI watchdog: Bind interrupt failed. Firmware may not support SDEI !
[   46.386678] integrity: Unable to open file: /etc/keys/x509_ima.der (-2)
[   46.386680] integrity: Unable to open file: /etc/keys/x509_evm.der (-2)
[   46.388586] Freeing unused kernel memory: 4608K
```
## 网卡和 firmware 沟通，firmware 的能力

```txt
History:        #0
Commit:         8d4bd96b54dcb5997d1035f4dfd300c04d07ec11
Author:         Michael Chan <michael.chan@broadcom.com>
Committer:      David S. Miller <davem@davemloft.net>
Author Date:    Mon 05 Oct 2020 03:23:01 AM CST
Committer Date: Mon 05 Oct 2020 05:41:05 AM CST

bnxt_en: Eliminate unnecessary RX resets.

Currently, the driver will schedule RX ring reset when we get a buffer
error in the RX completion record.  These RX buffer errors can be due
to normal out-of-buffer conditions or a permanent error in the RX
ring.  Because the driver cannot distinguish between these 2
conditions, we assume all these buffer errors require reset.

This is very disruptive when it is just a normal out-of-buffer
condition.  Newer firmware will now monitor the rings for the permanent
failure and will send a notification to the driver when it happens.
This allows the driver to reset only when such a notification is
received.  In environments where we have predominently out-of-buffer
conditions, we now can avoid these unnecessary resets.

Reviewed-by: Edwin Peer <edwin.peer@broadcom.com>
Signed-off-by: Michael Chan <michael.chan@broadcom.com>
Signed-off-by: David S. Miller <davem@davemloft.net>
```

## firmware 中来做图形
- https://github.com/adi1090x/plymouth-themes
- https://gitlab.freedesktop.org/plymouth/plymouth
  - 才意识到，这个图形库居然是依赖 seabios / UEFI 运行的，至少是受影响的

## flint 的代码是开源的
https://github.com/Mellanox/mstflint/blob/master/kernel/mst_kernel.h


## 疑惑的东西
1. 如果 nvme 的固件会导致底噪，那么意味着 CPU 会去执行 nvme 的固件，
但是 nvme 的固件是架构无关的吧
3. 如果 BIOS 中关闭了 vmx ，可以在 os 中重新打开吗?

## efibootmgr 源码看看，他最后是操作哪里

```txt
efibootmgr version 17
usage: efibootmgr [options]
        -a | --active         sets bootnum active
        -A | --inactive       sets bootnum inactive
        -b | --bootnum XXXX   modify BootXXXX (hex)
        -B | --delete-bootnum delete bootnum
        -c | --create         create new variable bootnum and add to bootorder
        -C | --create-only      create new variable bootnum and do not add to bootorder
        -D | --remove-dups      remove duplicate values from BootOrder
        -d | --disk disk       (defaults to /dev/sda) containing loader
        -r | --driver         Operate on Driver variables, not Boot Variables.
        -e | --edd [1|3|-1]   force EDD 1.0 or 3.0 creation variables, or guess
        -E | --device num      EDD 1.0 device number (defaults to 0x80)
        -g | --gpt            force disk with invalid PMBR to be treated as GPT
        -i | --iface name     create a netboot entry for the named interface
        -l | --loader name     (defaults to "\EFI\openEuler\grub.efi")
        -L | --label label     Boot manager display label (defaults to "Linux")
        -m | --mirror-below-4G t|f mirror memory below 4GB
        -M | --mirror-above-4G X percentage memory to mirror above 4GB
        -n | --bootnext XXXX   set BootNext to XXXX (hex)
        -N | --delete-bootnext delete BootNext
        -o | --bootorder XXXX,YYYY,ZZZZ,...     explicitly set BootOrder (hex)
        -O | --delete-bootorder delete BootOrder
        -p | --part part        partition containing loader (defaults to 1 on partitioned devices)
        -q | --quiet            be quiet
        -t | --timeout seconds  set boot manager timeout waiting for user input.
        -T | --delete-timeout   delete Timeout.
        -u | --unicode | --UCS-2  handle extra args as UCS-2 (default is ASCII)
        -v | --verbose          print additional information
        -V | --version          return version and exit
        -w | --write-signature  write unique sig to MBR if needed
        -y | --sysprep          Operate on SysPrep variables, not Boot Variables.
        -@ | --append-binary-args file  append extra args from file (use "-" for stdin)
        -h | --help             show help/usage
```
而且这里看到了我们的好朋友 EDD 了

## UEFI EmulatorPkg

这个师傅阿
```txt
/home/martins3/core/edk2/EmulatorPkg/Unix/Host/X11GraphicsWindow.c:18:10: fatal error: X11/extensions/XShm.h: No such file or direct
ory
   18 | #include <X11/extensions/XShm.h>
      |          ^~~~~~~~~~~~~~~~~~~~~~~
```
edk2 提供了模拟软件，所以有 X11 的接口。

## 进入到 grub shell 中，ls ，发现有一个目录是 proc

## 难道 qemu 中 -kernel 参数依赖这里吗?
OvmfPkg/QemuKernelLoaderFsDxe/QemuKernelLoaderFsDxe.c

## 这个是啥
https://news.ycombinator.com/item?id=41467420

## 这里的 qemu_fw_cfg 是哪里的代码控制的
```txt
ls /sys/firmware/

acpi  dmi  memmap  qemu_fw_cfg
```

## kernel 中 drivers/firmware/ 内容简单看看


## 看看使用 efi 启动的时候，这个目录的差别是什么?
```txt
➜  efi ls -la
total 0
drwxr-xr-x 4 root root    0 Aug 31 11:07 .
drwxr-xr-x 6 root root    0 Aug 31 11:07 ..
-r--r--r-- 1 root root 4096 Aug 31 11:09 config_table
dr-xr-xr-x 2 root root    0 Aug 31 11:07 efivars
-r--r--r-- 1 root root 4096 Aug 31 11:09 fw_platform_size
-r--r--r-- 1 root root 4096 Aug 31 11:09 fw_vendor
-r--r--r-- 1 root root 4096 Aug 31 11:09 runtime
drwxr-xr-x 8 root root    0 Aug 31 11:09 runtime-map
-r-------- 1 root root 4096 Aug 31 11:09 systab
```
这里的内容也是需要解析分析下的

## qboot 到底简化了什么?

## 看看是如何调用 efi 的

```txt
#3  efi_call_rts (work=<optimized out>) at drivers/firmware/efi/runtime-wrappers.c:223
#4  0xffffffff811b74bc in process_one_work (worker=0xffff8881003bfbc0, work=0xffffffff84fc64e0 <efi_rts_work+16>) at kernel/workqueue.c:3231
#5  process_scheduled_works (worker=0xffff8881003bfbc0) at kernel/workqueue.c:3312
#6  0xffffffff811b9858 in worker_thread (__worker=0xffff8881003bfbc0) at kernel/workqueue.c:3393
#7  0xffffffff811c02c8 in kthread (_create=0xffff888100fe4b40) at kernel/kthread.c:389
#8  0xffffffff81111eb7 in ret_from_fork (prev=<optimized out>, regs=0xffffc90000837f58, fn=0xffffffff811c01d0 <kthread>, fn_arg=0xffff888100fe4b40) at arch/x86/kernel/process.c:147
#9  0xffffffff8100274a in ret_from_fork_asm () at /home/martins3/data/linux-build/arch/x86/entry/entry_64.S:244
#10 0x0000000000000000 in ?? ()
```

似乎是调用的这个:
```c
typedef union {
	struct {
		efi_table_hdr_t				hdr;
		efi_get_time_t __efiapi			*get_time;
		efi_set_time_t __efiapi			*set_time;
		efi_get_wakeup_time_t __efiapi		*get_wakeup_time;
		efi_set_wakeup_time_t __efiapi		*set_wakeup_time;
		efi_set_virtual_address_map_t __efiapi	*set_virtual_address_map;
		void					*convert_pointer;
		efi_get_variable_t __efiapi		*get_variable;
		efi_get_next_variable_t __efiapi	*get_next_variable;
		efi_set_variable_t __efiapi		*set_variable;
		efi_get_next_high_mono_count_t __efiapi	*get_next_high_mono_count;
		efi_reset_system_t __efiapi		*reset_system;
		efi_update_capsule_t __efiapi		*update_capsule;
		efi_query_capsule_caps_t __efiapi	*query_capsule_caps;
		efi_query_variable_info_t __efiapi	*query_variable_info;
	};
	efi_runtime_services_32_t mixed_mode;
} efi_runtime_services_t;
```

这个环境初始化的地方:
```txt
#0  efi_native_runtime_setup () at drivers/firmware/efi/runtime-wrappers.c:557
#1  0xffffffff84b54e06 in __efi_enter_virtual_mode () at arch/x86/platform/efi/efi.c:864
#2  0xffffffff84b54b69 in efi_enter_virtual_mode () at arch/x86/platform/efi/efi.c:893
#3  0xffffffff84b2584a in start_kernel () at init/main.c:1075
#4  0xffffffff84b33054 in x86_64_start_reservations (real_mode_data=0xbc53e000 <error: Cannot access memory at address 0xbc53e000>)
    at arch/x86/kernel/head64.c:507
#5  0xffffffff84b32f51 in x86_64_start_kernel (real_mode_data=0xbc53e000 <error: Cannot access memory at address 0xbc53e000>)
    at arch/x86/kernel/head64.c:488
#6  0xffffffff810fe477 in secondary_startup_64 () at /home/martins3/data/linux-build/arch/x86/kernel/head_64.S:420
#7  0x0000000000000000 in ?? ()
```

好吧，先停停吧，但是这个是一个突破口，就是彻底理解内核如何调用 UEFI 的。

## 写的好详细，真棒
https://github.com/tc-homework/TC_UEFI_Project-2048

## 直接解析内核
https://github.com/awslabs/python-uefivars/tree/v1.2

内核中有 fs/efivarfs/ ，那么 uefi 中是不是有类似的结构


## 原来 firmweare 可以是 ELF 格式的
🧀  file gsp_ga10x.bin
gsp_ga10x.bin: ELF 64-bit LSB relocatable, UCB RISC-V, soft-float ABI, version 1 (SYSV), not stripped

大致找了一下其中的内容，有如下:
```txt
./iwlwifi-5000-1.ucode:                                       0420 Alliant virtual executable not stripped
./ti-connectivity/wl127x-nvs.bin:                             88K BCS executable
./bnx2x-e1-4.8.53.0.fw:                                       Adobe Photoshop Color swatch, version 0, 10336 colors; 1st RGB space (0), w 0x60, x 0, y 0x630, z 0; 2nd space (10440), w 0, x 0x1608, y 0, z 0x2f00
./amdgpu/sienna_cichlid_ta.bin:                               Apple DiskCopy 4.2 image , 6356992 bytes, 0x3000000 tag size, GCR CLV ssdd (400k), 0x11 format
./slicoss/gbrcvucode.sys:                                     Atari DEGAS Elite bitmap 640 x 400 x 2, color palette 0000 4775 0100 04a0 1301 ...
./dpaa2/mc/mc_10.10.0_ls2088a.itb:                            Device Tree Blob version 17, size=1012808, boot CPU=0, string block size=80, DT structure block size=1012220
./amdgpu/navi10_me.bin:                                       TTComp archive data, binary, 2K dictionary
```

## 这个看看
https://docs.kernel.org/devicetree/index.html

## fwupdmgr
在 n100 的机器上报告这个错误了:
```txt
🧀  fwupdmgr get-upgrades
Devices with no available firmware updates:
 • Fanxiang S500Pro 256GB
 • Intel Management Engine
 • UEFI Device Firmware
 • UEFI Device Firmware
 • UEFI Device Firmware
Micro Computer (HK) Tech Limited Venus Series
│
└─UEFI dbx:
  │   Device ID:          362301da643102b9f38477387e2193e57abaa590
  │   Summary:            UEFI revocation database
  │   Current version:    217
  │   Minimum Version:    217
  │   Vendor:             UEFI:Linux Foundation
  │   Install Duration:   1 second
  │   GUIDs:              043fbcc4-4cee-5f4e-9764-e9adda8c2efa ← UEFI\CRT_4990AE1C1412A306D8B604937AE69902BF9CB3A9B2BF3B9B29DD8C4C556CDE15&ARCH_X64
  │                       d07ff664-b0e1-5f4e-a723-d7fbcbfcb94f ← UEFI\CRT_3CD3F0309EDAE228767A976DD40D9F4AFFC4FBD5218F2E8CC3C9DD97E8AC6F9D&ARCH_X64
  │                       f8ba2887-9411-5c36-9cee-88995bb39731 ← UEFI\CRT_A1117F516A32CEFCBA3F2D1ACE10A87972FD6BBE8FE0D0B996E09E65D802A503&ARCH_X64
  │   Device Flags:       • Internal device
  │                       • Updatable
  │                       • Supported on remote server
  │                       • Needs a reboot after installation
  │                       • Device is usable for the duration of the update
  │                       • Only version upgrades are allowed
  │                       • Signed Payload
  │
  ├─Secure Boot dbx Configuration Update:
  │     New version:      371
  │     Remote ID:        lvfs
  │     Release ID:       35287
  │     Summary:          UEFI Secure Boot Forbidden Signature Database
  │     Variant:          x64
  │     License:          Proprietary
  │     Size:             21.2 kB
  │     Created:          2023-05-09
  │     Urgency:          High
  │     Tested by Lenovo:
  │       Tested:         2024-11-20
  │       Distribution:   fedora 39 (workstation)
  │       Old version:    217
  │       Version[fwupd]: 1.9.20
  │     Tested by Wistron:
  │       Tested:         2024-06-06
  │       Distribution:   ubuntu 22.04
  │       Old version:    267
  │       Version[fwupd]: 1.9.15
  │     Tested by HP:
  │       Tested:         2024-06-04
  │       Distribution:   fedora 39 (workstation)
  │       Old version:    77
  │       Version[fwupd]: 1.9.20
  │     Tested by HP:
  │       Tested:         2024-04-24
  │       Distribution:   ubuntu 22.04
  │       Old version:    83
  │       Version[fwupd]: 1.9.16
  │     Tested by HP:
  │       Tested:         2024-03-21
  │       Distribution:   fedora 39 (workstation)
  │       Old version:    217
  │       Version[fwupd]: 1.9.15
  │     Tested by Lenovo:
  │       Tested:         2024-02-20
  │       Distribution:   fedora 39 (workstation)
  │       Old version:    77
  │       Version[fwupd]: 1.9.5
  │     Tested by Lenovo:
  │       Tested:         2024-01-12
  │       Distribution:   fedora 39 (workstation)
  │       Old version:    220
  │       Version[fwupd]: 1.9.11
  │     Tested by DMC Group:
  │       Tested:         2023-07-11
  │       Distribution:   fedora 38 (workstation)
  │       Old version:    211
  │       Version[fwupd]: 1.9.2
  │     Tested by Jabra:
  │       Tested:         2023-07-03
  │       Distribution:   ubuntu 22.04
  │       Old version:    220
  │       Version[fwupd]: 1.9.3
  │     Vendor:           Linux Foundation
  │     Duration:         1 second
  │     Release Flags:    • Trusted metadata
  │                       • Is upgrade
  │     Description:
  │     Insecure versions of the Microsoft Windows boot manager affected by Black Lotus were added to the list of forbidden signatures due to a discovered security problem.This updates the dbx to the latest release from Microsoft.
  │
  │     Before installing the update, fwupd will check for any affected executables in the ESP and will refuse to update if it finds any boot binaries signed with any of the forbidden signatures.Applying this update may also cause some Windows install media to not start correctly.
  │     Issue:            CVE-2022-21894
  │     Checksum:         fc3feb015df2710fcfa07583d31b5975ee398357016699cfff067f422ab91e13
  │
  └─Secure Boot dbx Configuration Update:
        New version:      220
        Remote ID:        lvfs
        Release ID:       28499
        Summary:          UEFI Secure Boot Forbidden Signature Database
        Variant:          x64
        License:          Proprietary
        Size:             13.9 kB
        Created:          2023-03-14
        Urgency:          High
        Vendor:           Linux Foundation
        Duration:         1 second
        Release Flags:    • Trusted metadata
                          • Is upgrade
        Description:
        Insecure versions of software from Trend Micro, vmware, CPSD, Eurosoft, and New Horizon Datasys Inc were added to the list of forbidden signatures due to discovered security problems.This updates the dbx to the latest release from Microsoft.

        Before installing the update, fwupd will check for any affected executables in the ESP and will refuse to update if it finds any boot binaries signed with any of the forbidden signatures.
        Issue:            CVE-2023-28005
        Checksum:         4016aca8f305115c1015c9c6079d855832f8ae8cc231f412990e4e8f9ed1bfc2
```

## https://news.ycombinator.com/item?id=43397811

## 2025-10-15 记录的日志

```txt
🧀  sudo fwupdmgr get-upgrades
[sudo] password for martins3:
Devices with no available firmware updates:
 • ASUSTeK KEK Certificate
 • ASUSTeK SW Key Certificate
 • Fanxiang S790 4TB
 • KEK CA
 • Master Certificate Authority
 • Master Certificate Authority
 • System Firmware
 • UEFI Device Firmware
 • UEFI Device Firmware
 • WD20EZBX-00AYRA0
 • Windows Production PCA
 • ZHITAI SC001 Active 512GB SSD
 • ZHITAI TiPlus7100 1TB
 • ZHITAI TiPro7000 1TB
ASUS System Product Name
│
├─UEFI CA:
│ │   Device ID:          5bc922b7bd1adb5b6f99592611404036bd9f42d0
│ │   Current version:    2011
│ │   Vendor:             Microsoft (UEFI:Microsoft)
│ │   GUIDs:              26f42cba-9bf6-5365-802b-e250eb757e96 ← UEFI\VENDOR_Microsoft&NAME_Microsoft-UEFI-CA
│ │                       c34a7e6a-bd86-5244-8bd0-7db66fd3c073 ← UEFI\CRT_E30CF09DABEAB32A6E3B07A7135245DE05FFB658
│ │   Device Flags:       • Internal device
│ │                       • Updatable
│ │                       • Supported on remote server
│ │                       • Needs a reboot after installation
│ │                       • Signed Payload
│ │                       • Can tag for emulation
│ │
│ └─Secure Boot Signature Database Configuration Update:
│       New version:      2023
│       Remote ID:        lvfs
│       Release ID:       116503
│       Summary:          UEFI Secure Boot Signature Database
│       License:          Proprietary
│       Size:             10.0 kB
│       Created:          2025-04-29 00:00:00
│       Urgency:          High
│         Tested:         2025-09-17 00:00:00
│         Distribution:   fedora 42 (workstation)
│         Old version:    2011
│         Version[fwupd]: 2.0.16
│         Tested:         2025-07-24 00:00:00
│         Distribution:   nixos 25.11
│         Old version:    2011
│         Version[fwupd]: 2.0.12
│       Vendor:           Linux Foundation
│       Release Flags:    • Trusted metadata
│                         • Is upgrade
│       Description:
│       This updates the 3rd Party UEFI Signature Database (the "db") to the latest release from Microsoft. It also adds the latest OptionROM UEFI Signature Database update.
│       Checksum:         6819c8098f09f4332a102194df6a033563aa288073b16315c5b88860fefb7e74
│
└─UEFI dbx:
  │   Device ID:          362301da643102b9f38477387e2193e57abaa590
  │   Summary:            UEFI revocation database
  │   Current version:    20220801
  │   Minimum Version:    20220801
  │   Vendor:             UEFI:Microsoft
  │   Install Duration:   1 second
  │   GUIDs:              f8ff0d50-c757-5dc3-951a-39d86e16f419 ← UEFI\CRT_D7F66BE77CEF858C174BF4338A99263C8795B74E02026411F5F532F716AE3263&ARCH_X64
  │                       f8ba2887-9411-5c36-9cee-88995bb39731 ← UEFI\CRT_A1117F516A32CEFCBA3F2D1ACE10A87972FD6BBE8FE0D0B996E09E65D802A503&ARCH_X64
  │                       0c7691e1-b6f2-5d71-bc9c-aabee364c916 ← UEFI\CRT_ED1FE72CB9CA31C9AF5B757AFCD733323D675825032E6CED7FE1AE9EB767998C&ARCH_X64
  │   Device Flags:       • Internal device
  │                       • Updatable
  │                       • Supported on remote server
  │                       • Needs a reboot after installation
  │                       • Device is usable for the duration of the update
  │                       • Only version upgrades are allowed
  │                       • Signed Payload
  │                       • Can tag for emulation
  │
  ├─Secure Boot dbx Configuration Update:
  │     New version:      20250507
  │     Remote ID:        lvfs
  │     Release ID:       115586
  │     Summary:          UEFI Secure Boot Forbidden Signature Database
  │     Variant:          x64
  │     License:          Proprietary
  │     Size:             24.0 kB
  │     Created:          2025-01-17 00:00:00
  │     Urgency:          High
  │       Tested:         2025-06-11 00:00:00
  │       Distribution:   fedora 42 (workstation)
  │       Old version:    20241101
  │       Version[fwupd]: 2.0.11
  │     Vendor:           Linux Foundation
  │     Duration:         1 second
  │     Release Flags:    • Trusted metadata
  │                       • Is upgrade
  │                       • Tested by trusted vendor
  │     Description:
  │     This updates the list of forbidden signatures (the "dbx") to the latest release from Microsoft.
  │
  │     Some insecure versions of BiosFlashShell and Dtbios by DT Research Inc were added, due to a security vulnerability that allowed an attacker to bypass UEFI Secure Boot.
  │     Issues:           806555
  │                       CVE-2025-3052
  │     Checksum:         40d3a4630619b83026f66bc64d97a582bbd9223ad53aa3f519ff5e2121d11ca6
  │
  └─Secure Boot dbx Configuration Update:
        New version:      20241101
        Remote ID:        lvfs
        Release ID:       105821
        Summary:          UEFI Secure Boot Forbidden Signature Database
        Variant:          x64
        License:          Proprietary
        Size:             15.1 kB
        Created:          2025-01-17 00:00:00
        Urgency:          High
        Vendor:           Linux Foundation
        Duration:         1 second
        Release Flags:    • Trusted metadata
                          • Is upgrade
        Description:
        This updates the list of forbidden signatures (the "dbx") to the latest release from Microsoft.

        An insecure version of Howyar's SysReturn software was added, due to a security vulnerability that allowed an attacker to bypass UEFI Secure Boot.
        Issues:           529659
                          CVE-2024-7344
        Checksum:         093e6913dfecefbdaa9374a2e1caee7bf7e74c7eda847624e456e344884ba5f6
```

## sysfs 中 firmware_node 是做什么的?

## 原来还存在 firmware 模拟器
https://support.xfusion.com/server-simulators/bios-demo/purley/v127/index.html?lang=cn
https://simulator.h3c.com/home/bios

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
