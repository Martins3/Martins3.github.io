# sfc
## 记得 sfc 有两个常用工具
```txt
[root@test01 22:01:52 ]$ sfboot -i p2p2_0 --repair^C
[root@test01 22:02:19 ]$ sfupdate --write
Solarflare firmware update utility [v8.2.4]
Copyright 2002-2020 Xilinx, Inc.
Loading firmware images from /usr/share/sfutils/sfupdate_images
ens2f0np0: updating Bundle firmware from 8.0.1.1002 to 8.5.2.1000
ens2f0np0: writing Bundle firmware
[100%] Erasing
[100%] Writing
[  0%] Validating
```

似乎只有这样，才可以配置 sriov ，不然 sriov 在 sysfs 下，连目录都没有:
```txt
sfboot --adapter=ens2f1np1 switch-mode=sriov pf-count=1 vf-count=120
```

## 基本的调试方法
echo "0000:00:0a.0" | sudo tee /sys/bus/pci/drivers/sfc/bind
echo "module sfc +p" > /sys/kernel/debug/dynamic_debug/control

## 固件升级有趣的问题
```txt
[test01 16:59:18 ~]$ dmesg | grep sfc
[   25.845297] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): Solarflare NIC detected
[   26.231198] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): Part Number : X2522-25G
[   26.281102] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): MCPU watchdog reset at PC = 0x00051edc in thread 0x00111708
[   26.281105] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R01 (?): 0x00000000
[   26.281106] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R02 (?): 0x0014bec8
[   26.281107] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R03 (?): 0x00000015
[   26.281108] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R04 (?): 0x00000001
[   26.281108] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R05 (?): 0x00000040
[   26.281109] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R06 (?): 0x00000000
[   26.281110] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R07 (?): 0x00000078
[   26.281111] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R08 (?): 0x00000081
[   26.281111] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R09 (?): 0x00111c42
[   26.281112] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R10 (?): 0x00000001
[   26.281113] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R11 (?): 0xfffffffe
[   26.281113] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R12 (?): 0x00000001
[   26.281114] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R13 (?): 0x0014bec8
[   26.281115] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R14 (?): 0x00000000
[   26.281115] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R15 (?): 0x47b5481d
[   26.281116] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R16 (?): 0x00000078
[   26.281117] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R17 (?): 0x00000002
[   26.281117] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R18 (?): 0x00000003
[   26.281118] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R19 (?): 0x0000ffff
[   26.281119] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R20 (?): 0x00000001
[   26.281119] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R21 (?): 0x0010fb1c
[   26.281120] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R22 (?): 0x00000080
[   26.281121] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R23 (?): 0xffff0001
[   26.281121] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R24 (?): 0xe4bc4715
[   26.281122] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R25 (?): 0x00013e34
[   26.281123] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R26 (?): 0x00000000
[   26.281123] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R27 (?): 0x00010580
[   26.281124] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R28 (?): 0x001124ac
[   26.281125] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R29 (?): 0x00169268
[   26.281125] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R30 (?): 0x00000001
[   26.281126] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R31 (?): 0x00051d28
[   26.290251] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): Insufficient resources for 32 XDP event queues (33 other channels, max 32)
[   26.290253] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): WARNING: Insufficient MSI-X vectors available (32 < 33).
[   26.290253] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): WARNING: Performance may be reduced.
[   26.290705] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): ERROR: PTP requires MSI-X and 1 additional interruptvector. PTP disabled
[   27.088020] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): MC rebooted
[   27.098908] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): MC reboot detected
[   27.483940] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): MC command 0x8b inlen 8 failed rc=-5 (raw=0) arg=0
[   27.587278] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): Unable to set UDP tunnel ports; rc=-22.
[   27.587280] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): failed to create NIC
[   27.599387] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): MCPU watchdog reset at PC = 0x00051df4 in thread 0x00111708
[   27.617621] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R01 (?): 0x00000000
[   27.629907] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R02 (?): 0x0014bec8
[   27.642204] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R03 (?): 0x00000000
[   27.654477] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R04 (?): 0x00000001
[   27.666771] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R05 (?): 0x00000000
[   27.679036] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R06 (?): 0x00000016
[   27.691334] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R07 (?): 0x0000000b
[   27.703610] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R08 (?): 0x00000080
[   27.715893] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R09 (?): 0x00111c3c
[   27.728195] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R10 (?): 0x00000000
[   27.740480] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R11 (?): 0xfffffffe
[   27.752774] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R12 (?): 0x00000001
[   27.765107] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R13 (?): 0x0014bec8
[   27.777442] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R14 (?): 0x00000000
[   27.789768] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R15 (?): 0x00158f50
[   27.802126] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R16 (?): 0xffffff50
[   27.814490] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R17 (?): 0x00000002
[   27.826831] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R18 (?): 0x00000004
[   27.839191] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R19 (?): 0x0000ffff
[   27.851549] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R20 (?): 0x00000000
[   27.863908] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R21 (?): 0x0010fb1c
[   27.876263] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R22 (?): 0x00000080
[   27.888648] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R23 (?): 0xffff0000
[   27.901033] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R24 (?): 0x00000048
[   27.913408] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R25 (?): 0x00010cec
[   27.925741] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R26 (?): 0x00000000
[   27.937913] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R27 (?): 0x00010580
[   27.949961] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R28 (?): 0x001124ac
[   27.961830] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R29 (?): 0x00169268
[   27.973607] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R30 (?): 0x00000000
[   27.985313] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R31 (?): 0x00051d50
[   28.006366] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): Insufficient resources for 32 XDP event queues (33 other channels, max 32)
[   28.025161] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): WARNING: Insufficient MSI-X vectors available (32 < 33).
[   28.042780] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): WARNING: Performance may be reduced.
[   28.056955] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): ERROR: PTP requires MSI-X and 1 additional interruptvector. PTP disabled
[   28.800198] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): MC rebooted
[   28.811848] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): MC reboot detected
[   29.082211] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): MC command 0x8b inlen 8 failed rc=-5 (raw=0) arg=0
[   29.800139] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): Unable to set UDP tunnel ports; rc=-22.
[   29.800143] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): failed to create NIC
[   30.034238] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): MCPU watchdog reset at PC = 0x00051e20 in thread 0x00111708
[   30.053213] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R01 (?): 0x00000000
[   30.065684] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R02 (?): 0x0014bec8
[   30.078078] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R03 (?): 0x00000000
[   30.090439] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R04 (?): 0x00000001
[   30.102814] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R05 (?): 0x00000000
[   30.115195] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R06 (?): 0x00000018
[   30.127565] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R07 (?): 0x0000000c
[   30.139936] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R08 (?): 0x00000080
[   30.152324] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R09 (?): 0x00111c3c
[   30.164698] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R10 (?): 0x00000000
[   30.177092] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R11 (?): 0xfffffffe
[   30.189481] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R12 (?): 0x00000000
[   30.201876] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R13 (?): 0x0014bec8
[   30.214268] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R14 (?): 0x00000000
[   30.226647] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R15 (?): 0x47b5481d
[   30.239019] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R16 (?): 0xffffff50
[   30.251403] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R17 (?): 0x00000002
[   30.263806] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R18 (?): 0x00000004
[   30.276191] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R19 (?): 0x0000ffff
[   30.288576] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R20 (?): 0x00000000
[   30.300953] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R21 (?): 0x0010fb1c
[   30.313355] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R22 (?): 0x00000080
[   30.325739] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R23 (?): 0xffff0000
[   30.338116] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R24 (?): 0xe4bc4715
[   30.350494] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R25 (?): 0x0006d108
[   30.362883] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R26 (?): 0x00000000
[   30.375275] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R27 (?): 0x00010580
[   30.387668] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R28 (?): 0x001124ac
[   30.400064] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R29 (?): 0x00169268
[   30.412458] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R30 (?): 0x00000000
[   30.424863] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): R31 (?): 0x00051d50
[   30.446685] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): Insufficient resources for 32 XDP event queues (33 other channels, max 32)
[   30.467186] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): WARNING: Insufficient MSI-X vectors available (32 < 33).
[   30.485990] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): WARNING: Performance may be reduced.
[   30.500556] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): ERROR: PTP requires MSI-X and 1 additional interruptvector. PTP disabled
[   31.244194] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): MC rebooted
[   31.255899] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): MC reboot detected
[   31.522203] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): MC command 0x8b inlen 8 failed rc=-5 (raw=0) arg=0
[   31.776774] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): Unable to set UDP tunnel ports; rc=-22.
[   31.776776] sfc 0000:38:00.0 (unnamed net_device) (uninitialized): failed to create NIC
[   31.789529] sfc: probe of 0000:38:00.0 failed with error -5
[   31.789651] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): Solarflare NIC detected
[   31.868199] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): Part Number : X2522-25G
[   31.868378] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): MCPU watchdog reset at PC = 0x00051df4 in thread 0x00111708
[   31.887304] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R01 (?): 0x00000000
[   31.899761] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R02 (?): 0x0014bec8
[   31.912164] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R03 (?): 0x00000000
[   31.924435] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R04 (?): 0x00000001
[   31.936604] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R05 (?): 0x00000000
[   31.948748] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R06 (?): 0x00000016
[   31.960900] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R07 (?): 0x0000000b
[   31.973031] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R08 (?): 0x00000080
[   31.985163] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R09 (?): 0x00111c3c
[   31.997308] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R10 (?): 0x00000000
[   32.009459] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R11 (?): 0xfffffffe
[   32.021644] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R12 (?): 0x00000001
[   32.033814] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R13 (?): 0x0014bec8
[   32.045996] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R14 (?): 0x00000000
[   32.058180] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R15 (?): 0x47b5481d
[   32.070349] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R16 (?): 0xffffff50
[   32.082530] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R17 (?): 0x00000002
[   32.094708] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R18 (?): 0x00000004
[   32.106846] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R19 (?): 0x0000ffff
[   32.118998] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R20 (?): 0x00000000
[   32.131163] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R21 (?): 0x0010fb1c
[   32.143329] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R22 (?): 0x00000080
[   32.155471] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R23 (?): 0xffff0000
[   32.167634] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R24 (?): 0xe4bc4715
[   32.179792] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R25 (?): 0x0006d108
[   32.191979] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R26 (?): 0x00000000
[   32.204151] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R27 (?): 0x00010580
[   32.216309] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R28 (?): 0x001124ac
[   32.228477] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R29 (?): 0x00169268
[   32.240653] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R30 (?): 0x00000000
[   32.252835] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R31 (?): 0x00051d50
[   32.274289] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): Insufficient resources for 32 XDP event queues (33 other channels, max 32)
[   32.294412] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): WARNING: Insufficient MSI-X vectors available (32 < 33).
[   32.313276] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): WARNING: Performance may be reduced.
[   32.327845] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): ERROR: PTP requires MSI-X and 1 additional interruptvector. PTP disabled
[   33.071194] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): MC rebooted
[   33.082862] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): MC reboot detected
[   33.346203] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): MC command 0x8b inlen 8 failed rc=-5 (raw=0) arg=0
[   33.632133] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): Unable to set UDP tunnel ports; rc=-22.
[   33.632135] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): failed to create NIC
[   33.644548] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): MCPU watchdog reset at PC = 0x00051e10 in thread 0x00111708
[   33.663330] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R01 (?): 0x00000000
[   33.675797] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R02 (?): 0x0014bec8
[   33.688221] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R03 (?): 0x00000001
[   33.700489] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R04 (?): 0x00000001
[   33.712656] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R05 (?): 0x00000000
[   33.724826] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R06 (?): 0x00000015
[   33.736977] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R07 (?): 0x0000000a
[   33.749140] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R08 (?): 0x00000081
[   33.761274] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R09 (?): 0x00111c42
[   33.773432] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R10 (?): 0x00000001
[   33.785593] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R11 (?): 0xfffffffe
[   33.797752] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R12 (?): 0x00000000
[   33.809941] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R13 (?): 0x0014bec8
[   33.822090] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R14 (?): 0x00000000
[   33.834260] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R15 (?): 0x47b5481d
[   33.846436] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R16 (?): 0xffffff50
[   33.858622] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R17 (?): 0x00000002
[   33.870797] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R18 (?): 0x00000004
[   33.882945] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R19 (?): 0x0000ffff
[   33.895094] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R20 (?): 0x00000001
[   33.907284] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R21 (?): 0x0010fb1c
[   33.919460] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R22 (?): 0x00000080
[   33.931635] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R23 (?): 0xffff0001
[   33.943810] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R24 (?): 0xe4bc4715
[   33.955974] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R25 (?): 0x0006d108
[   33.968148] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R26 (?): 0x00000000
[   33.980321] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R27 (?): 0x00010580
[   33.992478] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R28 (?): 0x001124ac
[   34.004638] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R29 (?): 0x00169268
[   34.016795] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R30 (?): 0x00000000
[   34.028957] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R31 (?): 0x00051d50
[   34.050540] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): Insufficient resources for 32 XDP event queues (33 other channels, max 32)
[   34.070655] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): WARNING: Insufficient MSI-X vectors available (32 < 33).
[   34.089498] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): WARNING: Performance may be reduced.
[   34.104056] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): ERROR: PTP requires MSI-X and 1 additional interruptvector. PTP disabled
[   34.847194] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): MC rebooted
[   34.858829] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): MC reboot detected
[   35.122203] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): MC command 0x8b inlen 8 failed rc=-5 (raw=0) arg=0
[   35.804277] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): Unable to set UDP tunnel ports; rc=-22.
[   35.804279] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): failed to create NIC
[   36.122241] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): MCPU watchdog reset at PC = 0x00051e10 in thread 0x00111708
[   36.141015] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R01 (?): 0x00000000
[   36.153476] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R02 (?): 0x0014bec8
[   36.165903] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R03 (?): 0x00000001
[   36.178183] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R04 (?): 0x00000001
[   36.190347] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R05 (?): 0x00000000
[   36.202486] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R06 (?): 0x00000015
[   36.214640] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R07 (?): 0x0000000a
[   36.226794] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R08 (?): 0x00000081
[   36.238946] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R09 (?): 0x00111c42
[   36.251088] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R10 (?): 0x00000001
[   36.263258] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R11 (?): 0xfffffffe
[   36.275433] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R12 (?): 0x00000000
[   36.287621] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R13 (?): 0x0014bec8
[   36.299784] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R14 (?): 0x00000000
[   36.311958] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R15 (?): 0x47b5481d
[   36.324120] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R16 (?): 0xffffff50
[   36.336291] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R17 (?): 0x00000002
[   36.348456] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R18 (?): 0x00000004
[   36.360630] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R19 (?): 0x0000ffff
[   36.372805] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R20 (?): 0x00000001
[   36.384976] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R21 (?): 0x0010fb1c
[   36.397150] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R22 (?): 0x00000080
[   36.409325] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R23 (?): 0xffff0001
[   36.421494] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R24 (?): 0xe4bc4715
[   36.433676] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R25 (?): 0x0006d108
[   36.445842] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R26 (?): 0x00000000
[   36.458029] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R27 (?): 0x00010580
[   36.470184] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R28 (?): 0x001124ac
[   36.482341] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R29 (?): 0x00169268
[   36.494511] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R30 (?): 0x00000000
[   36.506676] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): R31 (?): 0x00051d50
[   36.528105] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): Insufficient resources for 32 XDP event queues (33 other channels, max 32)
[   36.548219] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): WARNING: Insufficient MSI-X vectors available (32 < 33).
[   36.567069] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): WARNING: Performance may be reduced.
[   36.581610] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): ERROR: PTP requires MSI-X and 1 additional interruptvector. PTP disabled
[   37.325195] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): MC rebooted
[   37.336862] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): MC reboot detected
[   37.602201] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): MC command 0x8b inlen 8 failed rc=-5 (raw=0) arg=0
[   37.832814] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): Unable to set UDP tunnel ports; rc=-22.
[   37.832816] sfc 0000:38:00.1 (unnamed net_device) (uninitialized): failed to create NIC
[   37.845328] sfc: probe of 0000:38:00.1 failed with error -5
```

```txt
[root@test01 08:55:33 ]$ lsinitrd | grep sfc
drwxr-xr-x   2 root     root            0 Jun 13  2024 usr/lib/modules/martins3-5.10.x86_64/kernel/drivers/net/ethernet/sfc
-rw-r--r--   1 root     root       171600 Jun 13  2024 usr/lib/modules/martins3-5.10.x86_64/kernel/drivers/net/ethernet/sfc/sfc.ko.xz
-rw-r--r--   1 root     root        28792 Jun 13  2024 usr/lib/modules/martins3-5.10.x86_64/updates/sfc_driverlink.ko
-rw-r--r--   1 root     root      1627656 Jun 13  2024 usr/lib/modules/martins3-5.10.x86_64/updates/sfc.ko
[root@test01 08:55:45 ]$ cat ^C
[root@test01 08:55:47 ]$ ls -la /sys/iommu^C
[root@test01 08:55:58 ]$ ls -la /sys/kernel/debug/sfc/
cards/         if_ens2f1np1/  if_p2p2_1/     if_p2p2_3/     if_p2p2_5/     nic_ens2f0np0/ nic_p2p2_0/    nic_p2p2_2/    nic_p2p2_4/    nic_p2p2_6/
if_ens2f0np0/  if_p2p2_0/     if_p2p2_2/     if_p2p2_4/     if_p2p2_6/     nic_ens2f1np1/ nic_p2p2_1/    nic_p2p2_3/    nic_p2p2_5/
[root@test01 08:55:58 ]$ ls -la /sys/kernel/debug/sfc/
cards/         if_ens2f1np1/  if_p2p2_1/     if_p2p2_3/     if_p2p2_5/     nic_ens2f0np0/ nic_p2p2_0/    nic_p2p2_2/    nic_p2p2_4/    nic_p2p2_6/
if_ens2f0np0/  if_p2p2_0/     if_p2p2_2/     if_p2p2_4/     if_p2p2_6/     nic_ens2f1np1/ nic_p2p2_1/    nic_p2p2_3/    nic_p2p2_5/
[root@test01 08:55:58 ]$ ls -la /sys/kernel/debug/sfc/
cards/         if_ens2f1np1/  if_p2p2_1/     if_p2p2_3/     if_p2p2_5/     nic_ens2f0np0/ nic_p2p2_0/    nic_p2p2_2/    nic_p2p2_4/    nic_p2p2_6/
if_ens2f0np0/  if_p2p2_0/     if_p2p2_2/     if_p2p2_4/     if_p2p2_6/     nic_ens2f1np1/ nic_p2p2_1/    nic_p2p2_3/    nic_p2p2_5/
[root@test01 08:55:58 ]$ ls -la /sys/kernel/debug/sfc/
total 0
drwxr-xr-x  3 root root 0 Dec 30 08:50 .
drwx------ 40 root root 0 Dec 30 08:49 ..
drwxr-xr-x 11 root root 0 Dec 30 08:49 cards
lrwxrwxrwx  1 root root 0 Dec 30 08:49 if_ens2f0np0 -> cards/0000:38:00.0/port0
lrwxrwxrwx  1 root root 0 Dec 30 08:49 if_ens2f1np1 -> cards/0000:38:00.1/port0
lrwxrwxrwx  1 root root 0 Dec 30 08:50 if_p2p2_0 -> cards/0000:38:0f.2/port0
lrwxrwxrwx  1 root root 0 Dec 30 08:50 if_p2p2_1 -> cards/0000:38:0f.3/port0
lrwxrwxrwx  1 root root 0 Dec 30 08:50 if_p2p2_2 -> cards/0000:38:0f.4/port0
lrwxrwxrwx  1 root root 0 Dec 30 08:50 if_p2p2_3 -> cards/0000:38:0f.5/port0
lrwxrwxrwx  1 root root 0 Dec 30 08:50 if_p2p2_4 -> cards/0000:38:0f.6/port0
lrwxrwxrwx  1 root root 0 Dec 30 08:50 if_p2p2_5 -> cards/0000:38:0f.7/port0
lrwxrwxrwx  1 root root 0 Dec 30 08:50 if_p2p2_6 -> cards/0000:38:10.0/port0
lrwxrwxrwx  1 root root 0 Dec 30 08:49 nic_ens2f0np0 -> cards/0000:38:00.0
lrwxrwxrwx  1 root root 0 Dec 30 08:49 nic_ens2f1np1 -> cards/0000:38:00.1
lrwxrwxrwx  1 root root 0 Dec 30 08:50 nic_p2p2_0 -> cards/0000:38:0f.2
lrwxrwxrwx  1 root root 0 Dec 30 08:50 nic_p2p2_1 -> cards/0000:38:0f.3
lrwxrwxrwx  1 root root 0 Dec 30 08:50 nic_p2p2_2 -> cards/0000:38:0f.4
lrwxrwxrwx  1 root root 0 Dec 30 08:50 nic_p2p2_3 -> cards/0000:38:0f.5
lrwxrwxrwx  1 root root 0 Dec 30 08:50 nic_p2p2_4 -> cards/0000:38:0f.6
lrwxrwxrwx  1 root root 0 Dec 30 08:50 nic_p2p2_5 -> cards/0000:38:0f.7
lrwxrwxrwx  1 root root 0 Dec 30 08:50 nic_p2p2_6 -> cards/0000:38:10.0
```

错误的关键在于:
```txt
  b'efx_mcdi_alloc_vis'
  b'efx_ef10_dimension_resources'
  b'efx_probe_nic'
  b'efx_probe_all'
  b'efx_pci_probe_main'
  b'efx_pci_probe_post_io'
  b'efx_pci_probe'
  b'local_pci_probe'
  b'pci_call_probe'
  b'pci_device_probe'
  b'really_probe'
  b'driver_probe_device'
  b'device_driver_attach'
  b'bind_store'
  b'kernfs_fop_write_iter'
  b'new_sync_write'
  b'vfs_write'
  b'ksys_write'
  b'do_syscall_64'
  b'entry_SYSCALL_64_after_hwframe'
  b'[unknown]'
  b'[unknown]'
```

主线内核 sfc 驱动和 onload sfc 驱动有一个关键区别，

主线内核是在探测驱动的时候调用到 efx_mcdi_alloc_vis ，
而 onload 中是 ip link set dev ens10 up 的时候调用 efx_mcdi_alloc_vis

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
