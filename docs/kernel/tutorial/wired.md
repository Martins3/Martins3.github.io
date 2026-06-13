# 记录一些极其奇怪的问题

## 一台机器无法启动

使用 tty 来观察，发现 ipmi 启动到这里就结束了，而 bmc 始终无反应
```txt
pci 0000:40:03.3: PCI bridge to [bus 41-42]
pci 0000:40:03.3:   bridge window [io  0x3000-0x3fff]
pci 0000:40:03.3:   bridge window [mem 0xc6800000-0xc6afffff]
pci 0000:40:03.3:   bridge window [mem 0x2bf40000000-0x2bf4/input/input1
ACPI: button: Power Button [PWRF]
Serial: 8250/16550 driver, 4 ports, IRQ sharing disabled
```
最后发现，这个机器本来是 ttyS0 ，然后启动之后，会切换到 ttyS1 上。

## 内核警告 ACPI: Unable to map lapic to logical cpu number

原来是默认配置的 CPU 数量太少了。

默认只有 64 ，添加配置 CONFIG_NR_CPUS=128 即可

## kunpeng 机器有这个警告
kunpeng 使用自己构建的内核有这个报错
```txt
[   26.608388] platform HISI0213:08: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.615602] platform HISI0233:04: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.622895] platform HISI0243:06: deferred probe pending: hisi_hha_pmu: IRQ index 0 not found
[   26.630102] platform HISI0213:09: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.637309] platform HISI0233:05: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.644598] platform HISI0213:0a: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.651800] platform HISI0243:07: deferred probe pending: hisi_hha_pmu: IRQ index 0 not found
[   26.659012] platform HISI0233:06: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.666303] platform HISI0213:0b: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.673506] platform HISI0213:0c: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.680707] platform HISI0233:07: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.687999] platform HISI0233:08: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.695290] platform HISI0233:09: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.702581] platform HISI0213:00: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.709780] platform HISI0213:0d: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.716982] platform HISI0233:0a: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.724277] platform HISI0233:0b: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.731569] platform HISI0233:0c: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.738860] platform HISI0233:0d: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.746152] platform HISI0233:00: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.753452] platform HISI0213:10: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.760656] platform HISI0233:0e: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.767944] platform HISI0213:01: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.775147] platform HISI0243:00: deferred probe pending: hisi_hha_pmu: IRQ index 0 not found
[   26.782347] platform HISI0213:11: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.789550] platform HISI0233:0f: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.796839] platform HISI0233:01: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.804126] platform HISI0213:12: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.811330] platform HISI0213:02: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.818535] platform HISI0213:13: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.825735] platform HISI0243:01: deferred probe pending: hisi_hha_pmu: IRQ index 0 not found
[   26.832942] platform HISI0213:14: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.840149] platform HISI0243:02: deferred probe pending: hisi_hha_pmu: IRQ index 0 not found
[   26.847348] platform HISI0213:15: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.854552] platform HISI0213:03: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.861756] platform HISI0213:18: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.868958] platform HISI0233:02: deferred probe pending: hisi_ddrc_pmu: IRQ index 0 not found
[   26.876246] platform HISI0213:19: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.883452] platform HISI0243:03: deferred probe pending: hisi_hha_pmu: IRQ index 0 not found
[   26.890656] platform HISI0213:1a: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.897857] platform HISI0213:04: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.905059] platform HISI0213:1b: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.912259] platform HISI0243:04: deferred probe pending: hisi_hha_pmu: IRQ index 0 not found
[   26.919464] platform HISI0213:1c: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.926672] platform HISI0213:05: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
[   26.933875] platform HISI0213:1d: deferred probe pending: hisi_l3c_pmu: IRQ index 0 not found
```

## 19.60 ARM 的 megaraid 卡问题

现在内核是 6.6 的:
```txt
[1211758.307523][   C85] sd 0:0:13:0: [sde] tag#57 BRCM Debug mfi stat 0x2d, data len requested/completed 0x11000/0x0
[1211758.309552][   C14] sd 0:0:13:0: [sde] tag#56 BRCM Debug mfi stat 0x2d, data len requested/completed 0x31000/0x0
[1211758.329644][   C14] sd 0:0:13:0: [sde] tag#55 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[1211758.352841][   C14] sd 0:0:13:0: [sde] tag#54 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[1211758.363618][   C14] sd 0:0:13:0: [sde] tag#53 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[1211758.374398][   C14] sd 0:0:13:0: [sde] tag#52 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[1211758.385195][   C14] sd 0:0:13:0: [sde] tag#51 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[1211758.396001][   C14] sd 0:0:13:0: [sde] tag#50 BRCM Debug mfi stat 0x2d, data len requested/completed 0x2000/0x0
[1211758.406726][   C14] sd 0:0:13:0: [sde] tag#49 BRCM Debug mfi stat 0x2d, data len requested/completed 0x3e000/0x0
[1211758.417550][   C14] sd 0:0:13:0: [sde] tag#48 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[1211758.428396][   C14] sd 0:0:13:0: [sde] tag#47 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[1211758.439268][   C14] sd 0:0:13:0: [sde] tag#46 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[1211758.450160][   C14] sd 0:0:13:0: [sde] tag#45 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[1211758.461069][   C14] sd 0:0:13:0: [sde] tag#44 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[1211758.471985][   C14] sd 0:0:13:0: [sde] tag#43 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[1211758.482917][   C14] sd 0:0:13:0: [sde] tag#42 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[1211758.639468][   C85] sd 0:0:13:0: [sde] tag#58 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=7s
[1211758.640372][   C14] sd 0:0:13:0: [sde] tag#59 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=13s
[1211758.640535][   C85] sd 0:0:13:0: [sde] tag#58 CDB: Write(10) 2a 00 17 0f 7b 28 00 00 88 00
[1211758.641452][   C14] sd 0:0:13:0: [sde] tag#59 CDB: Write(10) 2a 00 15 93 7a 00 00 01 88 00
[1211758.642329][   C85] I/O error, dev sde, sector 386890536 op 0x1:(WRITE) flags 0x9800 phys_seg 17 prio class 2
[1211758.643139][   C14] I/O error, dev sde, sector 361986560 op 0x1:(WRITE) flags 0x800 phys_seg 23 prio class 2
[1211758.644057][ T1445] Aborting journal on device sde4-8.
[1211758.644940][   C14] sd 0:0:13:0: [sde] tag#60 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=13s
[1211758.645586][ T6527] EXT4-fs error (device sde4): ext4_journal_check_start:85: comm rs:main Q:Reg: Detected aborted journal
[1211758.646448][   C14] sd 0:0:13:0: [sde] tag#60 CDB: Write(10) 2a 00 15 93 78 00 00 02 00 00
[1211758.646451][   C14] I/O error, dev sde, sector 361986048 op 0x1:(WRITE) flags 0x4800 phys_seg 30 prio class 2
[1211758.752580][   C14] sd 0:0:13:0: [sde] tag#63 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=13s
[1211758.763924][   C14] sd 0:0:13:0: [sde] tag#63 CDB: Write(10) 2a 00 15 93 76 00 00 02 00 00
[1211758.772930][   C14] I/O error, dev sde, sector 361985536 op 0x1:(WRITE) flags 0x4800 phys_seg 29 prio class 2
[1211758.783586][   C14] sd 0:0:13:0: [sde] tag#0 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=13s
[1211758.794842t: dx_probe:823: inode #3154052: lblock 0: comm tuned: error -5 reading directory block
[1211760.580387][ T7050] EXT4-fs warning (device sde4): dx_probe:823: inode #3154052: lblock 0: comm tuned: error -5 reading directory block
[1211760.588993][T876871] EXT4-fs warning (device sde4): dx_probe:823: inode #3145731: lblock 0: comm (rpc.nfsd): error -5 reading directory block
[1211760.593411][ T7050] EXT4-fs warning (device sde4): dx_probe:823: inode #3154052: lblock 0: comm tuned: error -5 reading directory block
[1211760.599070][T876874] EXT4-fs warning (device sde4): dx_probe:823: inode #3145731: lblock 0: comm (exportfs): error -5 reading directory block
[1211760.608555][ T7050] EXT4-fs warning (device sde4): dx_probe:823: inode #3154052: lblock 0: comm tuned: error -5 reading directory block
[1211760.615675][ T6242] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm rpcbind: error -5 reading directory block
[1211760.620671][T62830] sd 0:0:13:0: [sde] Synchronizing SCSI cache
[1211760.622064][ T7050] EXT4-fs warning (device sde4): dx_probe:823: inode #3154052: lblock 0: comm tuned: error -5 reading directory block
[1211760.624482][ T6242] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm rpcbind: error -5 reading directory block
[1211760.839728][T62830] sd 0:0:13:0: [sde] Synchronize Cache(10) failed: Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK
[1211761.057558][T62830] megaraid_sas 0000:87:00.0: scanning for scsi0...
[1211772.516667][T62830] scsi 0:0:13:0: Direct-Access     ATA      DSS200-B 240GB   T6.0 PQ: 0 ANSI: 6
[1211772.712623][ T1594] EXT4-fs warning: 47 callbacks suppressed
[1211772.712624][T62830] sd 0:0:13:0: Attached scsi generic sg1 type 0
[1211772.712628][ T1594] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm systemd-udevd: error -5 reading directory block
[1211772.713417][T872621] sd 0:0:13:0: [sdg] 468862128 512-byte logical blocks: (240 GB/224 GiB)
[1211772.713527][ T1594] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm systemd-udevd: error -5 reading directory block
[1211772.715237][T872621] sd 0:0:13:0: [sdg] Write Protect is off
[1211772.716326][ T1594] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm systemd-udevd: error -5 reading directory block
[1211772.723221][T62830] megaraid_sas 0000:87:00.0: scanning for scsi0...
[1211772.723405][T872621] sd 0:0:13:0: [sdg] Write cache: enabled, read cache: enabled, supports DPO and FUA
[1211772.737359][ T1594] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm systemd-udevd: error -5 reading directory block
[1211772.774061][T872621]  sdg: sdg1 sdg2 sdg3 sdg4
[1211772.788407][ T1594] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm systemd-udevd: error -5 reading directory block
[1211772.792768][T872621] sd 0:0:13:0: [sdg] Attached SCSI disk
[1211772.802312][ T1594] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm systemd-udevd: error -5 reading directory block
[1211772.858075][ T7050] EXT4-fs warning (device sde4): dx_probe:823: inode #3154052: lblock 0: comm tuned: error -5 reading directory block
[1211772.872216][ T7050] EXT4-fs warning (device sde4): dx_probe:823: inode #3154052: lblock 0: comm tuned: error -5 reading directory block
[1211772.885317][ T7050] EXT4-fs warning (device sde4): dx_probe:823: inode #3154052: lblock 0: comm tuned: error -5 reading directory block
[1211772.902514][ T7050] EXT4-fs warning (device sde4): dx_probe:823: inode #3154052: lblock 0: comm tuned: error -5 reading directory block
[1211789.163961][ T7978] EXT4-fs warning: 12 callbacks suppressed
[1211789.163967][ T7978] EXT4-fs warning (device sde4): dx_probe:823: inode #3145731: lblock 0: comm containerd: error -5 reading directory block
[1211789.187926][ T7978] EXT4-fs warning (device sde4): dx_probe:823: inode #3145730: lblock 0: comm containerd: error -5 reading directory block
[1211789.205576][ T7978] EXT4-fs warning (device sde4): dx_probe:823: inode #3145731: lblock 0: comm containerd: error -5 reading directory block
[1211789.219877][ T7978] EXT4-fs warning (device sde4): dx_probe:823: inode #3145730: lblock 0: comm containerd: error -5 reading directory block
[1211789.242245][T872621] docker0: port 1(veth29851c0) entered disabled state
[1211789.249767][ T6985] vethbbed7a3: renamed from eth0
[1211789.288107][ T1594] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm systemd-udevd: error -5 reading directory block
[1211789.301912][    T1] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm systemd: error -5 reading directory block
[1211789.315443][    T1] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm systemd: error -5 reading directory block
[1211789.315540][ T6985] SELinux: inode_doinit_use_xattr:  getxattr returned 5 for dev=sde4 ino=4239908
[1211789.316620][    T1] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm systemd: error -5 reading directory block
[1211789.317633][ T6985] SELinux: inode_doinit_use_xattr:  getxattr returned 5 for dev=sde4 ino=4239908
[1211789.318822][    T1] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm systemd: error -5 reading directory block
[1211789.319913][ T6985] SELinux: inode_doinit_use_xattr:  getxattr returned 5 for dev=sde4 ino=4239908
[1211789.323941][    T1] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm systemd: error -5 reading directory block
[1211789.337371][ T6985] SELinux: inode_doinit_use_xattr:  getxattr returned 5 for dev=sde4 ino=4513557
[1211789.410744][ T6985] SELinux: inode_doinit_use_xattr:  getxattr returned 5 for dev=sde4 ino=4513557
[1211806.045778][T876924] EXT4-fs warning: 12 callbacks suppressed
[1211806.045785][T876924] EXT4-fs warning (device sde4): dx_probe:823: inode #1572866: lblock 0: comm zsh: error -5 reading directory block
[1211806.181830][T876928] EXT4-fs warning (device sde4): dx_probe:823: inode #1572866: lblock 0: comm zsh: error -5 reading directory block
[1211807.374460][T876933] EXT4-fs warning (device sde4): dx_probe:823: inode #3145730: lblock 0: comm zsh: error -5 reading directory block
[1211807.391629][T876933] EXT4-fs warning (device sde4): dx_probe:823: inode #3145731: lblock 0: comm zsh: error -5 reading directory block
[1211807.801486][T876934] EXT4-fs warning (device sde4): dx_probe:823: inode #3145730: lblock 0: comm zsh: error -5 reading directory block
[1211807.818488][T876934] EXT4-fs warning (device sde4): dx_probe:823: inode #3145731: lblock 0: comm zsh: error -5 reading directory block
[1211807.832078][T873541] EXT4-fs warning (device sde4): dx_probe:823: inode #3145730: lblock 0: comm zsh: error -5 reading directory block
[1211807.849263][T873541] EXT4-fs warning (device sde4): dx_probe:823: inode #3145730: lblock 0: comm zsh: error -5 reading directory block
[1211807.862596][T873541] EXT4-fs warning (device sde4): dx_probe:823: inode #3145731: lblock 0: comm zsh: error -5 reading directory block
[1211807.876017][T876935] EXT4-fs warning (device sde4): dx_probe:823: inode #3145730: lblock 0: comm zsh: error -5 reading directory block
[1211817.457302][ T6547] EXT4-fs warning: 6 callbacks suppressed
[1211817.457307][ T6547] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm crond: error -5 reading directory block
[1211817.478915][ T6547] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm crond: error -5 reading directory block
[1211877.491282][ T6547] EXT4-fs warning (device sde4): dx_probe:823: inode #262145: lblock 0: comm crond: error -5 reading directory blocksync page write
[1211758.873275][ T6527] EXT4-fs (sde4): I/O error while writing superblock
[1211758.873277][ T6527] EXT4-fs (sde4): Remounting filesystem read-only
[1211758.873950][T875111] sde4: writeback error on inode 4240163, offset 0, sector 47427584
[1211758.875144][T62830] JBD2: I/O error when updating journal superblock for sde2-8.
[1211758.876495][T875111] sde4: writeback error on inode 4239928, offset 0, sector 67321856
[1211758.880653][T62830] Buffer I/O error on dev sde3, logical block 1, lost async page write
[1211758.883997][T876738] overlayfs: failed to get metacopy (-5)
[1211758.884079][T876738] overlayfs: failed to get metacopy (-5)
[1211759.105068][T62830] Buffer I/O error on dev sde3, logical block 3, lost async page write
[1211759.113904][T62830] Buffer I/O error on dev sde3, logical block 4, lost async page write
[1211759.122741][T62830] Buffer I/O error on dev sde3, logical block 5, lost async page write
[1211759.131577][T62830] Buffer I/O error on dev sde3, logical block 6, lost async page write
[1211759.140416][T62830] Buffer I/O error on dev sde3, logical block 7, lost async page write
[1211759.149256][T62830] Buffer I/O error on dev sde3, logical block 14, lost async page write
[1211759.988985][T62830] EXT4-fs (sde3): shut down requested (2)
[1211759.995278][T62830] Aborting journal on device sde3-8.
[1211760.005175][T62830] JBD2: I/O error when updating journal superblock for sde3-8.
[1211760.546908][ T7050] EXT4-fs warning (device sde4): dx_probe:823: inode #3154052: lblock 0: comm tuned: error -5 reading directory block
[1211760.564256][ T7050] EXT4-fs warning (device sde4)[   C14] sd 0:0:13:0: [sde] tag#0 CDB: Write(10) 2a 00 15 93 74 00 00 02 00 00
[1211758.803758][   C14] I/O error, dev sde, sector 361985024 op 0x1:(WRITE) flags 0x4800 phys_seg 30 prio class 2
[1211758.814413][   C14] sd 0:0:13:0: [sde] tag#1 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=13s
[1211758.825669][   C14] sd 0:0:13:0: [sde] tag#1 CDB: Write(10) 2a 00 15 93 72 00 00 02 00 00
[1211758.834586][   C14] I/O error, dev sde, sector 361984512 op 0x1:(WRITE) flags 0x4800 phys_seg 32 prio class 2
[1211758.845240][   C14] sd 0:0:13:0: [sde] tag#2 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=13s
[1211758.856495][   C14] sd 0:0:13:0: [sde] tag#2 CDB: Write(10) 2a 00 15 93 70 00 00 02 00 00
[1211758.860885][   C84] sd 0:0:13:0: [sde] tag#12 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=0s
[1211758.861873][T62830] sd 0:0:13:0: SCSI device is removed
[1211758.865419][   C14] I/O error, dev sde, sector 361984000 op 0x1:(WRITE) flags 0x4800 phys_seg 32 prio class 2
[1211758.866484][   C84] sd 0:0:13:0: [sde] tag#12 CDB: Write(10) 2a 08 17 03 80 00 00 00 08 00
[1211758.867053][   C14] sd 0:0:13:0: [sde] tag#3 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=13s
[1211758.867055][T875111] sde4: writeback error on inode 4240163, offset 2097152, sector 47443968
[1211758.867904][   C84] I/O error, dev sde, sector 386105344 op 0x1:(WRITE) flags 0x29800 phys_seg 1 prio class 2
[1211758.868766][   C14] sd 0:0:13:0: [sde] tag#3 CDB: Write(10) 2a 00 15 93 3f f0 00 00 10 00
[1211758.869637][   C84] Buffer I/O error on dev sde4, logical block 8945664, lost sync page write
[1211758.870441][   C14] I/O error, dev sde, sector 361971696 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[1211758.871344][ T1445] JBD2: I/O error when updating journal superblock for sde4-8.
[1211758.872159][T875111] sde4: writeback error on inode 4240163, offset 2088960, sector 47431664
[1211758.872159][   C14] sd 0:0:13:0: [sde] tag#4 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=13s
[1211758.872165][   C14] sd 0:0:13:0: [sde] tag#4 CDB: Write(10) 2a 00 15 93 3e 00 00 01 f0 00
[1211758.872166][   C14] I/O error, dev sde, sector 361971200 op 0x1:(WRITE) flags 0x800 phys_seg 29 prio class 2
[1211758.873161][T62830] EXT4-fs (sde2): shut down requested (2)
[1211758.873167][T62830] Aborting journal on device sde2-8.
[1211758.873184][  T976] Buffer I/O error on dev sde2, logical block 131072, lost sync page write
[1211758.873262][  T811] Buffer I/O error on dev sde4, logical block 0, los]
```

各种内核都换了，最后直接开机都无法识别这个盘了
最后发现，如果使用华为的内核，这个 megaraid 卡就是正常的

之后再测试，发现 /dev/sda 就是有故障，使用 fio 测试，可以很容易的发现
所以，
```txt
[0:0:12:0]   disk    ATA      DSS200-B 240GB   T6.0  /dev/sdc
[0:0:13:0]   disk    ATA      DSS200-B 240GB   T6.0  /dev/sda
[0:0:15:0]   disk    ATA      ST8000NM0055-1RM SNA5  /dev/sdb
[0:0:16:0]   disk    ATA      HGST HUS728T8TAL W9G0  /dev/sde
[0:0:17:0]   disk    ATA      HGST HUS728T8TAL W9G0  /dev/sdd
[0:0:18:0]   disk    ATA      HGST HUS728T8TAL W9G0  /dev/sdf
[0:0:65:0]   enclosu HUAWEI   Expander 12Gx16  131   -
[N:0:0:1]    disk    INTEL SSDPE2KE016T8__1                     /dev/nvme0n1
[N:1:0:1]    disk    INTEL SSDPE2KE016T8__1                     /dev/nvme1n1
```

测试的时候触发的日志为:
```txt
[ 1378.339352] sd 0:0:13:0: [sda] tag#1018 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.347384] sd 0:0:13:0: [sda] tag#1032 BRCM Debug mfi stat 0x2d, data len requested/completed 0x1000/0x0
[ 1378.349692] sd 0:0:13:0: [sda] tag#1039 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.370240] sd 0:0:13:0: [sda] tag#1037 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.380561] sd 0:0:13:0: [sda] tag#1035 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.390880] sd 0:0:13:0: [sda] tag#1031 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.401199] sd 0:0:13:0: [sda] tag#1029 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.411518] sd 0:0:13:0: [sda] tag#1027 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.421836] sd 0:0:13:0: [sda] tag#1025 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.432155] sd 0:0:13:0: [sda] tag#990 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.442388] sd 0:0:13:0: [sda] tag#1087 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.452706] sd 0:0:13:0: [sda] tag#1083 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.463026] sd 0:0:13:0: [sda] tag#1081 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.473344] sd 0:0:13:0: [sda] tag#1079 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.484283] sd 0:0:13:0: [sda] tag#1077 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.495169] sd 0:0:13:0: [sda] tag#1075 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.506052] sd 0:0:13:0: [sda] tag#1073 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.516930] sd 0:0:13:0: [sda] tag#1071 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.527812] sd 0:0:13:0: [sda] tag#1069 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.538694] sd 0:0:13:0: [sda] tag#1067 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.549573] sd 0:0:13:0: [sda] tag#1065 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.560455] sd 0:0:13:0: [sda] tag#1062 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.571334] sd 0:0:13:0: [sda] tag#1060 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.582215] sd 0:0:13:0: [sda] tag#1058 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.593094] sd 0:0:13:0: [sda] tag#1056 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.603974] sd 0:0:13:0: [sda] tag#1053 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.614855] sd 0:0:13:0: [sda] tag#1004 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.625735] sd 0:0:13:0: [sda] tag#1014 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.636613] sd 0:0:13:0: [sda] tag#986 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.647406] sd 0:0:13:0: [sda] tag#1016 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.658284] sd 0:0:13:0: [sda] tag#1013 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.669167] sd 0:0:13:0: [sda] tag#995 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1378.760301] sd 0:0:13:0: [sda] tag#1001 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=7s
[ 1378.771553] sd 0:0:13:0: [sda] tag#1001 CDB: Read(10) 28 00 17 8b 3a c0 00 00 08 00
[ 1378.780459] blk_update_request: I/O error, dev sda, sector 395000512 op 0x0:(READ) flags 0x200000 phys_seg 1 prio class 0
[ 1378.792669] sd 0:0:13:0: [sda] tag#1002 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=7s
[ 1378.803912] sd 0:0:13:0: [sda] tag#1002 CDB: Read(10) 28 00 00 18 0e 00 00 02 00 00
[ 1378.812818] blk_update_request: I/O error, dev sda, sector 1576448 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1378.824851] sd 0:0:13:0: [sda] tag#1003 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=7s
[ 1378.836096] sd 0:0:13:0: [sda] tag#1003 CDB: Read(10) 28 00 00 18 10 00 00 02 00 00
[ 1378.845006] blk_update_request: I/O error, dev sda, sector 1576960 op 0x0:(READ) flags 0x200000 phys_seg 5 prio class 0
[ 1378.857040] sd 0:0:13:0: [sda] tag#1005 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=7s
[ 1378.868287] sd 0:0:13:0: [sda] tag#1005 CDB: Read(10) 28 00 00 18 12 00 00 02 00 00
[ 1378.877197] blk_update_request: I/O error, dev sda, sector 1577472 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1378.889230] sd 0:0:13:0: [sda] tag#1006 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=7s
[ 1378.900478] sd 0:0:13:0: [sda] tag#1006 CDB: Read(10) 28 00 00 18 14 00 00 02 00 00
[ 1378.909390] blk_update_request: I/O error, dev sda, sector 1577984 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1378.921424] sd 0:0:13:0: [sda] tag#1007 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=7s
[ 1378.932677] sd 0:0:13:0: [sda] tag#1007 CDB: Read(10) 28 00 00 18 16 00 00 02 00 00
[ 1378.941586] blk_update_request: I/O error, dev sda, sector 1578496 op 0x0:(READ) flags 0x200000 phys_seg 5 prio class 0
[ 1378.953620] sd 0:0:13:0: [sda] tag#1009 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=7s
[ 1378.964865] sd 0:0:13:0: [sda] tag#1009 CDB: Read(10) 28 00 00 18 18 00 00 02 00 00
[ 1378.973775] blk_update_request: I/O error, dev sda, sector 1579008 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1378.985809] sd 0:0:13:0: [sda] tag#1010 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=7s
[ 1378.997054] sd 0:0:13:0: [sda] tag#1010 CDB: Read(10) 28 00 00 18 1a 00 00 02 00 00
[ 1379.005964] blk_update_request: I/O error, dev sda, sector 1579520 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1379.017997] sd 0:0:13:0: [sda] tag#1011 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=7s
[ 1379.029240] sd 0:0:13:0: [sda] tag#1011 CDB: Read(10) 28 00 00 18 1c 00 00 02 00 00
[ 1379.038148] blk_update_request: I/O error, dev sda, sector 1580032 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1379.054473] sd 0:0:13:0: [sda] tag#1015 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=7s
[ 1379.065724] sd 0:0:13:0: [sda] tag#1015 CDB: Read(10) 28 00 00 18 1e 00 00 02 00 00
[ 1379.074635] blk_update_request: I/O error, dev sda, sector 1580544 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1379.539159] sd 0:0:13:0: SCSI device is removed
[ 1379.549991] sd 0:0:13:0: [sda] Synchronizing SCSI cache
[ 1379.853494] sd 0:0:13:0: [sda] Synchronize Cache(10) failed: Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK
[ 1379.907978] megaraid_sas 0000:87:00.0: scanning for scsi0...
[ 1392.881500] scsi 0:0:13:0: Direct-Access     ATA      DSS200-B 240GB   T6.0 PQ: 0 ANSI: 6
[ 1392.896485] sd 0:0:13:0: Attached scsi generic sg1 type 0
[ 1392.897693] sd 0:0:13:0: [sda] 468862128 512-byte logical blocks: (240 GB/224 GiB)
[ 1392.912966] sd 0:0:13:0: [sda] Write Protect is off
[ 1392.913110] megaraid_sas 0000:87:00.0: scanning for scsi0...
[ 1392.918838] sd 0:0:13:0: [sda] Mode Sense: 9b 00 10 08
[ 1392.926008] sd 0:0:13:0: [sda] Write cache: enabled, read cache: enabled, supports DPO and FUA
[ 1393.015070]  sda: sda1 sda2 sda3 sda4
[ 1393.020019] sd 0:0:13:0: [sda] Attached SCSI disk
[ 1423.307653] sd 0:0:13:0: [sda] tag#1010 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.318675] sd 0:0:13:0: [sda] tag#898 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.329417] sd 0:0:13:0: [sda] tag#971 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.340155] sd 0:0:13:0: [sda] tag#957 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.350894] sd 0:0:13:0: [sda] tag#942 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.361632] sd 0:0:13:0: [sda] tag#1021 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.372462] sd 0:0:13:0: [sda] tag#900 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.383208] sd 0:0:13:0: [sda] tag#968 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.393969] sd 0:0:13:0: [sda] tag#984 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.404735] sd 0:0:13:0: [sda] tag#930 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.415505] sd 0:0:13:0: [sda] tag#911 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.426281] sd 0:0:13:0: [sda] tag#960 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.437064] sd 0:0:13:0: [sda] tag#929 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.447852] sd 0:0:13:0: [sda] tag#970 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.458646] sd 0:0:13:0: [sda] tag#980 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.469455] sd 0:0:13:0: [sda] tag#1012 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.480357] sd 0:0:13:0: [sda] tag#931 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.491171] sd 0:0:13:0: [sda] tag#1003 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.502068] sd 0:0:13:0: [sda] tag#1004 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.512961] sd 0:0:13:0: [sda] tag#904 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.523767] sd 0:0:13:0: [sda] tag#1009 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.534664] sd 0:0:13:0: [sda] tag#912 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.545474] sd 0:0:13:0: [sda] tag#1019 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.556366] sd 0:0:13:0: [sda] tag#961 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.567173] sd 0:0:13:0: [sda] tag#923 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.577979] sd 0:0:13:0: [sda] tag#947 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.588789] sd 0:0:13:0: [sda] tag#897 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.599599] sd 0:0:13:0: [sda] tag#952 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.610407] sd 0:0:13:0: [sda] tag#995 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.621214] sd 0:0:13:0: [sda] tag#955 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.632019] sd 0:0:13:0: [sda] tag#965 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.642824] sd 0:0:13:0: [sda] tag#915 BRCM Debug mfi stat 0x2d, data len requested/completed 0x40000/0x0
[ 1423.797384] scsi_io_completion_action: 61 callbacks suppressed
[ 1423.804199] sd 0:0:13:0: [sda] tag#909 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=5s
[ 1423.815366] sd 0:0:13:0: [sda] tag#909 CDB: Read(10) 28 00 00 00 30 00 00 02 00 00
[ 1423.824192] print_req_error: 246 callbacks suppressed
[ 1423.830215] blk_update_request: I/O error, dev sda, sector 12288 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1423.842096] sd 0:0:13:0: [sda] tag#910 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=5s
[ 1423.853257] sd 0:0:13:0: [sda] tag#910 CDB: Read(10) 28 00 00 00 34 00 00 02 00 00
[ 1423.862079] blk_update_request: I/O error, dev sda, sector 13312 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1423.873946] sd 0:0:13:0: [sda] tag#913 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=5s
[ 1423.885104] sd 0:0:13:0: [sda] tag#913 CDB: Read(10) 28 00 00 00 32 00 00 02 00 00
[ 1423.893929] blk_update_request: I/O error, dev sda, sector 12800 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1423.910070] sd 0:0:13:0: [sda] tag#914 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=5s
[ 1423.921224] sd 0:0:13:0: [sda] tag#914 CDB: Read(10) 28 00 00 00 38 00 00 02 00 00
[ 1423.930039] blk_update_request: I/O error, dev sda, sector 14336 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1423.941903] sd 0:0:13:0: [sda] tag#916 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=5s
[ 1423.953056] sd 0:0:13:0: [sda] tag#916 CDB: Read(10) 28 00 00 00 36 00 00 02 00 00
[ 1423.961878] blk_update_request: I/O error, dev sda, sector 13824 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1423.973738] sd 0:0:13:0: [sda] tag#917 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=6s
[ 1423.984900] sd 0:0:13:0: [sda] tag#917 CDB: Read(10) 28 00 00 00 2e 00 00 02 00 00
[ 1423.993729] blk_update_request: I/O error, dev sda, sector 11776 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1424.009892] sd 0:0:13:0: [sda] tag#918 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=6s
[ 1424.021058] sd 0:0:13:0: [sda] tag#918 CDB: Read(10) 28 00 00 00 6c 00 00 02 00 00
[ 1424.029882] blk_update_request: I/O error, dev sda, sector 27648 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1424.041748] sd 0:0:13:0: [sda] tag#919 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=6s
[ 1424.052916] sd 0:0:13:0: [sda] tag#919 CDB: Read(10) 28 00 00 00 6a 00 00 02 00 00
[ 1424.061747] blk_update_request: I/O error, dev sda, sector 27136 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1424.073612] sd 0:0:13:0: [sda] tag#920 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=6s
[ 1424.084779] sd 0:0:13:0: [sda] tag#920 CDB: Read(10) 28 00 00 00 68 00 00 02 00 00
[ 1424.093607] blk_update_request: I/O error, dev sda, sector 26624 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1424.105475] sd 0:0:13:0: [sda] tag#921 FAILED Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK cmd_age=6s
[ 1424.116642] sd 0:0:13:0: [sda] tag#921 CDB: Read(10) 28 00 00 00 66 00 00 02 00 00
[ 1424.125472] blk_update_request: I/O error, dev sda, sector 26112 op 0x0:(READ) flags 0x200000 phys_seg 4 prio class 0
[ 1424.553449] sd 0:0:13:0: SCSI device is removed
[ 1424.564259] sd 0:0:13:0: [sda] Synchronizing SCSI cache
[ 1424.735569] sd 0:0:13:0: [sda] Synchronize Cache(10) failed: Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK
[ 1424.807496] megaraid_sas 0000:87:00.0: scanning for scsi0...
[ 1441.905777] scsi 0:0:13:0: Direct-Access     ATA      DSS200-B 240GB   T6.0 PQ: 0 ANSI: 6
[ 1441.920848] sd 0:0:13:0: Attached scsi generic sg1 type 0
[ 1441.922027] sd 0:0:13:0: [sda] 468862128 512-byte logical blocks: (240 GB/224 GiB)
[ 1441.937436] sd 0:0:13:0: [sda] Write Protect is off
[ 1441.937593] megaraid_sas 0000:87:00.0: scanning for scsi0...
[ 1441.943307] sd 0:0:13:0: [sda] Mode Sense: 9b 00 10 08
[ 1441.950490] sd 0:0:13:0: [sda] Write cache: enabled, read cache: enabled, supports DPO and FUA
[ 1442.042512]  sda: sda1 sda2 sda3 sda4
[ 1442.047413] sd 0:0:13:0: [sda] Attached SCSI disk
```

这终于是一个完整的，知道一个盘坏掉的过程了。

## 开机之后，发现这个问题
```txt
[   44.680704] scsi host0: waking up host to restart
[   44.685407] scsi host0: host state transition : recovery->running
[   44.691587] scsi host0: scsi_eh_0: sleeping
[   44.933443] scsi host0: host state transition : running->recovery
[   44.955975] scsi host0: Waking error handler thread
[   44.955985] scsi host0: scsi_eh_0: waking up 0/1/1
[   44.965667] scsi host0: scsi_eh_prt_fail_stats: cmds failed: 1, cancel: 0
[   44.972450] scsi host0: Total of 1 commands on 1 devices require eh work
[   44.979146] sd 0:0:6:0: [sda] tag#741 scsi_eh_0: requesting sense
[   44.985687] sd 0:0:6:0: [sda] tag#741 scsi_eh_done result: 0
[   44.991351] sd 0:0:6:0: [sda] tag#741 scsi_send_eh_cmnd timeleft: 9994
[   44.997894] sd 0:0:6:0: [sda] tag#741 scsi_send_eh_cmnd: scsi_eh_completed_normally 2002
[   45.005984] sd 0:0:6:0: [sda] tag#741 sense requested, result 8000002
[   45.012419] sd 0:0:6:0: [sda] tag#741 Sense Key : No Sense [current]
[   45.018864] sd 0:0:6:0: [sda] tag#741 Add. Sense: No additional sense information
[   45.026345] sd 0:0:6:0: [sda] tag#741 scsi_eh_0: flush finish cmd
[   45.032442] scsi host0: waking up host to restart
[   45.037144] scsi host0: host state transition : recovery->running
[   45.043413] scsi host0: scsi_eh_0: sleeping
```

```txt
[root@vhost1731 12:08:25 device]$ cat ioeh_cnt
0x3
[root@vhost1731 12:08:30 device]$ cat ioerr_cnt
0x7
[root@vhost1731 12:08:33 device]$ cat iotiming_out_cnt
0x0
[root@vhost1731 12:08:40 device]$ cat iotmo_cnt
0x0
[root@vhost1731 12:08:42 device]$ cat ioabrt_cnt
0x0
```


```txt
[root@vhost1731 12:08:45 device]$ ls -la /sys/block/
total 0
drwxr-xr-x  2 root root 0 Jun 11 12:04 .
dr-xr-xr-x 13 root root 0 Jun 11 12:04 ..
lrwxrwxrwx  1 root root 0 Jun 11 12:04 loop0 -> ../devices/virtual/block/loop0
lrwxrwxrwx  1 root root 0 Jun 11 12:04 md126 -> ../devices/virtual/block/md126
lrwxrwxrwx  1 root root 0 Jun 11 12:04 md127 -> ../devices/virtual/block/md127
lrwxrwxrwx  1 root root 0 Jun 11 12:04 sda -> ../devices/pci0000:00/0000:00:01.0/0000:01:00.0/host0/target0:0:6/0:0:6:0/block/sda
lrwxrwxrwx  1 root root 0 Jun 11 12:04 sdb -> ../devices/pci0000:00/0000:00:01.0/0000:01:00.0/host0/target0:0:7/0:0:7:0/block/sdb
lrwxrwxrwx  1 root root 0 Jun 11 12:04 sdc -> ../devices/pci0000:00/0000:00:01.0/0000:01:00.0/host0/target0:0:8/0:0:8:0/block/sdc
lrwxrwxrwx  1 root root 0 Jun 11 12:04 sdd -> ../devices/pci0000:00/0000:00:01.0/0000:01:00.0/host0/target0:0:9/0:0:9:0/block/sdd
lrwxrwxrwx  1 root root 0 Jun 11 12:04 sde -> ../devices/pci0000:00/0000:00:01.0/0000:01:00.0/host0/target0:0:10/0:0:10:0/block/sde
lrwxrwxrwx  1 root root 0 Jun 11 12:04 sdf -> ../devices/pci0000:00/0000:00:01.0/0000:01:00.0/host0/target0:0:11/0:0:11:0/block/sdf
lrwxrwxrwx  1 root root 0 Jun 11 12:04 sdg -> ../devices/pci0000:00/0000:00:11.4/ata4/host4/target4:0:0/4:0:0:0/block/sdg
```

果然，这是 sata 特殊问题。

应该是，scsi_complete 中:
```c
	switch (disposition) {
	case SUCCESS:
		scsi_finish_command(cmd);
		break;
	case NEEDS_RETRY:
		scsi_queue_insert(cmd, SCSI_MLQUEUE_EH_RETRY);
		break;
	case ADD_TO_MLQUEUE:
		scsi_queue_insert(cmd, SCSI_MLQUEUE_DEVICE_BUSY);
		break;
	default:
		scsi_eh_scmd_add(cmd);
		break;
	}
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
