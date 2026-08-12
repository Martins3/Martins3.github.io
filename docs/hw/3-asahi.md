# Asahi Linux

我长期使用 Linux 桌面环境的用户而言，且工作学习和 Linux
关联性很强的用户，Mac 几乎没有任何价值。

![](./asahilinux.jpeg)

因为 Apple 没有对应的硬件手册，全靠社区逆向，内核为了支持 Apple
，不得不增加一些窒息的代码 例如为了 Mac 的 nvme 驱动
[drivers/nvme/host/apple.c](https://github.com/torvalds/linux/blob/master/drivers/nvme/host/apple.c)

总体来说，Asahi Linux 的硬件很特殊，使用过程中，我从中学到了很多 Linux 知识。
## 被修理掉的问题

### ftrace 不能使用

### perf 功能无法正常支持
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

### 无法简单的 enable kdump

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

2026-06-15 可以了，但是无法正常的生成

### nvme over tcp
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

## 有趣的发现

### 64kB hugepages
```txt
➜  lib64 git:(main) ✗ ls /sys/devices/system/node/node0/hugepages/
hugepages-1048576kB  hugepages-2048kB  hugepages-32768kB  hugepages-64kB
```

2026-06-15 6.19.14-400.asahi.fc42.aarch64+16k 中发现已经不存在了

```txt
🤒  ls /sys/devices/system/node/node0/hugepages/
hugepages-2048kB  hugepages-1048576kB
```



### 网卡是接到 usb 上的
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
### 中断控制器都不是 GIC

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

### 查看电池的剩余量
```sh
cat /sys/class/power_supply/macsmc-battery/capacity
```

### 插上电源自动存在如下日志
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
### iommu

记录在 docs/hw/3-asahi.txt 中

## 性能
从 0 开始构建内核需要时间大约 4m50s

这是第一次构建内核，后面都是有 ccache ，

四舍五入和 96 core 的 kunpeng 机器都差不多了。


## asahi linux 中启动后 feature 解析

```txt
Oct 13 08:00:00 fedora kernel: smp: Brought up 1 node, 8 CPUs
Oct 13 08:00:00 fedora kernel: SMP: Total of 8 processors activated.
Oct 13 08:00:00 fedora kernel: CPU: All CPU(s) started at EL2
Oct 13 08:00:00 fedora kernel: CPU features: detected: Branch Target Identification
Oct 13 08:00:00 fedora kernel: CPU features: detected: ARMv8.4 Translation Table Level
Oct 13 08:00:00 fedora kernel: CPU features: detected: Data cache clean to the PoU not required for I/D coherence
Oct 13 08:00:00 fedora kernel: CPU features: detected: Common not Private translations
Oct 13 08:00:00 fedora kernel: CPU features: detected: CRC32 instructions
Oct 13 08:00:00 fedora kernel: CPU features: detected: Data cache clean to Point of Deep Persistence
Oct 13 08:00:00 fedora kernel: CPU features: detected: Data cache clean to Point of Persistence
Oct 13 08:00:00 fedora kernel: CPU features: detected: Data independent timing control (DIT)
Oct 13 08:00:00 fedora kernel: CPU features: detected: E0PD
Oct 13 08:00:00 fedora kernel: CPU features: detected: Enhanced Counter Virtualization
Oct 13 08:00:00 fedora kernel: CPU features: detected: Enhanced Virtualization Traps
Oct 13 08:00:00 fedora kernel: CPU features: detected: Fine Grained Traps
Oct 13 08:00:00 fedora kernel: CPU features: detected: Generic authentication (IMP DEF algorithm)
Oct 13 08:00:00 fedora kernel: CPU features: detected: RCpc load-acquire (LDAPR)
Oct 13 08:00:00 fedora kernel: CPU features: detected: LSE atomic instructions
Oct 13 08:00:00 fedora kernel: CPU features: detected: Privileged Access Never
Oct 13 08:00:00 fedora kernel: CPU features: detected: RAS Extension Support
Oct 13 08:00:00 fedora kernel: CPU features: detected: Speculation barrier (SB)
Oct 13 08:00:00 fedora kernel: CPU features: detected: Stage-2 Force Write-Back
Oct 13 08:00:00 fedora kernel: CPU features: detected: TLB range maintenance instructions
Oct 13 08:00:00 fedora kernel: CPU features: detected: Speculative Store Bypassing Safe (SSBS)
Oct 13 08:00:00 fedora kernel: alternatives: applying system-wide alternatives
Oct 13 08:00:00 fedora kernel: CPU features: detected: ACTLR virtualization (architectural?)
Oct 13 08:00:00 fedora kernel: CPU features: detected: TSO memory model (Apple)
Oct 13 08:00:00 fedora kernel: Memory: 15877984K/16296480K available (22400K kernel code, 6098K rwdata, 18896K rodata, 14464K init, 10371K bss, 334832K reserved, 65536K cma-reserved)
```


二、关键启动行

smp: Brought up 1 node, 8 CPUs / Total of 8 processors activated
SMP 子系统把 8 个核全部拉起来。注意 boot log 是按 CPU 逐个 online 的（每核一行 [CPU0] ... 之类，你没贴出来），这里是最终汇总：1 个 NUMA 节点。

CPU: All CPU(s) started at EL2
这句信息量最大。普通 ARM64 机器内核跑在 EL1，只有虚拟化层跑 EL2。这里所有核都在 EL2 启动 = 内核启用了 VHE（Virtualization Host Extensions），即
内核自己就是 hypervisor，以 EL2 身份运行。
这也是后面那一堆
Enhanced Counter Virtualization / Enhanced Virtualization Traps / Fine Grained Traps
/ Stage-2 Force Write-Back 特性的前提——这些全是给 KVM 用的。Asahi 的 KVM（muvm）就依赖这个。

三、CPU features 分类解读

alternatives: applying system-wide alternatives 这行表示：内核检测完特性后，开始运行时打补丁（把指令序列替换成利用新特性的版本，比如把 LDXR/STXR
 循环换成 LSE 单指令原子操作）。所以下面这堆 feature 是"检测结果"，之后会被 alternatives 机制用起来。

1. 原子与内存模型
- LSE atomic instructions — CAS/LDADD 等单指令原子操作，qspinlock、refcount 都靠它加速
- RCpc load-acquire (LDAPR) — 弱化版的 acquire 语义，qspinlock 用它
- TSO memory model (Apple) — Apple 核实现的是 x86 式全序存储模型，比 ARM 弱内存模型"强"，内核检测到后可以放松屏障（用更便宜的 barrier）。这是 Asahi 内核的本地补丁，主线里没有这行字符串。

2. 安全加固（Spectre/Meltdown 类缓解）
- Branch Target Identification (BTI) — 指令跳转目标校验，防 JOP
- Privileged Access Never (PAN) — 内核默认禁止访问用户内存，内核态提权防护
- E0PD — 阻止 EL0 的投机访问，缓解侧信道
- Speculative Store Bypassing Safe (SSBS) — SSB 漏洞的硬件缓解开关
- Speculation barrier (SB) — 投机屏障指令
- Generic authentication (IMP DEF algorithm) — 指针认证（PAC），苹果实现版

3. 缓存/内存管理
- ARMv8.4 Translation Table Level — 用上了更高的页表级数（大地址空间支持）
- Data cache clean to the PoU not required for I/D coherence — 硬件自动保证 I/D 缓存一致性，内核做指令 patch（alternatives/ftrace/kprobes）时不 用再手动刷 dcache
- Data cache clean to Point of Persistence / Deep Persistence — 提供 DC CVAP/CVADP 指令
- Common not Private translations (CNP) — 多核可共享页表，省 TLB 维护
- TLB range maintenance instructions — TLBI 批量失效，比逐条刷快

4. KVM 虚拟化专属
- Enhanced Counter Virtualization — 虚拟化系统计数器偏移（CNTPOFF），虚拟机里时钟不抖
- Enhanced Virtualization Traps / Fine Grained Traps — 细粒度指令陷出控制，KVM 少陷入、快退出
- Stage-2 Force Write-Back — KVM 二级页表内存属性简化

5. 杂项
- CRC32 instructions — 硬件 CRC32C
- DIT — 数据无关定时（加密常量时间操作）
- RAS Extension Support — 硬件错误上报
- ACTLR virtualization (architectural?) — Asahi 内核的本地检测项：Apple 的 ACTLR 辅助控制寄存器访问被虚拟化/陷出处理，"architectural?" 表示"这算不算架构规范行为还不确定"。

## 链接
- https://daynix.github.io/2023/06/03/developing-qemu-on-asahi-linux-linux-port-for-apple-silicon.html
- http://cdn.kernel.org/pub/linux/kernel/people/will/docs/qemu/qemu-arm64-howto.html
- https://futurewei-cloud.github.io/ARM-Datacenter/qemu/how-to-launch-aarch64-vm/
- https://asahilinux.org/2022/12/gpu-drivers-now-in-asahi-linux/
- asahi linux built from source
	- https://codentium.com/building-the-linux-asahi-kernel/
- https://news.ycombinator.com/item?id=40585842 : Vulkan1.3 on the M1 in one month
- https://www.reddit.com/r/AsahiLinux/comments/1dzwkn8/no_official_news_in_6_months/
	- 想不到 asahi linux 只有 5 个人
- https://news.ycombinator.com/item?id=40350408 : 2024/5 还是没有我的心中的神机出现
- https://github.com/AsahiLinux/linux 内核源码
- https://news.ycombinator.com/item?id=41799068 : AAA Gaming on Asahi Linux

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
