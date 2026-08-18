## open-gpu-kernel-modules
<!-- a36ad757-528a-49ca-8e49-d59f3c78b526 -->

### 针对虚拟机构建测试
make -j$(nproc) \
    SYSSRC=/home/martins3/data/kernel/linux-build \
    SYSOUT=/home/martins3/data/kernel/linux-build \
    modules

原来 open-gpu-kernel-modules 中的包就是这些了:
kmod-nvidia-6.19.10-100.fc42.x86_64-580.142-1.fc42.x86_64

然后生成 compile_commands.json

cd /home/martins3/data/open-gpu-kernel-modules/kernel-open
  /home/martins3/data/kernel/linux-build/scripts/clang-tools/gen_compile_commands.py \
    -d /home/martins3/data/open-gpu-kernel-modules/kernel-open \
    -o /home/martins3/data/open-gpu-kernel-modules/compile_commands.json \
    modules.order

### 在物理机中构建和测试
make modules -j$(nproc)
sudo make modules_install -j$(nproc)

modinfo 结果:
```txt
filename:       /lib/modules/7.0.8-200.fc44.x86_64/kernel/drivers/video/nvidia.ko
import_ns:      DMA_BUF
alias:          char-major-195-*
description:    NVIDIA core GPU kernel module
version:        595.71.05
supported:      external
license:        Dual MIT/GPL
firmware:       nvidia/595.71.05/gsp_tu10x.bin
firmware:       nvidia/595.71.05/gsp_ga10x.bin
softdep:        pre: ecdh_generic,ecdsa_generic
srcversion:     58D233B8E3F4A2973D73151
alias:          pci:v000010DEd*sv*sd*bc06sc80i00*
alias:          pci:v000010DEd*sv*sd*bc03sc02i00*
alias:          pci:v000010DEd*sv*sd*bc03sc00i00*
alias:          of:N*T*Cnvidia,tegra264-displayC*
alias:          of:N*T*Cnvidia,tegra264-display
alias:          of:N*T*Cnvidia,tegra234-displayC*
alias:          of:N*T*Cnvidia,tegra234-display
depends:
name:           nvidia
retpoline:      Y
vermagic:       7.0.8-200.fc44.x86_64 SMP preempt mod_unload
```

### 仓库代码的关系
• 有，这 4 个模块这个仓库里都有对应源码。

  对应关系是：

  - nvidia -> nvidia.ko
    代码在 kernel-open/nvidia/nvidia.Kbuild:12 和 src/nvidia/Makefile:1
  - nvidia_modeset -> nvidia-modeset.ko
    代码在 kernel-open/nvidia-modeset/nvidia-modeset.Kbuild:14 和 src/nvidia-modeset/
  - nvidia_uvm -> nvidia-uvm.ko
    代码在 kernel-open/nvidia-uvm/nvidia-uvm.Kbuild:16
  - nvidia_drm -> nvidia-drm.ko
    代码在 kernel-open/nvidia-drm/nvidia-drm.Kbuild:17

  有两个细节你要注意：

  - lsmod 里显示的是下划线名字，比如 nvidia_drm，源码/模块文件里常写成带连字符的 nvidia-drm.ko，这是正常的。
  - 这 4 个模块里，nvidia 和 nvidia_modeset 分成两部分：
    Linux 相关接口层在 kernel-open/
    OS-agnostic 部分在 src/
    README 也明确写了这一点，见 README.md:130

  另外一个关键限制是：
  “模块源码有”不等于“整个驱动栈都在这里”。这个仓库没有用户态驱动组件，也还依赖配套的 GSP firmware，见 README.md:18。


这四个模块的功能是:
- nvidia
  这是主模块，核心 GPU 驱动本体。负责设备探测、PCIe、显存管理、中断、提交控制、字符设备 /dev/nvidia*、加载固件等，很多
  其他模块都依赖它。源码入口可看 kernel-open/nvidia/nv.c:1。
- nvidia_modeset
  这是显示模式设置模块，主要管“显示输出”这条线：显示管线、分辨率/刷新率切换、显示器连接、VRR、HDMI/DP 等。源码里直接能
  看到很多显示相关参数和 nvkms 接口，见 kernel-open/nvidia-modeset/nvidia-modeset-linux.c:1。
- nvidia_drm
  这是把 NVIDIA 私有显示栈接到 Linux DRM/KMS 框架上的适配层。它让内核图形子系统、Wayland、DRM atomic modesetting、fbdev
  这些东西能和 NVIDIA 驱动协同工作。源码里模块参数就直接写着 Enable atomic kernel modesetting，见 kernel-open/nvidia-
  drm/nvidia-drm-linux.c:27。
- nvidia_uvm
  这是 Unified Virtual Memory，主要给 CUDA/计算用。它负责 CPU 和 GPU 之间的统一虚拟地址空间、按页迁移、缺页处理、HMM/内
  存映射之类。源码入口在 kernel-open/nvidia-uvm/uvm.c:1。

## 几个基本的 sysfs 的结果

1060 的环境:

🧀  ls
information  power  registry
nvidia/gpus/0000:01:00.0🔒 😸
🧀  cat information
Model:           NVIDIA GeForce GTX 1060 3GB
IRQ:             221
GPU UUID:        GPU-9ba0f26a-a046-9909-7b2a-17a000456830
Video BIOS:      86.06.11.00.48
Bus Type:        PCIe
DMA Size:        47 bits
DMA Mask:        0x7fffffffffff
Bus Location:    0000:01:00.0
Device Minor:    0
GPU Excluded:    No
nvidia/gpus/0000:01:00.0🔒 😸

🧀  cat power
Runtime D3 status:          Disabled by default
Video Memory:               Active

GPU Hardware Support:
 Video Memory Self Refresh: Not Supported
 Video Memory Off:          Not Supported

S0ix Power Management:
 Platform Support:          Not Supported
 Status:                    Disabled

Notebook Dynamic Boost:     Not Supported
nvidia/gpus/0000:01:00.0🔒 😸

🧀  cat registry
Binary: ""


🧀   cat /proc/driver/nvidia/version
NVRM version: NVIDIA UNIX x86_64 Kernel Module  580.142  Tue Mar  3 20:04:04 UTC 2026
GCC version:  gcc version 15.2.1 20260123 (Red Hat 15.2.1-7) (GCC)

## src/nvidia/ 和 kernel-open/nvidia/ 是什么关系

- src/nvidia/Makefile:1 是 OS-agnostic core，也就是和具体 Linux 内核版本无关的核心 RM 逻辑。
	它先被编成一个大对象 nv-kernel.o，见 Makefile:29。
- kernel-open/nvidia/nvidia.Kbuild:1 是 Linux kernel interface layer，也就是跟 Linux 内核 API 打交道的那层：PCI、
  procfs、mmap、acpi、dma、字符设备、内核回调这些。

仓库 README 也是这么定义的，见 README.md:132 和 README.md:155。

实际构建关系是：

1. 先在 src/nvidia/ 里把大量通用代码编成 src/nvidia/_out/.../nv-kernel.o
2. 顶层 Makefile:36 把它链接成 kernel-open/nvidia/nv-kernel.o_binary 的符号链接
3. 然后 kernel-open/nvidia/nvidia.Kbuild 再把这个 nv-kernel.o_binary 和 kernel-open/nvidia/*.o 一起链接成最终的
   nvidia.ko，见 kernel-open/nvidia/nvidia.Kbuild:19 和 kernel-open/nvidia/nvidia.Kbuild:48

所以不是“两个独立模块”，也不是简单的“一个包含另一个”，而是：

nvidia.ko = src/nvidia 里的核心逻辑 + kernel-open/nvidia 里的 Linux 适配层

你可以把它理解成：

- src/nvidia/：驱动的大脑
- kernel-open/nvidia/：大脑接 Linux 内核的手脚和接口层

如果你愿意，我可以下一步把 kernel-open/nvidia/nv.c 和 src/nvidia/srcs.mk 对起来，给你画出一个更具体的调用分层图。

### 不过 peermem 不一样

• 如果你说的是 nvidia-peermem，那它和 nvidia 不一样，没有 src/nvidia-peermem 这一半。

  关系是这样的：

  - kernel-open/nvidia-peermem/
    这里就是 完整的 nvidia-peermem.ko 模块源码，只有这一层，没有对应的 src/ 核心半边。
    证据见 kernel-open/nvidia-peermem/nvidia-peermem.Kbuild:9，它只编一个 nvidia-peermem/nvidia-peermem.c。
  - kernel-open/nvidia/
    这里提供的是 nvidia-peermem 依赖的 底层 P2P 接口实现，例如 nvidia_p2p_get_pages()、nvidia_p2p_dma_map_pages()、
    nvidia_p2p_free_page_table()，见 kernel-open/nvidia/nv-p2p.c:650 和 kernel-open/nvidia/nv-p2p.h:193。
  - kernel-open/nvidia-peermem/nvidia-peermem.c
    这个模块本身会调用上面这些 nvidia_p2p_* API，并向 IB/RDMA 栈注册 peer memory client，见 kernel-open/nvidia-peermem/
    nvidia-peermem.c:570。

  所以可以这么理解：

  - nvidia-peermem.ko 是一个 RDMA/InfiniBand 插件模块
  - 它自己全部代码都在 kernel-open/nvidia-peermem/
  - 它下面真正干活依赖的是 nvidia.ko 里导出的 P2P 能力
  - 没有 像 nvidia.ko 那样的 src/... + kernel-open/... 两段式拆分

  再补一句容易混淆的点：

  - src/nvidia/ 里也会出现很多 peer memory / peermem 字样，那是 NVIDIA 核心驱动内部的 peer memory 逻辑
  - 但它们 不是单独的 nvidia-peermem 模块源码目录

## 两种配置

vGPU 下:
```txt
$ lsmod | grep nvidia
nvidia_vgpu_vfio       77824  63
nvidia              56795136  6620
mdev                   28672  2 vfio_mdev,nvidia_vgpu_vfio
drm                   638976  4 drm_kms_helper,nvidia,mgag200
vfio                   45056  12 vfio_mdev,nvidia_vgpu_vfio,vfio_iommu_type1
```
图形界面:
```txt
$ lsmod | grep nvidia
nvidia_uvm           2576384  0
nvidia_drm            155648  16
nvidia_modeset       2187264  8 nvidia_drm
nvidia              16351232  200 nvidia_uvm,nvidia_modeset
drm_ttm_helper         20480  2 nvidia_drm,xe
video                  81920  4 asus_wmi,xe,i915,nvidia_modeset
```

## 关于 open 非 open 版本
如果在 L20 中没有安装 16.4 的驱动，不是 open 版本的，卸载驱动直接导致物理机宕机。
```txt
[48648.235726] ------------[ cut here ]------------
[48648.242425] WARNING: CPU: 59 PID: 2922063 at drivers/pci/msi.c:1244 pci_enable_msix_range+0x1d/0x40
[48648.253856] Modules linked in: pci_pf_stub ebtable_filter ebtables ip6table_filter ip6_tables nfsv3 nfs_acl nfs lockd grace nfs_ssc fscache scsi_transport_iscsi nf_conntrack_netlink xt_comment xt_set iptable_filter ip_set_hash_net ip_set_bitmap_port ip_set overlay sch_ingress rdma_ucm(OE) rdma_cm(OE) iw_cm(OE) ib_ipoib(OE) ib_cm(OE) ib_umad(OE) nfnetlink_cttimeout nfnetlink openvswitch nf_conncount vfio_pci vfio_virqfd sunrpc vfat fat nvidia_vgpu_vfio(OE) ast drm_vram_helper drm_ttm_helper loop nvidia(POE) amd64_edac_mod ttm edac_mce_amd bonding drm_kms_helper syscopyarea vfio_mdev crct10dif_pclmul crc32_pclmul sysfillrect mdev kvm_amd sysimgblt fb_sys_fops vfio_iommu_type1 ses ghash_clmulni_intel cec enclosure scsi_transport_sas joydev rapl pcspkr vfio drm k10temp i2c_piix4 sg ipmi_ssif ccp kvm irqbypass acpi_ipmi ipmi_si nf_nat_ftp ipmi_devintf ipmi_msghandler ip_tables ext4 mbcache jbd2 mlx5_ib(OE) ib_uverbs(OE) ib_core(OE) sr_mod cdrom raid1 mlx5_core(OE) mlxfw(OE) igb
[48648.253935]  tls uas sd_mod nvme ahci psample nvme_core libahci i2c_algo_bit mlxdevm(OE) t10_pi mlx_compat(OE) dca usb_storage libata pci_hyperv_intf megaraid_sas hisdk3(OE) hinic3(OE) hiudk3(OE) fuse nf_conntrack_ftp nf_nat_tftp nf_conntrack_tftp nf_nat nf_conntrack nf_defrag_ipv6 libcrc32c crc32c_intel nf_defrag_ipv4
[48648.403317] Hardware name: Suma R6240H0/62DB32, BIOS SXYH041031 11/27/2023
[48648.412411] RIP: 0010:pci_enable_msix_range+0x1d/0x40
[48648.419594] Code: b8 de ff ff ff e9 33 b6 7e 00 0f 1f 00 0f 1f 44 00 00 39 ca 7f 20 f6 87 0a 0b 00 00 20 75 0b 45 31 c9 45 31 c0 e9 03 fe ff ff <0f> 0b b8 ea ff ff ff e9 07 b6 7e 00 b8 de ff ff ff e9 fd b5 7e 00
[48648.442705] RSP: 0018:ffffb0f9f5e9bb90 EFLAGS: 00010202
[48648.450050] RAX: 0000000000000000 RBX: ffff9af130c86000 RCX: 0000000000000006
[48648.459476] RDX: 0000000000000006 RSI: ffff9ac35238ff00 RDI: ffff9ac3409d6000
[48648.468898] RBP: ffff9af130c86000 R08: 00000000000000c0 R09: 0000000000000000
[48648.478321] R10: 0000000000000001 R11: 00000000000000c1 R12: 0000000000000006
[48648.487796] R13: 0000000000000006 R14: ffff9af349248000 R15: ffff9af130c86000
[48648.497254] FS:  00007f0b3bfb9b80(0000) GS:ffff9b033f580000(0000) knlGS:0000000000000000
[48648.507768] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[48648.515676] CR2: 00007ffdf84d5cd0 CR3: 0000004089fc6000 CR4: 00000000003506e0
[48648.525197] Call Trace:
[48648.529612]  ? __warn+0x80/0x100
[48648.534753]  ? pci_enable_msix_range+0x1d/0x40
[48648.541227]  ? report_bug+0x9e/0xc0
[48648.546597]  ? handle_bug+0x41/0x90
[48648.551981]  ? exc_invalid_op+0x14/0x70
[48648.557708]  ? asm_exc_invalid_op+0x12/0x20
[48648.563818]  ? pci_enable_msix_range+0x1d/0x40
[48648.570473]  nv_init_msix+0x180/0x250 [nvidia]
[48648.577139]  nv_start_device+0x856/0x880 [nvidia]
[48648.584102]  nv_open_device+0x91/0x1a0 [nvidia]
[48648.590825]  nvidia_open+0x19a/0x5b0 [nvidia]
[48648.597341]  nvidia_frontend_open+0x53/0xa0 [nvidia]
[48648.604362]  chrdev_open+0xed/0x240
[48648.609732]  ? cdev_device_add+0xa0/0xa0
[48648.615603]  do_dentry_open+0x19c/0x3d0
[48648.621394]  do_open+0x1dc/0x320
[48648.626479]  path_openat+0x10b/0x1d0
[48648.631950]  ? bpf_lsm_path_notify+0x10/0x10
[48648.638220]  ? security_inode_alloc+0x45/0x90
[48648.644564]  do_filp_open+0xa1/0x150
[48648.650030]  ? __virt_addr_valid+0x104/0x130
[48648.656247]  ? is_vmalloc_or_module_addr+0x24/0x30
[48648.663031]  ? __check_object_size.part.0+0x135/0x1c0
[48648.670108]  do_sys_openat2+0x21c/0x310
[48648.675809]  __x64_sys_openat+0x54/0xa0
[48648.681511]  do_syscall_64+0x40/0x80
[48648.686904]  entry_SYSCALL_64_after_hwframe+0x67/0xcc
[48648.693929] RIP: 0033:0x7f0b3c195a1b
[48648.699322] Code: 4e 89 f0 25 00 00 41 00 3d 00 00 41 00 74 40 8b 05 5a d7 00 00 85 c0 75 61 89 f2 b8 01 01 00 00 48 89 fe bf 9c ff ff ff 0f 05 <48> 3d 00 f0 ff ff 0f 87 99 00 00 00 48 8b 4c 24 28 64 48 33 0c 25
[48648.722064] RSP: 002b:00007ffdf8449bd0 EFLAGS: 00000246 ORIG_RAX: 0000000000000101
[48648.731910] RAX: ffffffffffffffda RBX: 00007ffdf8449c50 RCX: 00007f0b3c195a1b
[48648.741254] RDX: 0000000000080002 RSI: 00007ffdf8449c50 RDI: 00000000ffffff9c
[48648.750608] RBP: 0000000000000000 R08: 0000000000000000 R09: 0000000000000000
[48648.760005] R10: 0000000000000000 R11: 0000000000000246 R12: 0000558f8522a600
[48648.769431] R13: 00007ffdf8449d8c R14: 0000000000000000 R15: 0000558f8522a600
[48648.778773] ---[ end trace d050dbb4aa691555 ]---
[48648.785482] NVRM: GPU 0000:c1:00.0: Failed to enable MSI-X.
[48648.809468] general protection fault, probably for non-canonical address 0x3020393535312030: 0000 [#1] SMP NOPTI
[48648.837398] Hardware name: Suma R6240H0/62DB32, BIOS SXYH041031 11/27/2023
[48648.846432] RIP: 0010:pci_read_config_dword+0x5/0x40
[48648.853324] Code: 44 89 c6 e9 5d fc ff ff b8 ff ff ff ff 66 89 02 b8 86 00 00 00 e9 9b 46 80 00 66 66 2e 0f 1f 84 00 00 00 00 00 0f 1f 44 00 00 <83> bf b4 00 00 00 03 48 89 d1 74 12 44 8b 47 38 48 8b 7f 10 89 f2
[48648.876036] RSP: 0018:ffffb0f9f5e9b980 EFLAGS: 00010293
[48648.883254] RAX: 0000000000000000 RBX: ffffb0f9cfcf9008 RCX: ffff9af34924ad6c
[48648.892579] RDX: ffff9af34924ad6c RSI: 0000000000000bf0 RDI: 3020393535312030
[48648.901914] RBP: ffff9af34924ad60 R08: 0000000000000020 R09: 0000000000000000
[48648.911247] R10: 0000000000088be0 R11: 000000009efe4d31 R12: ffffb0f9cfcf9008
[48648.920581] R13: ffff9af34924ad78 R14: 0000000000000bf0 R15: 0000000000000000
[48648.929936] FS:  00007f0b3bfb9b80(0000) GS:ffff9b033f500000(0000) knlGS:0000000000000000
[48648.940352] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[48648.948147] CR2: 00007fe425cda000 CR3: 0000004089fc6000 CR4: 00000000003506e0
[48648.957499] Call Trace:
[48648.961625]  ? __die_body.cold+0x8/0xd
[48648.967202]  ? die_addr+0x39/0x60
[48648.972290]  ? exc_general_protection+0x1c2/0x330
[48648.978929]  ? asm_exc_general_protection+0x1e/0x30
[48648.985804]  ? pci_read_config_dword+0x5/0x40
[48648.992298]  os_pci_read_dword+0x12/0x30 [nvidia]
[48648.999197]  _nv040058rm+0x15/0x30 [nvidia]
[48649.005586]  ? _nv025631rm+0x48/0xb0 [nvidia]
[48649.012062]  ? _nv032065rm+0x79/0x220 [nvidia]
[48649.018642]  ? _nv032073rm+0x12c/0x160 [nvidia]
[48649.025433]  ? _nv024984rm+0x114/0x250 [nvidia]
[48649.032276]  ? _nv024981rm+0xb5/0x130 [nvidia]
[48649.038964]  ? _nv025576rm+0x530/0xe20 [nvidia]
[48649.045629]  ? _nv025841rm+0x41d/0xbd0 [nvidia]
[48649.052306]  ? _nv000706rm+0x5bc/0x2080 [nvidia]
[48649.059074]  ? rm_init_adapter+0xcd/0xf0 [nvidia]
[48649.065712]  ? ttwu_queue_wakelist.part.0+0xb4/0xd0
[48649.072727]  ? nv_start_device+0x3d7/0x880 [nvidia]
[48649.079742]  ? nv_open_device+0x91/0x1a0 [nvidia]
[48649.086558]  ? nvidia_open+0x19a/0x5b0 [nvidia]
[48649.093197]  ? nvidia_frontend_open+0x53/0xa0 [nvidia]
[48649.100329]  ? chrdev_open+0xed/0x240
[48649.105788]  ? cdev_device_add+0xa0/0xa0
[48649.111523]  ? do_dentry_open+0x19c/0x3d0
[48649.117345]  ? do_open+0x1dc/0x320
[48649.122488]  ? path_openat+0x10b/0x1d0
[48649.128021]  ? bpf_lsm_path_notify+0x10/0x10
[48649.134132]  ? security_inode_alloc+0x45/0x90
[48649.140338]  ? do_filp_open+0xa1/0x150
[48649.145870]  ? __virt_addr_valid+0x104/0x130
[48649.152008]  ? is_vmalloc_or_module_addr+0x24/0x30
[48649.158742]  ? __check_object_size.part.0+0x135/0x1c0
[48649.165787]  ? do_sys_openat2+0x21c/0x310
[48649.171588]  ? __x64_sys_openat+0x54/0xa0
[48649.177379]  ? do_syscall_64+0x40/0x80
[48649.182872]  ? entry_SYSCALL_64_after_hwframe+0x67/0xcc
[48649.189997] Modules linked in: pci_pf_stub ebtable_filter ebtables ip6table_filter ip6_tables nfsv3 nfs_acl nfs lockd grace nfs_ssc fscache scsi_transport_iscsi nf_conntrack_netlink xt_comment xt_set iptable_filter ip_set_hash_net ip_set_bitmap_port ip_set overlay sch_ingress rdma_ucm(OE) rdma_cm(OE) iw_cm(OE) ib_ipoib(OE) ib_cm(OE) ib_umad(OE) nfnetlink_cttimeout nfnetlink openvswitch nf_conncount vfio_pci vfio_virqfd sunrpc vfat fat nvidia_vgpu_vfio(OE) ast drm_vram_helper drm_ttm_helper loop nvidia(POE) amd64_edac_mod ttm edac_mce_amd bonding drm_kms_helper syscopyarea vfio_mdev crct10dif_pclmul crc32_pclmul sysfillrect mdev kvm_amd sysimgblt fb_sys_fops vfio_iommu_type1 ses ghash_clmulni_intel cec enclosure scsi_transport_sas joydev rapl pcspkr vfio drm k10temp i2c_piix4 sg ipmi_ssif ccp kvm irqbypass acpi_ipmi ipmi_si nf_nat_ftp ipmi_devintf ipmi_msghandler ip_tables ext4 mbcache jbd2 mlx5_ib(OE) ib_uverbs(OE) ib_core(OE) sr_mod cdrom raid1 mlx5_core(OE) mlxfw(OE) igb
[48649.190069]  tls uas sd_mod nvme ahci psample nvme_core libahci i2c_algo_bit mlxdevm(OE) t10_pi mlx_compat(OE) dca usb_storage libata pci_hyperv_intf megaraid_sas hisdk3(OE) hinic3(OE) hiudk3(OE) fuse nf_conntrack_ftp nf_nat_tftp nf_conntrack_tftp nf_nat nf_conntrack nf_defrag_ipv6 libcrc32c crc32c_intel nf_defrag_ipv4
[48649.326917] kexec: Bye!
```

## 利用 virtme 虚拟机就可以了

vn 仓库

kimi -r session_a8ac9017-473b-40e6-bdb0-5b6b72cd1495

全部完成，测试通过。总结一下：

环境确认
• 直通的卡实际是 RTX 5060 Ti (GB206, sm_120)，位于 0000:01:00.0/.1，已绑 vfio-pci，virtme 虚拟机的 opt 里已配置
  直通（guest 内为 00:09.0）。
• ~/data/kernel/linux-drm 内核已是当前 HEAD 构建（7.1.2-00001-gfb512e2a3eed），且 virtme VM 正在用它运行，无需
  重新编译，所以没有动 ./build。

nvidia 驱动构建
• 本地 ~/data/open-gpu-kernel-modules 是 595.71.05，与本机用户态 libcuda.so.595.80 和 GSP 固件
  /usr/lib/firmware/nvidia/595.80/ 不匹配。
• fetch 了 595.80 tag，建了 worktree ~/data/open-gpu-kernel-modules-595.80，针对 linux-drm 构建成功：
  ```
    make -j SYSSRC=~/data/kernel/linux-drm SYSOUT=~/data/kernel/linux-drm modules
  ```

virtme 虚拟机内验证（通过 vsock SSH 登录）
• insmod nvidia.ko + nvidia-uvm.ko 成功；nvidia-modeset.ko 因内核未启用 backlight 配置缺符号而失败——只影响显示
  ，CUDA 不需要。
• 手动 mknod /dev/nvidiactl(195,255)、/dev/nvidia0(195,0)、/dev/nvidia-uvm(246,0) 后，nvidia-smi 正常：Driver
  595.80 / CUDA 13.2 / RTX 5060 Ti。

tutorial 编译运行（~/data/vn/gpu/cuda/tutorial）
途中解决了三个坑：
1. guest /tmp 不可写 → chmod 1777；
2. 系统 gcc 16 不被 nvcc 13.1 支持 → 用 nix gcc 14：编译时加
   HOST_CXX=/nix/store/61isrn2y99fxwv68yl95yzpc27xsqhm3-gcc-wrapper-14.3.0/bin/g++；
3. guest 的 /home 是 overlay，里面残留 6 月 14 日的旧 Makefile 遮蔽了 host 文件 → 把 host 版本同步进 guest。顺
   带修了 host 侧 Makefile 的 BASIC_TEST_BINS（引用了三个已不存在的源文件）。

测试结果：make basic-test 8 个程序全部 PASS（driver_probe、driver_module_load、memcpy_roundtrip、vector_add、
memory_bandwidth（kernel copy 355 GiB/s）、device_info（CC 12.0 Blackwell）、runtime_driver_device_info、
syncwarp_demo），另外 02.1-intro-to-cuda-cpp 和 04.01-unified-memory 章节 demo 也通过。

注意：guest 内编译产物写在 overlay tmpfs 上，VM 重启即丢失；重启后需要重新 insmod 模块和 mknod 设备节点。

2026-07-26 human 的提示
1. virtme 虚拟机应该使用自己的 tmpfs 才可以
2.


2. 此外，观察到了一个 nvidia 模块的 warning :
```txt
[84387.082003] nvidia: loading out-of-tree module taints kernel.
[84387.143929] nvidia-nvlink: Nvlink Core is being initialized, major device number 248

[84387.147714] nvidia 0000:00:09.0: vgaarb: pci_notify
[84387.147897] nvidia 0000:00:09.0: runtime IRQ mapping not provided by arch
[84387.148182] nvidia 0000:00:09.0: vgaarb: VGA decodes changed: olddecodes=io+mem,decodes=none:owns=io+mem
[84387.148435] nvidia 0000:00:09.0: vgaarb: decoding count now is: 0
[84389.748691] nvidia 0000:00:09.0: vgaarb: pci_notify
[84389.748958] NVRM: loading NVIDIA UNIX Open Kernel Module for x86_64  595.80  Release Build  (martins3@localhost.localdomain)  Sun Jul 26 11:05:51 AM CS
T 2026
[84389.790251] nvidia_modeset: Unknown symbol backlight_device_unregister (err -2)
[84389.790690] nvidia_modeset: Unknown symbol acpi_video_register_backlight (err -2)
[84389.791080] nvidia_modeset: Unknown symbol backlight_device_register (err -2)
[84389.791420] nvidia_modeset: Unknown symbol __acpi_video_get_backlight_type (err -2)
[84418.570670] ------------[ cut here ]------------
[84418.570845] DMA-API: nvidia 0000:00:09.0: mapping sg segment longer than device claims to support [len=528384] [max=65536]
[84418.571124] WARNING: kernel/dma/debug.c:1201 at debug_dma_map_sg+0x356/0x3b0, CPU#18: nv_open_q/4720
[84418.571359] Modules linked in: nvidia_uvm(O) nvidia(O) virtio_scsi virtio_net net_failover failover virtio_blk vmw_vsock_virtio_transport vmw_vsock_vir
tio_transport_common vsock overlay virtiofs fuse virtio_pci virtio_pci_legacy_dev virtio_pci_modern_dev [last unloaded: nova_core]
[84418.572074] CPU: 18 UID: 0 PID: 4720 Comm: nv_open_q Tainted: G           O        7.1.2-00001-gfb512e2a3eed #26 PREEMPT(lazy)
[84418.572358] Tainted: [O]=OOT_MODULE
[84418.572425] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS 0.0.0 02/06/2015
[84418.572635] RIP: 0010:debug_dma_map_sg+0x356/0x3b0
[84418.572737] Code: a8 89 4d b0 4c 89 4d b8 e8 57 33 6d 00 4c 8b 4d b8 8b 4d b0 48 8b 55 a8 44 8b 45 a4 48 89 c6 4c 89 4d b8 48 8d 3d 0a 58 88 01 <67> 48
 0f b9 3a 4c 8b 4d b8 8b 15 0b 14 87 01 85 d2 0f 85 2d fe ff
[84418.573210] RSP: 0018:ffffc900007274d8 EFLAGS: 00010282
[84418.573328] RAX: ffffffffc140b1b0 RBX: ffff888106d7b720 RCX: 0000000000081000
[84418.573497] RDX: ffff888101bf8160 RSI: ffffffffc140b1b0 RDI: ffffffff82c8d050
[84418.573660] RBP: ffffc90000727538 R08: 0000000000010000 R09: 00000000ffffffff
[84418.573824] R10: 0000000000000000 R11: 0000000000000001 R12: ffff88810116e280
[84418.573988] R13: ffff888102ae50c8 R14: 0000000000000001 R15: 0000000000000000
[84418.574153] FS:  0000000000000000(0000) GS:ffff8882b2fe2000(0000) knlGS:0000000000000000
[84418.574347] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[84418.574474] CR2: 00007fec4001a528 CR3: 000000000303e003 CR4: 0000000000f70ef0
[84418.574642] PKRU: 55555554
[84418.574687] Call Trace:
[84418.574726]  <TASK>
[84418.574762]  __dma_map_sg_attrs+0xa9/0x280
[84418.574865]  dma_map_sg_attrs+0x12/0x20
[84418.574963]  nv_map_dma_map_scatterlist+0x5f/0xa0 [nvidia]
[84418.575204]  nv_dma_map_scatterlist.constprop.0+0x83/0x260 [nvidia]
[84418.575406]  nv_dma_map_alloc+0x579/0x5a0 [nvidia]
[84418.575577]  osIovaMap+0x3c1/0x750 [nvidia]
[84418.575796]  iovaspaceAcquireMapping_IMPL+0x23a/0x4c0 [nvidia]
[84418.576028]  memdescMapIommu+0xa8/0x320 [nvidia]
[84418.576243]  memdescAlloc+0xaf0/0xd20 [nvidia]
[84418.576443]  ? __kmalloc_noprof+0x433/0x7d0
[84418.576544]  ? kvm_sched_clock_read+0x15/0x30
[84418.576645]  ? os_alloc_mem+0xf1/0x110 [nvidia]
[84418.576798]  ? _portMemAllocNonPagedUntracked+0x2c/0x40 [nvidia]
[84418.577002]  ? os_alloc_mem+0xf1/0x110 [nvidia]
[84418.577153]  ? _portMemAllocatorAlloc+0x2e/0xf0 [nvidia]
[84418.577338]  ? memdescCreate+0x103/0x450 [nvidia]
[84418.577553]  GspMsgQueuesInit+0x19d/0x5e0 [nvidia]
[84418.577765]  ? kgspConfigureFalcon_GA102+0x9e/0xc0 [nvidia]
[84418.578000]  kgspConstructEngine_IMPL+0x13d/0x780 [nvidia]
[84418.578230]  gpuCreateObject+0x11e/0x240 [nvidia]
[84418.578448]  gpuCreateChildObjects+0x110/0x480 [nvidia]
[84418.578681]  gpuPostConstruct_IMPL+0x6e0/0xaf0 [nvidia]
[84418.578915]  gpumgrAttachGpu+0x4ad/0xea0 [nvidia]
[84418.579107]  ? os_alloc_mem+0xf1/0x110 [nvidia]
[84418.579266]  RmInitAdapter+0x3d8/0x21c0 [nvidia]
[84418.579505]  ? up+0x64/0xb0
[84418.579561]  rm_init_adapter+0x135/0x140 [nvidia]
[84418.579791]  nv_start_device+0x256/0x800 [nvidia]
[84418.579945]  nv_open_device+0xa2/0x270 [nvidia]
[84418.580091]  nvidia_open_deferred+0x3a/0xf0 [nvidia]
[84418.580249]  _main_loop+0x85/0x140 [nvidia]
[84418.580399]  ? __pfx__main_loop+0x10/0x10 [nvidia]
[84418.580563]  kthread+0x10d/0x140
[84418.580629]  ? __pfx_kthread+0x10/0x10
[84418.580705]  ret_from_fork+0x2aa/0x3d0
[84418.580783]  ? __pfx_kthread+0x10/0x10
[84418.580883]  ret_from_fork_asm+0x1a/0x30
[84418.581000]  </TASK>
[84418.581045] irq event stamp: 39105
[84418.581135] hardirqs last  enabled at (39113): [<ffffffff813c5733>] __up_console_sem+0x63/0x80
[84418.581385] hardirqs last disabled at (39120): [<ffffffff813c5718>] __up_console_sem+0x48/0x80
[84418.581596] softirqs last  enabled at (38260): [<ffffffff8128bdfb>] kernel_fpu_end+0x3b/0x50
[84418.581797] softirqs last disabled at (38258): [<ffffffff8128c584>] kernel_fpu_begin_mask+0xd4/0x150
[84418.582015] ---[ end trace 0000000000000000 ]---
```
