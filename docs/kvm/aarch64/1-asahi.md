# hacking asahi linux

## [ ] 内核默认不支持 ftrace ?

好吧，ftrace 的功能需要仔细分析下

## perf 功能无法正常支持
```txt
🤒  sudo perf record -a  -- sleep 10
Error:
cycles:P: PMU Hardware doesn't support sampling/overflow-interrupts. Try 'perf stat'
```

执行 perf stat 的结果为:
```txt
🧀  sudo perf stat
 Performance counter stats for 'system wide':

      3,341,600.92 msec cpu-clock                        #    8.000 CPUs utilized
           437,563      context-switches                 #  130.944 /sec
               585      cpu-migrations                   #    0.175 /sec
               879      page-faults                      #    0.263 /sec
    11,541,113,133      cycles                           #    0.003 GHz
    22,766,787,829      instructions                     #    1.97  insn per cycle
   <not supported>      branches
   <not supported>      branch-misses

     417.690667805 seconds time elapsed
```

## 好家伙，中断控制器都不是 GIC

```txt
🧀  cat /proc/interrupts
           CPU0       CPU1       CPU2       CPU3       CPU4       CPU5       CPU6       CPU7
 33:          0          0          0          0          0          0          0          0       KVM   0 Level     kvm guest ptimer
 34:          0          0          0          0          0          0          0          0       KVM   1 Level     kvm guest vtimer
 35:    7755597    9655642    8566153    7032013   14442467   14024790   24046855   22478076   AIC-FIQ   0 Level     arch_timer
 37:          0          0          0          0          0          0          0          0      AIC2 66248 Level     206408000.mbox-recv
 38:          0          0          0          0          0          0          0          0      AIC2 66245 Level     206408000.mbox-send
 39:          0          0          0          0          0          0          0          0      AIC2 66074 Level     231c08000.mbox-recv
 40:          0          0          0          0          0          0          0          0      AIC2 66071 Level     231c08000.mbox-send
 41:       3111       1683        954        813      62373      15332       3660       2035      AIC2 66038 Level     23e408000.mbox-recv
 42:          0          0          0          0          0          0          0          0      AIC2 66035 Level     23e408000.mbox-send
 43:         14          3          2          4          1          0          2          3      AIC2 66403 Level     24e408000.mbox-recv
 44:          0          0          0          0          0          0          0          0      AIC2 66400 Level     24e408000.mbox-send
 45:          0          1          0          0          3          2         14         12      AIC2 66256 Level     277408000.mbox-recv
 46:          0          0          0          0          0          0          0          0      AIC2 66253 Level     277408000.mbox-send
 49:          0          0          0          0          0          0          0          0   AIC-FIQ   4 Level     arm-pmu
 50:          0          0          0          0          0          0          0          0   AIC-FIQ   5 Level     arm-pmu
 51:     157167      63352      32229      18788      54949      26252      25984      21969      AIC2 66260 Level     nvme-apple
 52:          0          0          0          0          0          0          0          0      AIC2 66089 Level     apple-dart fault handler, apple-dart fault handler
 53:          0          0          0          0          0          0          0          0      AIC2 66305 Level     apple-dart fault handler
 54:          0          0          0          0          0          0          0          0      AIC2 66384 Level     apple-dart fault handler
 55:          0          0          0          0          0          0          0          0      AIC2 66571 Level     apple-dart fault handler, apple-dart fault handler
 56:          0          0          0          0          0          0          0          0      AIC2 66652 Level     apple-dart fault handler, apple-dart fault handler
 57:          0          0          0          0          0          0          0          0      AIC2 66318 Level     apple-dart fault handler
 73:          0          0          0          0          0          0          0          0      AIC2 66301 Level     pasemi_apple_i2c
 74:          0          0          0          0          0          0          0          0      AIC2 66285 Level     235104000.spi
 75:         32          8         10          7         21         11         11         22      AIC2 66297 Level     pasemi_apple_i2c
 76:          0          0          0          0          0          0          0          0      AIC2 66298 Level     pasemi_apple_i2c
 77:          0          0          0          0          0          0          0          0      AIC2 66299 Level     pasemi_apple_i2c
 78:         65         31          8          4          4          6          4          6      AIC2 66300 Level     pasemi_apple_i2c
 93:          6          1          1          2          4          2          1          1  Apple-GPIO   8 Level     1-0038, 1-003f
 94:          0          0          0          0          0          0          0          0  dockchannel-irqc   2 Edge      apple-dockchannel-tx
 95:       2912        890        785        822       6962       2533       3463        412  dockchannel-irqc   3 Edge      apple-dockchannel-rx
 97:          0          0          0          0          0          1          0          0      PCIe  12 Edge      Link up
 98:          0          0          0          0          0          0          0          0      PCIe  14 Edge      Link down
100:          0          0          0          0          0          0          0          0  PCIe MSI   0 Edge      PCIe PME, aerdrv
101:          0          0          0          0          0          0          0          0      AIC2 66296 Level     238200000.dma-controller
102:          0          1          0          0          0          0          0          1  Apple-GPIO 149 Level     cs42l84
104:         39          7         52          6         25         18         43         28  PCIe MSI 526336 Edge      bcm4377
105:      69224      34563      14650      11604     373757     124035      78794      60963  PCIe MSI 524288 Edge      brcmf_pcie_intr
IPI0:    106088     116924     126718     117934     404619     382391     310192     242062       Rescheduling interrupts
IPI1:   6531937    5465585    4908987    4327894    3303307    3866256    3334291    2757015       Function call interrupts
IPI2:         0          0          0          0          0          0          0          0       CPU stop interrupts
IPI3:         0          0          0          0          0          0          0          0       CPU stop (for crash dump) interrupts
IPI4:         0          0          0          0          0          0          0          0       Timer broadcast interrupts
IPI5:         0          0          0          0          0          0          0          0       IRQ work interrupts
IPI6:         0          0          0          0          0          0          0          0       CPU wake-up interrupts
Err:          0
```
这里有几个中断都有点不认识。

## 如何才可以安装上虚拟机

https://daynix.github.io/2023/06/03/developing-qemu-on-asahi-linux-linux-port-for-apple-silicon.html

## 查看电池的剩余量
```sh
cat /sys/class/power_supply/macsmc-battery/capacity
```

## 似乎 GPU 驱动需要额外的安装
-  https://asahilinux.org/2022/12/gpu-drivers-now-in-asahi-linux/

## 尝试下启动虚拟机吧
- http://cdn.kernel.org/pub/linux/kernel/people/will/docs/qemu/qemu-arm64-howto.html
- https://futurewei-cloud.github.io/ARM-Datacenter/qemu/how-to-launch-aarch64-vm/

## 这个的时间是错误的
```txt
[Sat May 25 22:26:28 2024] xhci-hcd xhci-hcd.0.auto: xHCI Host Controller
[Sat May 25 22:26:28 2024] xhci-hcd xhci-hcd.0.auto: new USB bus registered, assigned bus number 1
[Sat May 25 22:26:28 2024] xhci-hcd xhci-hcd.0.auto: hcc params 0x0238ffcd hci version 0x110 quirks 0x0000008000000010
[Sat May 25 22:26:28 2024] xhci-hcd xhci-hcd.0.auto: irq 118, io mem 0x502280000
[Sat May 25 22:26:28 2024] xhci-hcd xhci-hcd.0.auto: xHCI Host Controller
[Sat May 25 22:26:28 2024] xhci-hcd xhci-hcd.0.auto: new USB bus registered, assigned bus number 2
[Sat May 25 22:26:28 2024] xhci-hcd xhci-hcd.0.auto: Host supports USB 3.1 Enhanced SuperSpeed
[Sat May 25 22:26:28 2024] usb usb1: New USB device found, idVendor=1d6b, idProduct=0002, bcdDevice= 6.08
[Sat May 25 22:26:28 2024] usb usb1: New USB device strings: Mfr=3, Product=2, SerialNumber=1
[Sat May 25 22:26:28 2024] usb usb1: Product: xHCI Host Controller
[Sat May 25 22:26:28 2024] usb usb1: Manufacturer: Linux 6.8.9-405.asahi.fc40.aarch64+16k xhci-hcd
[Sat May 25 22:26:28 2024] usb usb1: SerialNumber: xhci-hcd.0.auto
[Sat May 25 22:26:28 2024] hub 1-0:1.0: USB hub found
[Sat May 25 22:26:28 2024] hub 1-0:1.0: 1 port detected
[Sat May 25 22:26:28 2024] usb usb2: We don't know the algorithms for LPM for this host, disabling LPM.
[Sat May 25 22:26:28 2024] usb usb2: New USB device found, idVendor=1d6b, idProduct=0003, bcdDevice= 6.08
[Sat May 25 22:26:28 2024] usb usb2: New USB device strings: Mfr=3, Product=2, SerialNumber=1
[Sat May 25 22:26:28 2024] usb usb2: Product: xHCI Host Controller
[Sat May 25 22:26:28 2024] usb usb2: Manufacturer: Linux 6.8.9-405.asahi.fc40.aarch64+16k xhci-hcd
[Sat May 25 22:26:28 2024] usb usb2: SerialNumber: xhci-hcd.0.auto
[Sat May 25 22:26:28 2024] hub 2-0:1.0: USB hub found
[Sat May 25 22:26:28 2024] hub 2-0:1.0: 1 port detected
[Sat May 25 22:26:29 2024] usb 1-1: new high-speed USB device number 2 using xhci-hcd
[Sat May 25 22:26:29 2024] usb 1-1: New USB device found, idVendor=2717, idProduct=5007, bcdDevice= 1.30
[Sat May 25 22:26:29 2024] usb 1-1: New USB device strings: Mfr=1, Product=2, SerialNumber=0
[Sat May 25 22:26:29 2024] usb 1-1: Product: 4-Port USB 2.0 Hub
[Sat May 25 22:26:29 2024] usb 1-1: Manufacturer: Generic
[Sat May 25 22:26:29 2024] hub 1-1:1.0: USB hub found
[Sat May 25 22:26:29 2024] hub 1-1:1.0: 4 ports detected
[Sat May 25 22:26:29 2024] usb 1-1.4: new high-speed USB device number 3 using xhci-hcd
[Sat May 25 22:26:29 2024] usb 1-1.4: New USB device found, idVendor=0bda, idProduct=5412, bcdDevice= 1.36
[Sat May 25 22:26:29 2024] usb 1-1.4: New USB device strings: Mfr=1, Product=2, SerialNumber=0
[Sat May 25 22:26:29 2024] usb 1-1.4: Product: 4-Port USB 2.0 Hub
[Sat May 25 22:26:29 2024] usb 1-1.4: Manufacturer: Generic
[Sat May 25 22:26:29 2024] hub 1-1.4:1.0: USB hub found
[Sat May 25 22:26:29 2024] hub 1-1.4:1.0: 2 ports detected
[Sat May 25 22:26:32 2024] usb 2-1: new SuperSpeed USB device number 2 using xhci-hcd
[Sat May 25 22:26:32 2024] usb 2-1: New USB device found, idVendor=2717, idProduct=5006, bcdDevice= 1.30
[Sat May 25 22:26:32 2024] usb 2-1: New USB device strings: Mfr=1, Product=2, SerialNumber=0
[Sat May 25 22:26:32 2024] usb 2-1: Product: 4-Port USB 3.0 Hub
[Sat May 25 22:26:32 2024] usb 2-1: Manufacturer: Generic
[Sat May 25 22:26:32 2024] hub 2-1:1.0: USB hub found
[Sat May 25 22:26:32 2024] hub 2-1:1.0: 4 ports detected
[Sat May 25 22:26:32 2024] usb 2-1.2: new SuperSpeed USB device number 3 using xhci-hcd
[Sat May 25 22:26:32 2024] usb 2-1.2: New USB device found, idVendor=0bda, idProduct=8153, bcdDevice=31.00
[Sat May 25 22:26:32 2024] usb 2-1.2: New USB device strings: Mfr=1, Product=2, SerialNumber=6
[Sat May 25 22:26:32 2024] usb 2-1.2: Product: USB 10/100/1000 LAN
[Sat May 25 22:26:32 2024] usb 2-1.2: Manufacturer: Realtek
[Sat May 25 22:26:32 2024] usb 2-1.2: SerialNumber: 001000001
[Sat May 25 22:26:32 2024] usb 2-1.4: new SuperSpeed USB device number 4 using xhci-hcd
[Sat May 25 22:26:32 2024] usb 2-1.4: New USB device found, idVendor=0bda, idProduct=0412, bcdDevice= 1.36
[Sat May 25 22:26:32 2024] usb 2-1.4: New USB device strings: Mfr=1, Product=2, SerialNumber=0
[Sat May 25 22:26:32 2024] usb 2-1.4: Product: 4-Port USB 3.0 Hub
[Sat May 25 22:26:32 2024] usb 2-1.4: Manufacturer: Generic
[Sat May 25 22:26:32 2024] hub 2-1.4:1.0: USB hub found
[Sat May 25 22:26:32 2024] hub 2-1.4:1.0: 2 ports detected
[Sat May 25 22:26:32 2024] usbcore: registered new device driver r8152-cfgselector
[Sat May 25 22:26:33 2024] r8152-cfgselector 2-1.2: reset SuperSpeed USB device number 3 using xhci-hcd
[Sat May 25 22:26:33 2024] r8152 2-1.2:1.0: load rtl8153b-2 v2 04/27/23 successfully
[Sat May 25 22:26:33 2024] r8152 2-1.2:1.0 eth0: v1.12.13
[Sat May 25 22:26:33 2024] usbcore: registered new interface driver r8152
[Sat May 25 22:26:33 2024] usbcore: registered new interface driver cdc_ether
[Sat May 25 22:26:33 2024] usbcore: registered new interface driver r8153_ecm
[Sat May 25 22:26:33 2024] r8152 2-1.2:1.0 enu1u2: renamed from eth0
[Sat May 25 22:30:37 2024] r8152 2-1.2:1.0 enu1u2: carrier on
[Sat May 25 22:30:54 2024] r8152 2-1.2:1.0 enu1u2: carrier off
[Sat May 25 22:31:01 2024] r8152 2-1.2:1.0 enu1u2: Stop submitting intr, status -71
[Sat May 25 22:31:03 2024] r8152-cfgselector 2-1.2: USB disconnect, device number 3
[Sat May 25 22:31:03 2024] usb 2-1.4: USB disconnect, device number 4
[Sat May 25 22:31:03 2024] usb 2-1: reset SuperSpeed USB device number 2 using xhci-hcd
[Sat May 25 22:31:03 2024] usb 2-1.4: new SuperSpeed USB device number 5 using xhci-hcd
[Sat May 25 22:31:03 2024] usb 2-1.4: New USB device found, idVendor=0bda, idProduct=0412, bcdDevice= 1.36
[Sat May 25 22:31:03 2024] usb 2-1.4: New USB device strings: Mfr=1, Product=2, SerialNumber=0
[Sat May 25 22:31:03 2024] usb 2-1.4: Product: 4-Port USB 3.0 Hub
[Sat May 25 22:31:03 2024] usb 2-1.4: Manufacturer: Generic
[Sat May 25 22:31:03 2024] hub 2-1.4:1.0: USB hub found
[Sat May 25 22:31:03 2024] hub 2-1.4:1.0: 2 ports detected
[Sat May 25 22:31:05 2024] usb 2-1.2: new SuperSpeed USB device number 6 using xhci-hcd
[Sat May 25 22:31:05 2024] usb 2-1.2: New USB device found, idVendor=0bda, idProduct=8153, bcdDevice=31.00
[Sat May 25 22:31:05 2024] usb 2-1.2: New USB device strings: Mfr=1, Product=2, SerialNumber=6
[Sat May 25 22:31:05 2024] usb 2-1.2: Product: USB 10/100/1000 LAN
[Sat May 25 22:31:05 2024] usb 2-1.2: Manufacturer: Realtek
[Sat May 25 22:31:05 2024] usb 2-1.2: SerialNumber: 001000001
[Sat May 25 22:31:05 2024] r8152-cfgselector 2-1.2: reset SuperSpeed USB device number 6 using xhci-hcd
[Sat May 25 22:31:05 2024] r8152 2-1.2:1.0: load rtl8153b-2 v2 04/27/23 successfully
[Sat May 25 22:31:05 2024] r8152 2-1.2:1.0 eth0: v1.12.13
[Sat May 25 22:31:05 2024] r8152 2-1.2:1.0 enu1u2: renamed from eth0
[Sat May 25 22:31:09 2024] r8152 2-1.2:1.0 enu1u2: carrier on
[Sat May 25 22:49:11 2024] a
```

## nvmetcp 也是不可以使用的，绝望
```txt
🧀      sudo nvme discover -t tcp -a 10.0.0.2 -s 4420

No transport address for 'apple-nvme'
failed to lookup ctrl
Failed to open /dev/nvme-fabrics: No such file or directory
option "transport" ignored
option "traddr" ignored
option "host_traddr" ignored
option "host_iface" ignored
option "trsvcid" ignored
option "hostnqn" ignored
option "hostid" ignored
Failed to open /dev/nvme-fabrics: No such file or directory
failed to add controller, error failed to open nvme-fabrics device
```

## 插上电源
```txt
[73159.893185] macsmc-power macsmc-power: Port 1 state change (charge port: 1)
[73159.903961] macsmc-power macsmc-power: Charging: 0
[73159.909902] macsmc-rtkit 23e400000.smc: RTKit: syslog message: aceElec.cpp:703:      Elec: Elec Cause 0x20
[73195.335580] macsmc-rtkit 23e400000.smc: RTKit: syslog message: aceElec.cpp:703:      Elec: Elec Cause 0x0
[73195.418983] macsmc-power macsmc-power: Port 1 state change (charge port: 1)
[73195.419623] macsmc-power macsmc-power: Port 1 state change (charge port: 1)
[73195.419689] macsmc-power macsmc-power: Charging: 1
[73195.423993] macsmc-power macsmc-power: Port 1 state change (charge port: 1)
[73195.430754] macsmc-rtkit 23e400000.smc: RTKit: syslog message: aceElec.cpp:703:      Elec: Elec Cause 0x0
[73195.435352] macsmc-rtkit 23e400000.smc: RTKit: syslog message: aceElec.cpp:703:      Elec: Elec Cause 0x0
[73195.445602] macsmc-power macsmc-power: Charging: 0
[73195.822306] macsmc-power macsmc-power: Port 1 state change (charge port: 1)
[73195.822369] macsmc-power macsmc-power: Charging: 1
[73195.825019] macsmc-power macsmc-power: Port 1 state change (charge port: 1)
[73195.833547] macsmc-rtkit 23e400000.smc: RTKit: syslog message: aceElec.cpp:703:      Elec: Elec Cause 0x8000
```


## 测试下 kdump 机制

## 将启动的时候声音 disable 掉
https://www.howtogeek.com/260693/how-to-disable-the-boot-sound-or-startup-chime-on-a-mac/#:~:text=Key%20Takeaways,Macs%20without%20the%20graphical%20option.

## 似乎无法简单的 enable kdump
```txt
🧀  sudo systemctl status kdump.service
[sudo] password for martins3:
× kdump.service - Crash recovery kernel arming
     Loaded: loaded (/usr/lib/systemd/system/kdump.service; enabled; preset: disabled)
    Drop-In: /usr/lib/systemd/system/service.d
             └─10-timeout-abort.conf
     Active: failed (Result: exit-code) since Mon 2024-05-27 01:55:56 EDT; 50s ago
    Process: 1466 ExecStart=/usr/bin/kdumpctl start (code=exited, status=1/FAILURE)
   Main PID: 1466 (code=exited, status=1/FAILURE)
        CPU: 155ms

May 27 01:55:56 bogon systemd[1]: Starting kdump.service - Crash recovery kernel arming...
May 27 01:55:56 bogon kdumpctl[1479]: kdump: No kdump initial ramdisk found.
May 27 01:55:56 bogon kdumpctl[1479]: kdump: Rebuilding /boot/initramfs-6.8.9-405.asahi.fc40.aarch64+16kkdump.img
May 27 01:55:56 bogon kdumpctl[1479]: kdump: mkdumprd: failed to make kdump initrd
May 27 01:55:56 bogon kdumpctl[1479]: kdump: Starting kdump: [FAILED]
May 27 01:55:56 bogon systemd[1]: kdump.service: Main process exited, code=exited, status=1/FAILURE
May 27 01:55:56 bogon systemd[1]: kdump.service: Failed with result 'exit-code'.
May 27 01:55:56 bogon systemd[1]: Failed to start kdump.service - Crash recovery kernel arming.
```

kernel crash 的事情还是非常常见的，但是 kdump 没有实现。

在 https://github.com/AsahiLinux/docs 中搜索不到任何相关的信息

## asahi linux built from source
- https://codentium.com/building-the-linux-asahi-kernel/

### 解决掉 nvme tcp 的问题

## 有趣
https://news.ycombinator.com/item?id=40585842


## iommu


```txt
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    link_path_walk.part.0.constprop.0+396
    path_openat+132
    do_filp_open+168
    do_sys_openat2+208
    __arm64_sys_openat+108
    invoke_syscall+116
    el0_svc_common.constprop.0+200
    do_el0_svc+36
    el0_svc+60
    el0t_64_sync_handler+288
    el0t_64_sync+404
]: 1
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    __d_lookup+108
    d_hash_and_lookup+124
    proc_fill_cache+104
    proc_readfd_common+212
    proc_readfd+32
    iterate_dir+240
    __arm64_sys_getdents64+120
    invoke_syscall+116
    el0_svc_common.constprop.0+200
    do_el0_svc+36
    el0_svc+60
    el0t_64_sync_handler+288
    el0t_64_sync+404
]: 1
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    do_sys_openat2+148
    __arm64_sys_openat+108
    invoke_syscall+116
    el0_svc_common.constprop.0+200
    do_el0_svc+36
    el0_svc+60
    el0t_64_sync_handler+288
    el0t_64_sync+404
]: 1
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+264
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+60
    secondary_start_kernel+224
    __secondary_switched+184
]: 1
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+228
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+60
    rest_init+264
    arch_call_rest_init+24
    start_kernel+820
    __primary_switched+184
]: 1
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+260
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+64
    secondary_start_kernel+224
    __secondary_switched+184
]: 1
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+192
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+64
    secondary_start_kernel+224
    __secondary_switched+184
]: 1
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el0_interrupt+80
    __el0_irq_handler_common+24
    el0t_64_irq_handler+16
    el0t_64_irq+404
]: 2
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+264
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+64
    secondary_start_kernel+224
    __secondary_switched+184
]: 2
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    brcmf_msgbuf_alloc_pktid+144
    brcmf_msgbuf_rxbuf_ctrl_post+196
    brcmf_msgbuf_process_rx+692
    brcmf_proto_msgbuf_rx_trigger+96
    brcmf_pcie_isr_thread+292
    irq_thread_fn+52
    irq_thread+360
    kthread+244
    ret_from_fork+16
]: 3
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+260
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+60
    secondary_start_kernel+224
    __secondary_switched+184
]: 3
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+228
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+64
    rest_init+264
    arch_call_rest_init+24
    start_kernel+820
    __primary_switched+184
]: 4
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+228
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+60
    secondary_start_kernel+224
    __secondary_switched+184
]: 15
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+228
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+64
    secondary_start_kernel+224
    __secondary_switched+184
]: 24
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    brcmf_msgbuf_alloc_pktid+144
    brcmf_msgbuf_rxbuf_data_post+232
    brcmf_msgbuf_process_rx_complete+324
    brcmf_msgbuf_process_rx+532
    brcmf_proto_msgbuf_rx_trigger+64
    brcmf_pcie_isr_thread+292
    irq_thread_fn+52
    irq_thread+360
    kthread+244
    ret_from_fork+16
]: 32
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    brcmf_msgbuf_alloc_pktid+144
    brcmf_msgbuf_txflow+236
    brcmf_msgbuf_txflow_worker+76
    process_one_work+368
    worker_thread+684
    kthread+244
    ret_from_fork+16
]: 57
```

### 测试 iperf3 的时候
```txt
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+260
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+64
    secondary_start_kernel+224
    __secondary_switched+184
]: 12214
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    brcmf_msgbuf_alloc_pktid+144
    brcmf_msgbuf_txflow+236
    brcmf_msgbuf_txflow_worker+76
    process_one_work+368
    worker_thread+684
    kthread+244
    ret_from_fork+16
]: 53723
```
网卡经过了 usb ?

## asahi linux 如何手动构建

### .config 如何获取?

## 想不到 asahi linux 只有 5 个人
https://www.reddit.com/r/AsahiLinux/comments/1dzwkn8/no_official_news_in_6_months/

## kdump 的第一步就有问题
```txt
🤒  sudo systemctl status kdump.service
[sudo] password for martins3:
× kdump.service - Crash recovery kernel arming
     Loaded: loaded (/usr/lib/systemd/system/kdump.service; enabled; preset: disabled)
    Drop-In: /usr/lib/systemd/system/service.d
             └─10-timeout-abort.conf
     Active: failed (Result: exit-code) since Thu 1970-01-01 08:00:31 CST; 54 years 8 months ago
    Process: 1463 ExecStart=/usr/bin/kdumpctl start (code=exited, status=1/FAILURE)
   Main PID: 1463 (code=exited, status=1/FAILURE)
        CPU: 196ms
```

## 一直期待高通的笔记本，但是没有
https://news.ycombinator.com/item?id=40350408 : 2024/5 还是没有我的心中的神机出现

还不如 mac studio 了。

然后又可以买新的网卡来测试了。

## 将 https://github.com/AsahiLinux/linux 中 commit 的内容都检查下
- 例如其中的 touch screen 东西
- 例如其中的 dts 是做什么的?
  - kernel 需要 dts 来做什么的，那么 UEFI 既然已经识别了，为什么不去提供这些给操作系统的

看看这些代码，
的确是一个很不错的体验

## 如何理解这里的 64kB 的大叶
```txt
➜  lib64 git:(main) ✗ ls /sys/devices/system/node/node0/hugepages/
hugepages-1048576kB  hugepages-2048kB  hugepages-32768kB  hugepages-64kB
```

## 逆天，太强了
- https://news.ycombinator.com/item?id=41799068

## 从 0 开始构建内核需要时间
这是第一次构建内核，后面都是有 ccache
4m50s

四舍五入和 96 core 的 kunpeng 机器都差不多了。


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
