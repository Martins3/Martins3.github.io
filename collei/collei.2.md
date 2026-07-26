2. dpdk + ovs 环境

## 一些失败的想法

首先，引入的变量实在是太多了:

1. 存在哪些共享
  - host 共享 vn 到 /home/core/vn
    - 这个是为了，L1 方便的执行 mod 相关的代码
    - L1 可以容器可以方便的执行 env.sh (伪需求)
2. 让 podman 可以长期运行，开机自动启动，自动 ssh 到容器中
  - 可以有其他的什么无状态的方法吗?
3. https://github.com/psviderski/unregistry


## 其实在 x86 上调试 arm 的东西也不错的

似乎 tcg 性能没有想象的差

## kimi 教我如何安装虚拟机
### 4.2 完整示例：从 ISO 安装虚拟机

以下示例使用 Fedora 43 ISO 进行无人值守安装：

```bash
#!/bin/bash
set -E -e -u -o pipefail

QEMU=~/data/qemu/build/qemu-system-x86_64
ISO=~/data/hack/iso/Fedora-Server-dvd-x86_64-43-1.6.iso
DISK=~/data/hack/vm/fedora-mmoc/fedora-mmoc.raw
KS=~/data/hack/vm/fedora-mmoc/ks.cfg
SOCK=/tmp/connection-uds-fd
MEM_SIZE=8G

# 创建磁盘（如不存在）
[ -f "${DISK}" ] || qemu-img create -f raw "${DISK}" 60G

# 提取内核/initrd（用于 kickstart 安装）
mkdir -p ~/data/hack/vm/fedora-mmoc/boot
if [ ! -f ~/data/hack/vm/fedora-mmoc/boot/vmlinuz ]; then
  sudo mount -o loop "${ISO}" /mnt
  cp /mnt/images/pxeboot/vmlinuz ~/data/hack/vm/fedora-mmoc/boot/
  cp /mnt/images/pxeboot/initrd.img ~/data/hack/vm/fedora-mmoc/boot/
  sudo umount /mnt
fi

# 启动 http.server 提供 kickstart 文件（后台）
python3 -m http.server 8000 --directory ~/data/hack/vm/fedora-mmoc &
HTTP_PID=$!
trap "kill ${HTTP_PID}" EXIT

# 启动 QEMU
${QEMU} \
  -uuid 12345678-1234-1234-1234-123456789abc \
  -machine pc,hpet=off \
  -accel kvm \
  -cpu host \
  -smp 4 \
  -m ${MEM_SIZE},slots=8,maxmem=256G \
  -chardev socket,id=mmoc-chrdev,path=${SOCK},reconnect-ms=50 \
  -object memory-backend-memfd,id=mem0,size=${MEM_SIZE},share=on,mmoc=on,swap-storage=file://local/tmp/mmoc-swap \
  -numa node,nodeid=0,memdev=mem0 \
  -drive file="${DISK}",format=raw,if=virtio \
  -cdrom "${ISO}" \
  -kernel ~/data/hack/vm/fedora-mmoc/boot/vmlinuz \
  -initrd ~/data/hack/vm/fedora-mmoc/boot/initrd.img \
  -append "inst.ks=http://10.0.2.2:8000/ks.cfg console=ttyS0,115200n8" \
  -serial stdio \
  -netdev user,id=net0,hostfwd=tcp::2222-:22 \
  -device virtio-net-pci,netdev=net0
```

kickstart 示例（`ks.cfg`）：

```
lang en_US.UTF-8
keyboard us
timezone UTC --utc
rootpw --plaintext root
user --name=martin --password=martin --plaintext
text
reboot
cdrom
bootloader --location=mbr --append="console=ttyS0,115200n8"
zerombr
clearpart --all --initlabel
autopart --type=plain
network --bootproto=dhcp --device=link --activate
firewall --disabled
selinux --enforcing
services --enabled=sshd,serial-getty@ttyS0.service
skipx
%packages
@^server-product-environment
%end
%post
systemctl enable serial-getty@ttyS0.service
%end
```

## 为什么使用 cgroup 这么设计

```txt
systemd-run --user --scope --collect --unit="$vm_name" bash "$vm_dir/cmd.sh"
```

1. 现在的配置，启动脚本，默认在 cgroup 中

cat /proc/$$/cgroup
```txt
0::/user.slice/user-1000.slice/user@1000.service/app.slice/tmux-spawn-e6e3b687-d889-4e7e-b20e-86c3bd4d0977.scope

0::/user.slice/user-1000.slice/user@1000.service/app.slice/app-ghostty-surface-transient-1460744.scope
```

有点不可控，例如在 tmux nvim 的 terminal 中启动 qemu ，那么就是这样的:

```txt
cat /proc/410478/comm
cat /proc/429212/comm
cat /proc/429300/comm
cat /proc/429301/comm
cat /proc/429310/comm
cat /proc/429311/comm
cat /proc/436781/comm
cat /proc/436909/comm
cat /proc/437133/comm
cat /proc/437134/comm

zsh
.tig-wrapped
nvim
nvim
node
efm-langserver
zsh
bash
bash
qemu-system-x86
```

## 这意味着什么?
搭建的好处就是，可以自动的构建:
```txt
[543272.201563] nbd0: detected capacity change from 0 to 734003200
[543272.203274]  nbd0: p1 p2 p3
[543308.569908] EXT4-fs (dm-1): recovery complete
[543308.570700] EXT4-fs (dm-1): mounted filesystem 13ca871a-5612-4dcf-81bf-a77c0236af63 r/w with ordered data mode. Quota mode: none.
[543316.477444] BUG: unable to handle page fault for address: ffffc9000214fca8
[543316.477450] #PF: supervisor read access in kernel mode
[543316.477451] #PF: error_code(0x0000) - not-present page
[543316.477452] PGD 100000067 P4D 100000067 PUD 100626067 PMD 10b2eb067 PTE 0
[543316.477456] Oops: Oops: 0000 [#2] SMP NOPTI
[543316.477458] CPU: 24 UID: 0 PID: 615205 Comm: insmod Kdump: loaded Tainted: G      D W  OE       7.1.3-200.fc44.x86_64 #1 PREEMPT(lazy)
[543316.477461] Tainted: [D]=DIE, [W]=WARN, [O]=OOT_MODULE, [E]=UNSIGNED_MODULE
[543316.477462] Hardware name: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
[543316.477463] RIP: 0010:idempotent_init_module+0x104/0x360
[543316.477468] Code: 48 c1 eb 38 48 8b 14 dd 00 84 c6 84 48 8d 2c dd 00 84 c6 84 48 85 d2 0f 84 12 01 00 00 48 89 d0 48 83 e8 08 0f 84 08 02 00 00 <4c> 3b 20 74 3b 48 8b 40 08 48 85 c0 74 06 48 83 e8 08 75 ec 48 8d
[543316.477469] RSP: 0018:ffffc90027f5fdd8 EFLAGS: 00010292
[543316.477471] RAX: ffffc9000214fca8 RBX: 0000000000000075 RCX: ffff8881626a2ca0
[543316.477472] RDX: ffffc9000214fcb0 RSI: ffffffff831b48e7 RDI: ffffffff84c683e8
[543316.477473] RBP: ffffffff84c687a8 R08: ffff889fff418cc0 R09: 0000000000000000
[543316.477474] R10: ffffffff83c6a880 R11: ffffffff83c6a880 R12: ffff8885990244b0
[543316.477475] R13: ffff888d51d14180 R14: 000055cc2e4e2010 R15: 0000000000000000
[543316.477476] FS:  00007f2c52fdd780(0000) GS:ffff88a07a897000(0000) knlGS:0000000000000000
[543316.477477] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[543316.477478] CR2: ffffc9000214fca8 CR3: 00000017ab98c001 CR4: 0000000000f72ef0
[543316.477479] PKRU: 55555554
[543316.477480] Call Trace:
[543316.477482]  <TASK>
[543316.477484]  __x64_sys_finit_module+0x7f/0x110
[543316.477486]  do_syscall_64+0xe2/0x560
[543316.477490]  ? do_user_addr_fault+0x2cd/0x840
[543316.477493]  ? irqentry_exit+0x45/0x6d0
[543316.477495]  ? do_syscall_64+0x99/0x560
[543316.477496]  ? exc_page_fault+0x90/0x1f0
[543316.477498]  entry_SYSCALL_64_after_hwframe+0x76/0x7e
[543316.477501] RIP: 0033:0x7f2c528fb3fd
[543316.477534] Code: ff c3 66 2e 0f 1f 84 00 00 00 00 00 90 f3 0f 1e fa 48 89 f8 48 89 f7 48 89 d6 48 89 ca 4d 89 c2 4d 89 c8 4c 8b 4c 24 08 0f 05 <48> 3d 01 f0 ff ff 73 01 c3 48 8b 0d cb a9 0f 00 f7 d8 64 89 01 48
[543316.477535] RSP: 002b:00007ffccac60b08 EFLAGS: 00000246 ORIG_RAX: 0000000000000139
[543316.477537] RAX: ffffffffffffffda RBX: 000055cc2e4e2570 RCX: 00007f2c528fb3fd
[543316.477538] RDX: 0000000000000000 RSI: 000055cc2e4e2010 RDI: 0000000000000003
[543316.477538] RBP: 00007ffccac60ba0 R08: 0000000000000000 R09: 000055cc2e4e2030
[543316.477539] R10: 0000000000000000 R11: 0000000000000246 R12: 000055cc2e4e2010
[543316.477540] R13: 000055cc2e4e2520 R14: 0000000000000000 R15: 000055cc2e4e2570
[543316.477542]  </TASK>
[543316.477543] Modules linked in: force_swiotlb_for_pci(OE+) nbd vhost_vsock vsock_loopback vmw_vsock_virtio_transport_common vmw_vsock_vmci_transport vsock vmw_vmci tls isofs loop xt_CHECKSUM xt_multiport uinput vhost_net vhost vhost_iotlb tap xt_connmark xt_mark rfcomm snd_seq_dummy snd_hrtimer nf_conntrack_netlink xt_nat veth xt_conntrack xt_MASQUERADE bridge stp llc xt_set ip_set nft_chain_nat xt_addrtype nft_compat rpcrdma rdma_cm iw_cm ib_cm ib_core overlay binfmt_misc tun nf_tables nfnetlink_cttimeout nfnetlink openvswitch nsh nf_conncount nf_nat nf_conntrack nf_defrag_ipv6 nf_defrag_ipv4 psample qrtr des_generic libdes bnep md4 vfat fat snd_sof_pci_intel_tgl snd_sof_pci_intel_cnl snd_sof_intel_hda_generic soundwire_intel snd_sof_intel_hda_sdw_bpt snd_sof_intel_hda_common snd_soc_hdac_hda snd_sof_intel_hda_mlink snd_sof_intel_hda soundwire_cadence snd_sof_pci snd_sof_xtensa_dsp intel_rapl_msr intel_rapl_common snd_sof iwlmvm intel_uncore_frequency intel_uncore_frequency_common intel_tcc_cooling snd_sof_utils
[543316.477581]  snd_soc_acpi_intel_match x86_pkg_temp_thermal snd_soc_acpi_intel_sdca_quirks intel_powerclamp soundwire_generic_allocation snd_soc_sdw_utils coretemp snd_soc_acpi crc8 snd_hda_codec_intelhdmi soundwire_bus snd_hda_codec_hdmi snd_soc_sdca mac80211 snd_hda_codec_alc662 kvm_intel snd_soc_avs snd_hda_codec_realtek_lib snd_hda_codec_generic snd_soc_hda_codec libarc4 snd_hda_ext_core snd_hda_intel kvm snd_soc_core snd_hda_codec snd_hda_core asus_armoury snd_compress snd_intel_dspcfg btusb ac97_bus snd_intel_sdw_acpi snd_hwdep snd_pcm_dmaengine snd_seq btmtk iwlwifi btrtl rapl spi_nor snd_seq_device btbcm intel_cstate btintel iTCO_wdt snd_pcm ee1004 mtd eeepc_wmi intel_pmc_bxt r8169 firmware_attributes_class asus_wmi mei_pxp mei_hdcp i2c_i801 realtek snd_timer spi_intel_pci bluetooth intel_uncore igc wmi_bmof sparse_keymap cfg80211 intel_pmc_core snd phy_package spi_intel i2c_smbus idma64 soundcore joydev mei_me pmt_telemetry pmt_discovery pmt_class apple_mfi_fastcharge mei intel_pmc_ssram_telemetry acpi_tad
[543316.477617]  acpi_pad rfkill nfsd auth_rpcgss nfs_acl lockd grace nfs_localio sunrpc fuse dm_multipath zram lz4hc_compress lz4_compress xfs xe hid_apple drm_ttm_helper drm_suballoc_helper gpu_sched drm_gpuvm drm_exec drm_gpusvm_helper i915 drm_buddy i2c_algo_bit ttm nvme drm_display_helper video nvme_core intel_oc_wdt intel_vsec pinctrl_alderlake nvme_keyring cec wmi nvme_auth vfio_pci vfio_pci_core irqbypass vfio_iommu_type1 vfio iommufd scsi_dh_alua pkcs8_key_parser i2c_dev scsi_dh_emc scsi_dh_rdac
[543316.477640] Unloaded tainted modules: force_swiotlb_for_pci(OE):1 [last unloaded: nbd]
[543316.477643] CR2: ffffc9000214fca8
[543316.477644] ---[ end trace 0000000000000000 ]---
[543316.477646] RIP: 0010:exc_control_protection+0x1bf/0x1d0
[543316.477648] Code: e8 b9 09 00 00 00 48 8b 95 80 00 00 00 be 81 00 00 00 48 c7 c7 89 5e 1b 83 e8 dd 9a c6 fe 80 a5 8a 00 00 00 fb e9 74 fe ff ff <0f> 0b 66 2e 0f 1f 84 00 00 00 00 00 0f 1f 44 00 00 90 90 90 90 90
[543316.477650] RSP: 0018:ffffc9000214fa50 EFLAGS: 00010002
[543316.477651] RAX: 000000000000002a RBX: 0000000000000003 RCX: 0000000000000000
[543316.477652] RDX: 0000000000000000 RSI: 0000000000000001 RDI: ffff889fff29d440
[543316.477653] RBP: ffffc9000214fa78 R08: 0000000000000000 R09: ffffffff83d692d0
[543316.477654] R10: 3fffffffffffdfff R11: ffffc9000214f908 R12: 0000000000000000
[543316.477654] R13: 0000000000000000 R14: 0000000000000000 R15: 0000000000000000
[543316.477655] FS:  00007f2c52fdd780(0000) GS:ffff88a07a897000(0000) knlGS:0000000000000000
[543316.477656] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[543316.477657] CR2: ffffc9000214fca8 CR3: 00000017ab98c001 CR4: 0000000000f72ef0
[543316.477658] PKRU: 55555554
[543316.477659] note: insmod[615205] exited with irqs disabled
[543316.477667] note: insmod[615205] exited with preempt_count 1
[543343.024604] EXT4-fs (dm-1): unmounting filesystem 13ca871a-5612-4dcf-81bf-a77c0236af63.
[543343.165896] block nbd0: NBD_DISCONNECT
[543343.165905] block nbd0: Disconnected due to user request.
[543343.165906] block nbd0: shutting down sockets
[543349.861038] kvm_do_msr_access: 26 callbacks suppressed
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
