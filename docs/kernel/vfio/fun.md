# 让系统在虚拟机中启动

四年前，我在裸金属二进制翻译器的时候，调研过程中发现了空客写了一个叫 [ramooflax](https://github.com/airbus-seclab/ramooflax)
的项目，其基本想法是，对于一个正常使用的普通的操作系统，如果我插上一个 U 盘，这个 U 盘里面有一个 Hypervisor ，
那么系统引导先启动这个 Hypervisor ，然后这个 Hypervisor 来启动原来的操作系统，这样原来的操作系统相当于被架空了一层，他不再是直接运行在物理硬件，
而不是运行在虚拟化环境中的。

> The objective is to virtualize already installed operating systems on physical dedicated machine.

这个想法很有趣，我当时意识到这个可以通过 QEMU 实现，但是我对于 QEMU / edk2 / seabios 的理解还很薄弱，就一直搁置起来了。
不过今天(2025-12-27)，各种条件都成熟了，我尝试了一下走通这个操作，实际结果我想想还要顺利。

## 通过 qemu 安装

首先将 nvme 盘直通到虚拟机中，然后安装系统
关键的启动参数为:
```txt
	-drive file=/home/martins3/data/hack/iso/Fedora-Server-dvd-x86_64-43-1.6.iso,format=raw,if=none,id=cd0,readonly=on \
	-device scsi-cd,bus=scsi1.0,channel=0,scsi-id=20,lun=1,drive=cd0,bootindex=0 \

	-device virtio-blk-pci,drive=boot1,bootindex=1

	-drive file=/home/martins3/data/bios/ovmf_binary/usr/share/edk2/ovmf/OVMF_CODE.fd,if=pflash,format=raw,unit=0,readonly=on \
	-drive file=/home/martins3/data/hack/vm/yyds-nvme-boot/OVMF_VARS.fd,if=pflash,format=raw,unit=1 \
```

这里需要注意两个事情:
1. 因为现在物理机中使用默认方式安装了一个 fedora ，如果安装虚拟机也直接用默认方法，会出现 lvm 的 logical volume 名称冲突的问题，
导致两个系统都无法进入系统。具体报错参考[这个](https://www.reddit.com/r/Proxmox/comments/lyxuuj/multiple_vgs_found_with_the_same_name_skipping_pve/)
```txt
nvme2n1         259:5    0   3.6T  0 disk
├─nvme2n1p1     259:6    0   600M  0 part /boot/efi
├─nvme2n1p2     259:7    0     1G  0 part /boot
└─nvme2n1p3     259:8    0   3.6T  0 part
  └─fedora-root 252:0    0   3.6T  0 lvm  /
```
2. 启动虚拟机需要使用 ovmf ，而不是 seabios ，因为物理机的启动方式也是 UEFI 的。



虚拟机中安装好之后:
```txt
🧀  fastfetch
             .',;::::;,'.                 martins3@localhost
         .';:cccccccccccc:;,.             ------------------
      .;cccccccccccccccccccccc;.          OS: Fedora Linux 43 (Server Edition) x86_64
    .:cccccccccccccccccccccccccc:.        Host: KVM/QEMU Standard PC (i440FX + PIIX, 1996) (pc-i440fx-9.2)
  .;ccccccccccccc;.:dddl:.;ccccccc;.      Kernel: Linux 6.17.1-300.fc43.x86_64
 .:ccccccccccccc;OWMKOOXMWd;ccccccc:.     Uptime: 28 mins
.:ccccccccccccc;KMMc;cc;xMMc;ccccccc:.    Packages: 1317 (nix-user)
,cccccccccccccc;MMM.;cc;;WW:;cccccccc,    Shell: zsh 5.9
:cccccccccccccc;MMM.;cccccccccccccccc:    Display (QEMU Monitor): 1280x800 @ 75 Hz in 15"
:ccccccc;oxOOOo;MMM000k.;cccccccccccc:    Terminal: /dev/pts/1
cccccc;0MMKxdd:;MMMkddc.;cccccccccccc;    CPU: 13th Gen Intel(R) Core(TM) i9-13900K (32) @ 3.00 GHz
ccccc;XMO';cccc;MMM.;cccccccccccccccc'    GPU: Unknown Device 1111 (VGA compatible)
ccccc;MMo;ccccc;MMW.;ccccccccccccccc;     Memory: 2.08 GiB / 31.32 GiB (7%)
ccccc;0MNc.ccc.xMMd;ccccccccccccccc;      Swap: 0 B / 8.00 GiB (0%)
cccccc;dNMWXXXWM0:;cccccccccccccc:,       Disk (/): 32.12 GiB / 950.82 GiB (3%) - xfs
cccccccc;.:odl:.;cccccccccccccc:,.        Local IP (ens7): 10.0.2.15/24
ccccccccccccccccccccccccccccc:'.          Locale: en_US.UTF-8
:ccccccccccccccccccccccc:;,..
 ':cccccccccccccccc::;,.
```

```txt
NAME        MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
sda           8:0    0    10G  0 disk
sdb           8:16   0    10G  0 disk
sr0          11:0    1   3.2G  0 rom
zram0       251:0    0     8G  0 disk [SWAP]
vda         253:0    0   350G  0 disk
nvme0n1     259:0    0 953.9G  0 disk
├─nvme0n1p1 259:1    0   600M  0 part /boot/efi
├─nvme0n1p2 259:2    0     2G  0 part /boot
└─nvme0n1p3 259:3    0 951.3G  0 part /

root@localhost:~# blkid
/dev/nvme0n1p3: UUID="1eea5666-0bc7-48c1-a3ba-8e4156107f41" BLOCK_SIZE="512" TYPE="xfs" PARTUUID="be4b4958-2c4d-474c-a9c3-6b807ef1cd00"
/dev/nvme0n1p1: UUID="AE26-2141" BLOCK_SIZE="512" TYPE="vfat" PARTLABEL="EFI System Partition" PARTUUID="9512a2cb-5f8c-4c8a-a2ac-8b9cadb0aef6"
/dev/nvme0n1p2: UUID="06daf7d3-0d10-4a6d-a1a7-d28204877b97" BLOCK_SIZE="512" TYPE="xfs" PARTUUID="822fe3cf-3f8c-46e8-8ea9-11648f4816bb"
/dev/sr0: BLOCK_SIZE="2048" UUID="2025-10-23-03-25-56-00" LABEL="Fedora-S-dvd-x86_64-43" TYPE="iso9660" PTTYPE="PMBR"
/dev/zram0: LABEL="zram0" UUID="07038a94-efb1-487f-accb-2de7b185d679" TYPE="swap"
```

通过 QEMU 安装好虚拟机后，物理机中，盘的形态为:
```txt

🧀  lsblk
NAME            MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
sda               8:0    0   1.8T  0 disk
├─sda1            8:1    0   600M  0 part
├─sda2            8:2    0     1G  0 part
└─sda3            8:3    0   1.8T  0 part
sdb               8:16   0 476.9G  0 disk
├─sdb1            8:17   0   600M  0 part
├─sdb2            8:18   0     1G  0 part
└─sdb3            8:19   0    15G  0 part
zram0           251:0    0     8G  0 disk [SWAP]
nvme0n1         259:0    0 953.9G  0 disk
nvme1n1         259:1    0 953.9G  0 disk
├─nvme1n1p1     259:2    0   600M  0 part (新系统的 efi 分区)
├─nvme1n1p2     259:3    0     2G  0 part
└─nvme1n1p3     259:4    0 951.3G  0 part
nvme2n1         259:5    0   3.6T  0 disk
├─nvme2n1p1     259:6    0   600M  0 part /boot/efi
├─nvme2n1p2     259:7    0     1G  0 part /boot
└─nvme2n1p3     259:8    0   3.6T  0 part
  └─fedora-root 252:0    0   3.6T  0 lvm  /
```

## 配置 grub
在 /etc/grub.d/40_custom 里加入，这里的 AE26-2141 是执行 blkid 获取的，需要具体的调整:

```txt
menuentry "Fedora on nvme1n1 (EFI)" {
    search --no-floppy --fs-uuid --set=root AE26-2141
    chainloader /EFI/fedora/grubx64.efi
}
```
然后:
```txt
sudo grub2-mkconfig -o /etc/grub2-efi.cfg
```

## 验证结果
进入到 grub 界面后，选择最下面的一行，选择内核进入:

从物理机中进入，再次观察，可以发现还是之前的盘，但是硬件都切换为物理的硬件了:
```txt
🧀  fastfetch
             .',;::::;,'.                 martins3@localhost
         .';:cccccccccccc:;,.             ------------------
      .;cccccccccccccccccccccc;.          OS: Fedora Linux 43 (Server Edition) x86_64
    .:cccccccccccccccccccccccccc:.        Kernel: Linux 6.17.1-300.fc43.x86_64
  .;ccccccccccccc;.:dddl:.;ccccccc;.      Uptime: 4 mins
 .:ccccccccccccc;OWMKOOXMWd;ccccccc:.     Packages: 1317 (nix-user)
.:ccccccccccccc;KMMc;cc;xMMc;ccccccc:.    Shell: zsh 5.9
,cccccccccccccc;MMM.;cc;;WW:;cccccccc,    Display (DELL S2721QS): 3840x2160 @ 60 Hz in 27" [External]
:cccccccccccccc;MMM.;cccccccccccccccc:    Terminal: /dev/pts/0
:ccccccc;oxOOOo;MMM000k.;cccccccccccc:    CPU: 13th Gen Intel(R) Core(TM) i9-13900K (32) @ 5.80 GHz
cccccc;0MMKxdd:;MMMkddc.;cccccccccccc;    GPU 1: NVIDIA GeForce GTX 1060 3GB [Discrete]
ccccc;XMO';cccc;MMM.;cccccccccccccccc'    GPU 2: Intel UHD Graphics 770 @ 1.65 GHz [Integrated]
ccccc;MMo;ccccc;MMW.;ccccccccccccccc;     Memory: 1.75 GiB / 125.52 GiB (1%)
ccccc;0MNc.ccc.xMMd;ccccccccccccccc;      Swap: 0 B / 8.00 GiB (0%)
cccccc;dNMWXXXWM0:;cccccccccccccc:,       Disk (/): 32.16 GiB / 950.82 GiB (3%) - xfs
cccccccc;.:odl:.;cccccccccccccc:,.        Locale: en_US.UTF-8
ccccccccccccccccccccccccccccc:'.
:ccccccccccccccccccccccc:;,..
 ':cccccccccccccccc::;,.
```

```txt
~ 🦁
🧀  lsblk
NAME            MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
sda               8:0    0   1.8T  0 disk
├─sda1            8:1    0   600M  0 part
├─sda2            8:2    0     1G  0 part
└─sda3            8:3    0   1.8T  0 part
sdb               8:16   0 476.9G  0 disk
├─sdb1            8:17   0   600M  0 part
├─sdb2            8:18   0     1G  0 part
└─sdb3            8:19   0    15G  0 part
zram0           251:0    0     8G  0 disk [SWAP]
nvme0n1         259:0    0 953.9G  0 disk
nvme1n1         259:1    0 953.9G  0 disk
├─nvme1n1p1     259:2    0   600M  0 part /boot/efi
├─nvme1n1p2     259:3    0     2G  0 part /boot
└─nvme1n1p3     259:4    0 951.3G  0 part /
nvme2n1         259:5    0   3.6T  0 disk
├─nvme2n1p1     259:6    0   600M  0 part
├─nvme2n1p2     259:7    0     1G  0 part
└─nvme2n1p3     259:8    0   3.6T  0 part
  └─fedora-root 252:0    0   3.6T  0 lvm
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
