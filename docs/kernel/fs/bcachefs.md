# bcachefs

bcachefs 功能比较齐全，实现的相当复杂，我使用的并不稳定，导致了数据丢失，
而且 bcachefs 进入到主线，然后又离开了主线，那都是小插曲了，如果你做文件系统，
bcachefs 必须要调研一下

## 基本操作

```sh
sudo bcachefs format /dev/sd[ab] \
    --foreground_target /dev/sda \
    --promote_target /dev/sda \
    --background_target /dev/sdb
sudo mount -t bcachefs /dev/sda:/dev/sdb /home/martins3/data
```

测试了效果，这个缓存就像是不存在一样，有趣啊!

```txt
/dev/sda:/dev/sdb  2.1T   11G  2.1T   1% /home/martins3/data
```

### 观测
这么多 workqueue ，都是做啥的 ?

```txt
🧀  ps -elf | grep bcachefs
1 I root          32       2  0  60 -20 -     0 -      May10 ?        00:00:00 [kworker/4:0H-bcachefs_io]
1 I root          74       2  0  60 -20 -     0 -      May10 ?        00:00:00 [kworker/17:0H-bcachefs_io]
1 I root         128       2  0  60 -20 -     0 -      May10 ?        00:00:00 [kworker/26:0H-bcachefs_io]
1 I root        1016       2  0  60 -20 -     0 -      May10 ?        00:00:00 [kworker/7:1H-bcachefs_io]
1 I root        1064       2  0  60 -20 -     0 -      May10 ?        00:00:00 [kworker/3:1H-bcachefs_io]
1 I root       70858       2  0  60 -20 -     0 -      12:20 ?        00:00:00 [kworker/9:5H-bcachefs_io]
1 I root       70894       2  0  80   0 -     0 -      12:20 ?        00:00:00 [kworker/13:0-bcachefs_btree_io]
1 I root       95360       2  0  60 -20 -     0 -      12:22 ?        00:00:00 [kworker/21:2H-bcachefs_io]
1 I root      141012       2  0  60 -20 -     0 -      12:24 ?        00:00:00 [kworker/30:3H-bcachefs_io]
1 I root      141057       2  0  60 -20 -     0 -      12:24 ?        00:00:00 [kworker/19:105H-bcachefs_io]
1 I root      141104       2  0  60 -20 -     0 -      12:24 ?        00:00:00 [kworker/13:49H-bcachefs_io]
1 I root      141243       2  0  60 -20 -     0 -      12:25 ?        00:00:00 [kworker/25:39H-bcachefs_io]
1 I root      142190       2  0  60 -20 -     0 -      12:26 ?        00:00:00 [kworker/11:1H-bcachefs_io]
```

### 性能测试

sudo bcachefs format /dev/sdc
sudo mount -t bcachefs /dev/sdc /mnt

sudo umount /mnt
sudo mkfs.xfs -f /dev/sdc
sudo mount /dev/sdc /mnt

居然吊打 xfs 和 ext4 ，但是 xfs 在 virtio host 800k 下只有 40k - 60k ，显然是哪里配置有问题!
怎么可能这么差!

### 移除磁盘

https://bcachefs-docs.readthedocs.io/en/latest/mgmt-deviceaddrm.html

如何这样，那还不错:
```sh
sudo bcachefs device evacuate /dev/sdb
sudo bcachefs device remove /dev/sdb
```

## bug

### 1
吓人，升级到 6.9.1 之后，mount 需要这么长的时间
```txt
[  942.982833] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): mounting version 1.4: member_seq opts=foreground_target=/dev/sdb,background_target=/dev/sda,promote_target=/dev/sdb
[  942.982838] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): recovering from clean shutdown, journal seq 296655
[  942.982841] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): Doing compatible version upgrade from 1.4: member_seq to 1.7: mi_btree_bitmap
                 running recovery passes: check_allocations,check_subvols,check_dirents
[  943.969405] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): alloc_read... done
[  944.472728] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): stripes_read... done
[  944.472731] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): snapshots_read... done
[  944.472735] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): check_allocations... done
[ 1153.565915] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): going read-write
[ 1153.580218] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): journal_replay... done
[ 1153.580221] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): check_subvols... done
[ 1153.583369] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): check_dirents... done
[ 1154.175194] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): resume_logged_ops... done
[ 1154.175200] bcachefs (4d4b1caa-2f4a-4fb7-a9cd-4042fa74ca14): delete_dead_inodes... done
```

### 2
```txt
[ 1717.181573] WARNING: CPU: 11 PID: 36741 at fs/bcachefs/btree_iter.c:2871 bch2_trans_put+0x23e/0x270 [bcachefs]
[ 1717.181692] CPU: 11 PID: 36741 Comm: clang Tainted: P           O       6.9.1 #1-NixOS
[ 1717.181694] Hardware name: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
[ 1717.181694] RIP: 0010:bch2_trans_put+0x23e/0x270 [bcachefs]
[ 1717.181715] Code: 40 db 48 c7 c7 30 29 b5 c6 48 b8 cf f7 53 e3 a5 9b c4 20 48 29 ca 48 c1 ea 03 48 f7 e2 48 89 d6 48 c1 ee 04 e8 23 df ca d9 90 <0f> 0b 90 90 8b b5 a8 00 00 00 49 8d be 68 36 00 00 83 fe 01 77 0a
[ 1717.181716] RSP: 0018:ffffaf50ec7cfa90 EFLAGS: 00010282
[ 1717.181717] RAX: 0000000000000000 RBX: ffff951d527d4000 RCX: 0000000000000027
[ 1717.181717] RDX: ffff952b7eda1848 RSI: 0000000000000001 RDI: ffff952b7eda1840
[ 1717.181718] RBP: ffff951d527d4000 R08: 0000000000000000 R09: 0000000000000003
[ 1717.181718] R10: ffffaf50ec7cf938 R11: ffffffffa1f38308 R12: ffff951f0b07e390
[ 1717.181719] R13: ffff951f0b07e390 R14: ffff951e79580000 R15: 0000000000000001
[ 1717.181719] FS:  00007f7b7168b180(0000) GS:ffff952b7ed80000(0000) knlGS:0000000000000000
[ 1717.181720] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[ 1717.181720] CR2: 00001d30042a4000 CR3: 00000003e9882000 CR4: 0000000000f50ef0
[ 1717.181721] PKRU: 55555554
[ 1717.181721] Call Trace:
[ 1717.181722]  <TASK>
[ 1717.181724]  ? __warn+0x80/0x120
[ 1717.181727]  ? bch2_trans_put+0x23e/0x270 [bcachefs]
[ 1717.181748]  ? report_bug+0x164/0x190
[ 1717.181750]  ? handle_bug+0x42/0x70
[ 1717.181751]  ? exc_invalid_op+0x17/0x70
[ 1717.181752]  ? asm_exc_invalid_op+0x1a/0x20
[ 1717.181754]  ? bch2_trans_put+0x23e/0x270 [bcachefs]
[ 1717.181777]  __bch2_create+0x334/0x410 [bcachefs]
[ 1717.181808]  ? bch2_create+0x2a/0x70 [bcachefs]
[ 1717.181834]  bch2_create+0x2a/0x70 [bcachefs]
[ 1717.181859]  path_openat+0xe74/0x1140
[ 1717.181862]  do_filp_open+0xc4/0x170
[ 1717.181864]  do_sys_openat2+0xab/0xe0
[ 1717.181865]  __x64_sys_openat+0x6e/0xa0
[ 1717.181867]  do_syscall_64+0xb9/0x200
[ 1717.181868]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
[ 1717.181869] RIP: 0033:0x7f7b64916aa5
[ 1717.181887] Code: 75 53 89 f0 25 00 00 41 00 3d 00 00 41 00 74 45 80 3d ce 28 0e 00 00 74 69 89 da 48 89 ee bf 9c ff ff ff b8 01 01 00 00 0f 05 <48> 3d 00 f0 ff ff 0f 87 8f 00 00 00 48 8b 54 24 28 64 48 2b 14 25
[ 1717.181887] RSP: 002b:00007fff24202b60 EFLAGS: 00000202 ORIG_RAX: 0000000000000101
[ 1717.181888] RAX: ffffffffffffffda RBX: 00000000000800c2 RCX: 00007f7b64916aa5
[ 1717.181889] RDX: 00000000000800c2 RSI: 00007fff24202d68 RDI: 00000000ffffff9c
[ 1717.181889] RBP: 00007fff24202d68 R08: 0000000000000000 R09: 00000000000001b6
[ 1717.181890] R10: 00000000000001b6 R11: 0000000000000202 R12: 00000000000001b6
[ 1717.181890] R13: 00007fff24202cec R14: 00007fff24202bf8 R15: 00007f7b7168b0b8
[ 1717.181891]  </TASK>
[ 1717.181891] ---[ end trace 0000000000000000 ]---
```
刚刚 mount 一会，然后会出现这个问题。

看上是 mount 之后，就不要随便搞这个东西了，很脆弱啊。

### 3
```txt
[  224.946176] ------------[ cut here ]------------
[  224.946178] btree trans held srcu lock (delaying memory reclaim) for 21 seconds
[  224.946187] WARNING: CPU: 2 PID: 9826 at fs/bcachefs/btree_iter.c:2871 bch2_trans_srcu_unlock+0x11b/0x130 [bcachefs]
[  224.946244] Modules linked in: bcachefs lz4hc_compress lz4_compress xor raid6_pq tcp_diag inet_diag wireguard curve25519_x86_64 libchacha20poly1305 chacha_x86_64 poly1305_x86_64 libcurve25519_generic libchacha ip6_udp_tunnel udp_tunnel nf_conntrack_netlink xfrm_user xfrm_algo xt_addrtype overlay qrtr xt_MASQUERADE xt_mark nft_chain_nat rfcomm ccm af_packet nfnetlink_cttimeout uhid cmac algif_hash algif_skcipher af_alg bnep xt_conntrack ip6t_rpfilter ipt_rpfilter msr xt_pkttype xt_LOG nf_log_syslog xt_tcpudp nft_compat nf_tables sch_fq_codel nvidia_drm(PO) nvidia_modeset(PO) nvidia_uvm(PO) atkbd libps2 vivaldi_fmap loop cpufreq_powersave xt_nat br_netfilter veth macvlan bridge stp llc intel_rapl_msr intel_rapl_common snd_sof_pci_intel_tgl openvswitch snd_sof_intel_hda_common intel_uncore_frequency intel_uncore_frequency_common intel_tcc_cooling nsh nf_conncount snd_soc_hdac_hda nf_nat x86_pkg_temp_thermal intel_powerclamp soundwire_intel nf_conntrack snd_sof_intel_hda_mlink soundwire_cadence nf_defrag_ipv6
[  224.946279]  snd_sof_intel_hda coretemp nf_defrag_ipv4 snd_sof_pci snd_sof_xtensa_dsp snd_sof iwlmvm kvm_intel crc32_pclmul polyval_clmulni snd_sof_utils snd_soc_acpi_intel_match polyval_generic gf128mul ghash_clmulni_intel soundwire_generic_allocation sha512_ssse3 snd_soc_acpi sha256_ssse3 sha1_ssse3 soundwire_bus aesni_intel crypto_simd cryptd snd_soc_avs xfs kvm snd_soc_hda_codec mac80211 snd_hda_ext_core nls_iso8859_1 nls_cp437 vfat fat snd_hda_codec_realtek snd_hda_codec_generic snd_hda_scodec_component nvidia(PO) snd_hda_codec_hdmi snd_soc_core libarc4 nvmet_tcp snd_compress ac97_bus asus_nb_wmi eeepc_wmi snd_pcm_dmaengine nvmet btusb asus_wmi btrtl battery snd_hda_intel platform_profile btintel i8042 snd_intel_dspcfg nvme_keyring snd_intel_sdw_acpi btbcm iTCO_wdt sparse_keymap cmdlinepart btmtk iwlwifi snd_hda_codec spi_nor libcrc32c vhost_net r8169 igc intel_pmc_bxt serio bluetooth rapl mei_hdcp mei_pxp mtd watchdog ee1004 wmi_bmof snd_hda_core video tun intel_cstate realtek ptp snd_hwdep cfg80211 mdio_devres
[  224.946313]  mousedev evdev input_leds mac_hid snd_pcm joydev libphy ecdh_generic intel_uncore vhost snd_timer mei_me pps_core ecc intel_pmc_core tpm_crb led_class snd vhost_iotlb i2c_i801 spi_intel_pci intel_lpss_pci mei intel_lpss spi_intel tap i2c_smbus soundcore pmt_telemetry tiny_power_button tpm_tis idma64 rfkill thermal intel_vsec virt_dma fan wmi backlight tpm_tis_core pinctrl_alderlake pmt_class acpi_tad acpi_pad button nfsd auth_rpcgss nfs_acl scsi_debug lockd null_blk grace vmd vfio_pci vfio_pci_core sunrpc vfio_iommu_type1 vfio iommufd fuse configfs efi_pstore nfnetlink efivarfs tpm rng_core dmi_sysfs ip_tables x_tables autofs4 ext4 crc32c_generic crc16 mbcache jbd2 hid_generic sd_mod usbhid hid ahci libahci libata xhci_pci xhci_pci_renesas nvme firmware_class nvme_core xhci_hcd scsi_mod nvme_auth t10_pi crc64_rocksoft crc64 crc_t10dif crc32c_intel rtc_cmos crct10dif_generic crct10dif_pclmul scsi_common crct10dif_common dm_mod dax
[  224.946352] CPU: 2 PID: 9826 Comm: git Tainted: P           O       6.9.7 #1-NixOS
[  224.946354] Hardware name: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
[  224.946355] RIP: 0010:bch2_trans_srcu_unlock+0x11b/0x130 [bcachefs]
[  224.946398] Code: e7 de 48 c7 c7 78 c6 6e c6 48 b8 cf f7 53 e3 a5 9b c4 20 48 29 ca 48 c1 ea 03 48 f7 e2 48 89 d6 48 c1 ee 04 e8 c6 fd 31 dd 90 <0f> 0b 90 90 e9 5f ff ff ff 90 0f 0b 90 e9 6c ff ff ff 0f 1f 00 90
[  224.946400] RSP: 0018:ffffaf78254979d0 EFLAGS: 00010282
[  224.946401] RAX: 0000000000000000 RBX: ffffa284fee18000 RCX: 0000000000000027
[  224.946402] RDX: ffffa2933e921848 RSI: 0000000000000001 RDI: ffffa2933e921840
[  224.946403] RBP: ffffa284fd4c0000 R08: 0000000000000000 R09: 0000000000000003
[  224.946404] R10: ffffaf7825497878 R11: ffffffffa553a128 R12: ffffa284fee18478
[  224.946404] R13: ffffa284fee18000 R14: 0000000000000006 R15: ffffa284fee18478
[  224.946405] FS:  00007f0e750bc740(0000) GS:ffffa2933e900000(0000) knlGS:0000000000000000
[  224.946406] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[  224.946407] CR2: 00007f0e34253004 CR3: 0000000203ab2002 CR4: 0000000000f70ef0
[  224.946408] PKRU: 55555554
[  224.946408] Call Trace:
[  224.946410]  <TASK>
[  224.946412]  ? __warn+0x80/0x120
[  224.946415]  ? bch2_trans_srcu_unlock+0x11b/0x130 [bcachefs]
[  224.946452]  ? report_bug+0x164/0x190
[  224.946454]  ? handle_bug+0x3d/0x80
[  224.946457]  ? exc_invalid_op+0x17/0x70
[  224.946458]  ? asm_exc_invalid_op+0x1a/0x20
[  224.946461]  ? bch2_trans_srcu_unlock+0x11b/0x130 [bcachefs]
[  224.946496]  ? bch2_trans_begin+0xf8/0x600 [bcachefs]
[  224.946530]  bch2_trans_begin+0x5a5/0x600 [bcachefs]
[  224.946564]  ? __bch2_create+0x354/0x5c0 [bcachefs]
[  224.946613]  __bch2_create+0x197/0x5c0 [bcachefs]
[  224.946663]  ? bch2_create+0x2a/0x60 [bcachefs]
[  224.946709]  bch2_create+0x2a/0x60 [bcachefs]
[  224.946753]  path_openat+0xe8a/0x1150
[  224.946756]  do_filp_open+0xc4/0x170
[  224.946760]  do_sys_openat2+0xab/0xe0
[  224.946762]  ? __do_sys_getcwd+0x15d/0x1e0
[  224.946765]  __x64_sys_openat+0x57/0xa0
[  224.946767]  do_syscall_64+0xb8/0x200
[  224.946769]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
[  224.946770] RIP: 0033:0x7f0e751bd2b2
[  224.946796] Code: 83 e2 40 75 53 89 f0 f7 d0 a9 00 00 41 00 74 48 80 3d a1 9d 0e 00 00 74 6c 89 da 48 89 ee bf 9c ff ff ff b8 01 01 00 00 0f 05 <48> 3d 00 f0 ff ff 0f 87 92 00 00 00 48 8b 54 24 28 64 48 2b 14 25
[  224.946797] RSP: 002b:00007ffc376104e0 EFLAGS: 00000202 ORIG_RAX: 0000000000000101
[  224.946798] RAX: ffffffffffffffda RBX: 00000000000800c2 RCX: 00007f0e751bd2b2
[  224.946799] RDX: 00000000000800c2 RSI: 0000000033d49fa0 RDI: 00000000ffffff9c
[  224.946800] RBP: 0000000033d49fa0 R08: 0000000000000007 R09: 0000000000000006
[  224.946800] R10: 00000000000001b6 R11: 0000000000000202 R12: 0000000033d49ac0
[  224.946801] R13: 0000000033d49f28 R14: 00000000000001b6 R15: 000000000077fdf0
[  224.946803]  </TASK>
[  224.946803] ---[ end trace 0000000000000000 ]---
[  257.316465] ------------[ cut here ]------------
[  257.316472] btree trans held srcu lock (delaying memory reclaim) for 18 seconds
[  257.316494] WARNING: CPU: 10 PID: 9987 at fs/bcachefs/btree_iter.c:2871 bch2_trans_put+0x23e/0x270 [bcachefs]
```

## 资料
- https://bcachefs.org/bcachefs-principles-of-operation.pdf
- https://lwn.net/Articles/394672/
- https://lwn.net/Articles/895266/

- https://bcachefs.org/
- https://news.ycombinator.com/item?id=10096735 : 我靠，原来花费了 10 年的时间写的!
- https://nixos.wiki/wiki/Bcachefs : 先用起来，来个基本的感受吧
- https://bcachefs.org/Architecture/ : 架构设计，从 btree 开始
- https://hn.algolia.com/?q=bcachefs : 看上去大家真的很有兴趣

https://news.ycombinator.com/item?id=41076190 : 有趣，但是需要深入到代码中

https://www.reddit.com/r/linux/comments/1dr8df1/anyone_daily_driving_bcachefs_what_are_your/ : 大伙的态度已经开始微妙起来了

- https://www.phoronix.com/news/Linus-Torvalds-Bcachefs-Regrets
- https://www.phoronix.com/news/Bcachefs-Fixes-Two-Choices
	- 如何和其他人工作的确是一个值得思考的问题
- https://github.com/koverstreet/bcachefs/issues
	- 这里的问题尝试修一修，kent 恢复都很快

- https://linuxunplugged.com/644 : 关于 kent 的访谈， 能力有限，得改善一下听力了

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
