# 为什么 QEMU 这么复杂

首先我推荐你读一读 [A QEMU case study in grappling with software complexity](https://lwn.net/Articles/872321/)，
这是 QEMU 的 maintainer Bonzini 在 KVM forum 2021 上做的一篇报告。

其将复杂度分为两种:
1. essential complexity
2. accidental complexity
	1. “不完整的过渡”（这一概念受一篇关于GCC维护的论文启发），指的是当引入一种更新、更好的实现方式时，却未能将其一致地应用于整个代码库。
	2. 重复逻辑与缺失的抽象

这里我们主要聊聊 essential complexity ，也就是由于 QEMU 所支持的功能极为庞大，
导致 QEMU 自身非常的复杂。


## QEMU 可以模拟一个完整的机器

那么，一个完整机器有什么呢?

如果你手头有一个 Linux 机器，可以执行 lshw 看看

这是我让 codex 总结了下此时我正在使用的机器的情况:
```txt
  - 主板：ASUS TUF GAMING B660-PLUS WIFI D4
  - BIOS：American Megatrends 1620，日期 08/12/2022
  - CPU：Intel 13th Gen Core i9-13900K，24 cores / 32 threads
  - 内存：128 GiB DDR4，4 条 32 GiB Gloway，2400 MHz
  - GPU：
      - NVIDIA GB206 [GeForce RTX 5060 Ti]，driver=nvidia
      - Intel UHD Graphics 770，driver=i915

  - 存储：
      - NVMe Fanxiang S790 4TB：/dev/nvme1n1，driver=nvme
      - NVMe ZHITAI TiPlus7100 1TB：/dev/nvme2n1，当前是 swap，driver=nvme
      - NVMe ZHITAI TiPro7000：PCI 02:00.0，driver=vfio-pci
      - SATA HDD WDC WD20EZBX-00A：/dev/sda，约 2TB，ext4
      - SATA SSD ZHITAI SC001 Act：/dev/sdb，512GB

  - 网络：
      - WiFi：Intel CNVi WiFi，wlan0，driver=iwlwifi，IP 192.168.2.41
      - 有线：Intel I225-V，enp6s0，driver=igc，当前 1G link，IP 10.0.3.0
      - 有线：Realtek RTL8125 2.5GbE，enp5s0，driver=r8169，当前 disabled/link=no

  - USB：
      - Intel xHCI USB 3.2 controller
      - ASUS AURA LED Controller
      - Compx VGN Mouse 2.4G Receiver
      - ROYUAN Gaming keyboard
      - Intel AX201 Bluetooth
      - ASUS ASM107x USB hubs

  - 音频：
      - Intel Alder Lake-S HD Audio，driver=snd_hda_intel
      - NVIDIA HDMI/DP Audio，driver=snd_hda_intel

  - 其他控制器：
      - Intel HECI/MEI：driver=mei_me
      - Intel SMBus：driver=i801_smbus
      - Intel SPI：driver=intel-spi
      - Intel LPSS I2C controllers：driver=intel-lpss

  比较值得注意的是：ZHITAI TiPro7000 这块 NVMe 当前绑定到了 vfio-pci，不是普通 nvme 驱动；Realtek 2.5G 网卡当前是 disabled。
```
实际上，QEMU 支持以上的所有类型的设备。

这里就不再一一罗列，具体可以看
docs/qemu/qom/options.md 中的后半段。

最后你可以发现，随便搞搞，QEMU 的启动参数极多，单单看 man qemu 完全不够:
由于太过复杂，封装 QEMU 的软件都很多，例如 utm 和 quickemu

ffmpeg 的参数也非常的复杂，以至于有这个网站 https://evanhahn.github.io/ffmpeg-buddy/
这个项目恰好是 QEMU 作者的另一个的工具

## 模拟方法众多

CPU 模拟:
- 无硬件加速 : tcg
- 硬件加速，对于不同的操作系统，分别支持 kvm Hyper-V whpx

这里说一下，tcg 所代表的二进制翻译就是一个深不见底的领域。
其中如下:
1. 加速 capstone 的反汇编
2. multithread tcg
3. 增加 tb 的命中率
4. x86 几千条指令如何正确的翻译。

在 Loongarch 上运行微信，看一个订阅文章，然后打一个视频电话
其中的复杂性极为夸张:
1. 订阅文章用的是默认浏览器，其中包含了一个 v8 引擎，而 v8 会动态的生成 x86 代码执行，这需要 tcg 动态的将其翻译为 Loongarch 指令
2. 视频通话: 编解码大量使用 simd 指令，如何高效的翻译为 Loongarch 指令

即便是 virtio ，也可以划分为 [pci 和 mmio 后端](https://stackoverflow.com/questions/65550766/confusion-about-virtio-net-pci-and-virtio-net-in-qemu)

对于设备模拟也是类似各种，例如 qemu-system-$(uname -m) -chardev help
```txt
  ringbuf
  serial
  stdio
  spicevmc
  mux
  pipe
  qemu-vdagent
  hub
  null
  pty
  msmouse
  socket
  spiceport
  vc
  parallel
  dbus
  memory
  udp
  file
  wctablet
  testdev
```


真的引入复杂度的是云计算中广泛使用的:
- nvme
- virtio
- vhost
- vdap
- vfio


1. QEMU 支持多种存储的格式:
qemu-img -h 中显示的
```txt
Supported formats: blkdebug blklogwrites blkverify bochs cloop compress copy-before-write copy-on-read dmg file ftp ftps host_cdrom host_device http https iscsi iser luks nbd null-aio null-co nvme parallels preallocate qcow qcow2 qed quorum raw replication snapshot-access throttle vdi vhdx vmdk vpc vvfat
```

## 虚拟机状态控制

对于虚拟机本身状态的控制，也就是 CPU 状态 + 内存:
- snapshot
- migrate
- savevm
- 安全 ( 盘加密，可信计算，安全启动，vTPM)
- QEMU 热更新，利用 cpr 机制
- 热迁移
- vfio-user
- 热插拔

对于虚拟机使用的磁盘，需要支持如下功能:
- snapshot
- ratelimit

## 各种机制的可组合

QEMU 的强大之处在于，这些前端和后端很多时候可以互相不耦合。
例如虚拟机中看到的是 virtio-blk ，但是后端可能是利用 vfio 将 nvme 直通到用户态，然后添加上用户态驱动模拟出来的，具体参考
block/nvme.c

### QEMU tcg 模式使用直通设备

我第一次接触直通的时候，我当时正在做二进制翻译相关的东西，
我当时对于 VFIO 的原理不了解，只是感觉 tcg 很慢，VFIO 很快，两个东西放在一次有点不协调，
不过也想不到为什么他们会不兼容，今天花费了 8 分钟测试了一下，他们两个互相不冲突的。

2025-12-30 最新的 qemu 和 kernel

启动参数:
```txt
qemu-system-x86_64 \
	-device virtio-scsi,id=scsi1 \
	-device scsi-hd,drive=virtio-scsi_1,bus=scsi1.0,channel=0,scsi-id=1,lun=0 \
	-drive file=/home/martins3/data/hack/vm/yyds/img/virtio-scsi_1,if=none,id=virtio-scsi_1 \
	-device megasas-gen2,id=scsi2 \
	-device scsi-hd,drive=megasas-gen2_1,bus=scsi2.0,channel=0,scsi-id=1,lun=0 \
	-drive file=/home/martins3/data/hack/vm/yyds/img/megasas-gen2_1,if=none,id=megasas-gen2_1 \
	-smp 32,maxcpus=32 \
	-m 3G,slots=8,maxmem=256G \
	-object memory-backend-memfd,id=mem0,size=3G,prealloc=off,share=on \
	-numa node,nodeid=0,memdev=mem0 \
	-drive file=/home/martins3/data/hack/vm/yyds/img/boot1,format=qcow2,id=boot1,if=none,discard=on,aio=native,cache.direct=on,media=disk \
	-device virtio-blk-pci,drive=boot1,bootindex=1 \
	-kernel /home/martins3/data/kernel/linux-build/arch/x86/boot/bzImage \
	-append '  oops=panic panic=0 nokaslr apparmor=0 selinux=0 preempt=full systemd.unified_cgroup_hierarchy=1  mitigations=off  rcutree.sysrq_rcu=1  crashkernel=512M  loglevel=8 zswap.enabled=0  BOOT_IMAGE=(hd0,gpt2)/vmlinuz-6.14.0-63.fc42.x86_64 root=/dev/mapper/fedora-root ro rd.lvm.lv=fedora/root console=tty0 nokaslr console=ttyS0,9600 earlyprink=serial mitigations=off audit=0 ' \
	-device virtio-net,netdev=vif_s_29_0,mac=52:54:00:00:1d:00,iommu_platform=on,disable-legacy=on \
	-netdev tap,ifname=vif_s_29_0,id=vif_s_29_0,script=no,downscript=no,vhost=on \
	-device virtio-net,netdev=net1 \
	-netdev user,id=net1,hostfwd=tcp::50584-:22 \
	-machine q35,hpet=off,smm=off,cxl=on \
	-chardev socket,id=mon4,path=/home/martins3/data/hack/vm/yyds/s/qmp-no-pretty,server=on,wait=off \
	-mon chardev=mon4,mode=control \
	-chardev socket,id=mon3,path=/home/martins3/data/hack/vm/yyds/s/qmp-shell,server=on,wait=off \
	-mon chardev=mon3,mode=control \
	-chardev socket,id=mon2,path=/home/martins3/data/hack/vm/yyds/s/hmp,server=on,wait=off \
	-mon chardev=mon2 \
	-chardev socket,id=mon1,path=/home/martins3/data/hack/vm/yyds/s/qmp,server=on,wait=off \
	-mon chardev=mon1,mode=control,pretty=on \
	-initrd /home/martins3/data/kernel/initramfs-6.14.0-63.fc42.x86_64.img.linux-build.raw.zst \
	-device virtio-balloon,id=balloon0,deflate-on-oom=true \
	-device ipmi-bmc-sim,id=virt-bmc \
	-device pci-ipmi-kcs,bmc=virt-bmc,id=virt-bmc-pci \
	--accel tcg \
	-device edu \
	-pidfile /home/martins3/data/hack/vm/yyds/s/pid \
	-vga qxl \
	-vnc :44680,password=off \
	-device virtio-serial \
	-chardev stdio,id=main_char,server=on,wait=off,id=main_char,mux=on \
	-serial chardev:main_char \
	-device virtconsole,chardev=main_char \
	-mon chardev=main_char,mode=readline \
	-chardev pty,mux=on,id=char_pty \
	-device virtconsole,chardev=char_pty \
	-serial chardev:char_pty \
	-chardev socket,path=/home/martins3/data/hack/vm/yyds/s/qga.sock,server=on,wait=off,id=qga0 \
	-device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0 \
	-chardev socket,path=/home/martins3/data/hack/vm/yyds/s/vport.sock,server=on,wait=off,id=vport \
	-device virtserialport,chardev=vport,name=org.qemu.vport.0 \
	-device pcie-root-port,id=root_port_1 \
	-no-user-config \
	-nodefaults \
	-name guest=martins3,debug-threads=on \
	-uuid 6f5ea67d-7934-4351-83f4-b1ce6cb35867 \
	-device virtio-keyboard \
	-usb \
	-device qemu-xhci,p2=8,p3=8,id=usb \
	-device usb-kbd,id=input0,bus=usb.0,port=2 \
	-device usb-tablet,id=input1,bus=usb.0,port=3
```

```txt
🧀  fastfetch
             .',;::::;,'.                 martins3@localhost
         .';:cccccccccccc:;,.             ------------------
      .;cccccccccccccccccccccc;.          OS: Fedora Linux 42 (Server Edition) x84
    .:cccccccccccccccccccccccccc:.        Host: KVM/QEMU Standard PC (Q35 + ICH9,)
  .;ccccccccccccc;.:dddl:.;ccccccc;.      Kernel: Linux 6.18.1-martins3-00001-g34a
 .:ccccccccccccc;OWMKOOXMWd;ccccccc:.     Uptime: 2 mins
q:ccccccccccccc;KMMc;cc;xMMc;ccccccc:.     Packages: 1319 (nix-user)
,cccccccccccccc;MMM.;cc;;WW:;cccccccc,    Shell: zsh 5.9
:cccccccccccccc;MMM.;cccccccccccccccc:    Terminal: /dev/pts/0
:ccccccc;oxOOOo;MMM000k.;cccccccccccc:    CPU: QEMU Virtual version 2.5+ (32) @ 3.00 GHz
cccccc;0MMKxdd:;MMMkddc.;cccccccccccc;    GPU: Red Hat, Inc. QXL paravirtual graphic card
ccccc;XMO';cccc;MMM.;cccccccccccccccc'    Memory: 429.95 MiB / 2.33 GiB (18%)
ccccc;MMo;ccccc;MMW.;ccccccccccccccc;     Swap: 0 B / 2.33 GiB (0%)
ccccc;0MNc.ccc.xMMd;ccccccccccccccc;      Disk (/): 44.49 GiB / 348.93 GiB (13%) - xfs
cccccc;dNMWXXXWM0:;cccccccccccccc:,       Local IP (enp0s6): 10.0.2.15/24
cccccccc;.:odl:.;cccccccccccccc:,.        Locale: en_US.UTF-8
ccccccccccccccccccccccccccccc:'.
:ccccccccccccccccccccccc:;,..
 ':cccccccccccccccc::;,.
```


```txt
NAME            MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
sda               8:0    0     8M  0 disk
sdb               8:16   0    10G  0 disk
vda             251:0    0   350G  0 disk
├─vda1          251:1    0     1M  0 part
├─vda2          251:2    0     1G  0 part /boot
└─vda3          251:3    0   349G  0 part
  └─fedora-root 253:0    0   349G  0 lvm  /
zram0           252:0    0   2.3G  0 disk [SWAP]
nullb0          254:0    0   250G  0 disk
nvme0n1         259:0    0 953.9G  0 disk
```

发现存在这个问题，但是没关系，不用直通也有
```txt
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x555d4815ef40, 0xc000054000, 0x2000, 0x7f4580644000) = -22 (Invalid argument)
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x555d4815ef40, 0xc000057000, 0x1000, 0x7f4580647000) = -22 (Invalid argument)
qemu-system-x86_64: VFIO_MAP_DMA failed: Invalid argument
qemu-system-x86_64: vfio_container_dma_map(0x555d4815ef40, 0xc000040000, 0x10000 , 0x7f4580520000) = -22 (Invalid argument)
qemu-system-x86_64: vfio-pci: Cannot read device rom at 0000:02:00.0
```

## 如何解决
1. async io 模型 : 解决高效模拟的基础，使用 epoll 机制来监听 Guest 事件，然后提交给后端，当后端完成后，注入通知到 Guest 中
2. qom / qdev : 如何让成千上万的设备
3. 提供 hmp / qmp : 让管理面在外部，例如 libvirt
4. qtest / trace 等测试，观测机制
5. memory region : 设备模拟的基础

还好，总体来说，QEMU 中大多数内容都是对称的，核心机制掌握后，总体来说，难度可控。

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
