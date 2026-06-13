# QEMU tcg 模式使用直通设备

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

结束，睡觉！

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
