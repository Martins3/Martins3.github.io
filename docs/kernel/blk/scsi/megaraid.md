# megaraid
## megaraid 的日志 : FW now in Ready state
<!-- 05a586d7-3295-48c2-b17d-66256e87c647 -->

drivers/scsi/megaraid/megaraid_sas_base.c 中看到的，

当使用 qemu 模拟的时候发现，
这里的 `FW now in Ready state` 如何理解，难道是 qemu 中提供了固件吗?

在 qemu 中观察到的 megaraid 相关的日志:
```txt
root@localhost:/home# dmesg | grep mega
[    4.805378] megasas: 07.734.00.00-rc1
[    4.806188] megaraid_sas 0000:00:04.0: BAR:0x1  BAR's base_addr(phys):0x0000004240040000  mapped virt_addr:0x00000000b0a238c1
[    4.806203] megaraid_sas 0000:00:04.0: FW now in Ready state
[    4.806204] megaraid_sas 0000:00:04.0: 63 bit DMA mask and 32 bit consistent mask
[    4.806791] megaraid_sas 0000:00:04.0: requested/available msix 1/1 poll_queue 0
[    4.806792] megaraid_sas 0000:00:04.0: current msix/max num queues   : (1/32)
[    4.806792] megaraid_sas 0000:00:04.0: RDPQ mode     : (disabled)
[    4.807485] megaraid_sas 0000:00:04.0: controller type       : MR(512MB)
[    4.807488] megaraid_sas 0000:00:04.0: Online Controller Reset(OCR)  : Enabled
[    4.807490] megaraid_sas 0000:00:04.0: Secure JBOD support   : No
[    4.807491] megaraid_sas 0000:00:04.0: NVMe passthru support : No
[    4.807491] megaraid_sas 0000:00:04.0: FW provided TM TaskAbort/Reset timeout        : 0 secs/0 secs
[    4.807493] megaraid_sas 0000:00:04.0: JBOD sequence map support     : No
[    4.807493] megaraid_sas 0000:00:04.0: PCI Lane Margining support    : No
[    4.807500] megaraid_sas 0000:00:04.0: megasas_init_mfi: fw_support_ieee=67108864
[    4.807602] megaraid_sas 0000:00:04.0: INIT adapter done
[    4.807605] megaraid_sas 0000:00:04.0: JBOD sequence map is disabled megasas_setup_jbod_map 5803
[    4.807636] megaraid_sas 0000:00:04.0: Unknown command completed! [0x0]
[    4.807651] megaraid_sas 0000:00:04.0: Unknown command completed! [0x0]
[    4.807655] megaraid_sas 0000:00:04.0: pci id                : (0x1000)/(0x0079)/(0x1000)/(0x9261)
[    4.807657] megaraid_sas 0000:00:04.0: unevenspan support    : no
[    4.807658] megaraid_sas 0000:00:04.0: firmware crash dump   : no
[    4.807659] megaraid_sas 0000:00:04.0: JBOD sequence map     : disabled
[    4.807661] megaraid_sas 0000:00:04.0: Max firmware commands: 1007 shared with default hw_queues = 1 poll_queues 0
```
虚拟机中在对应的日志添加 dump_stack 的结果，我猜测就读 mmio 空间的一个位置，然后就知道固件 ready 了:
```txt
	abs_state = instance->instancet->read_fw_status_reg(instance);
```

```txt
[    1.255869] Call Trace:
[    1.255876]  <TASK>
[    1.255878]  dump_stack_lvl+0x6f/0xb0
[    1.255894]  megasas_transition_to_ready+0x5c/0x70 [megaraid_sas]
[    1.255904]  megasas_probe_one.cold+0x129/0x1798 [megaraid_sas]
[    1.255916]  local_pci_probe+0x42/0x80
[    1.255936]  pci_device_probe+0xd8/0x250
[    1.255943]  ? sysfs_do_create_link_sd+0x6e/0xe0
[    1.255962]  really_probe+0xcc/0x350
[    1.255972]  ? pm_runtime_barrier+0x54/0x90
[    1.255980]  ? __pfx___driver_attach+0x10/0x10
[    1.255982]  __driver_probe_device+0x78/0x110
[    1.255984]  driver_probe_device+0x1f/0xa0
[    1.255987]  __driver_attach+0xbe/0x1d0
[    1.255989]  bus_for_each_dev+0x91/0xe0
[    1.255995]  bus_add_driver+0x115/0x200
[    1.255999]  driver_register+0x72/0xd0
[    1.256004]  megasas_init+0xdf/0xff0 [megaraid_sas]
[    1.256011]  ? __pfx_megasas_init+0x10/0x10 [megaraid_sas]
[    1.256016]  do_one_initcall+0x6f/0x2c0
[    1.256023]  do_init_module+0x60/0x230
[    1.256039]  init_module_from_file+0x85/0xc0
[    1.256049]  idempotent_init_module+0x11a/0x310
[    1.256060]  __x64_sys_finit_module+0x6d/0xd0
[    1.256061]  ? syscall_trace_enter+0x70/0x200
[    1.256064]  do_syscall_64+0x74/0xfa0
[    1.256074]  entry_SYSCALL_64_after_hwframe+0x76/0x7e
[    1.256079] RIP: 0033:0x7f8488b02a8d
```

作为对比，这是物理机中的日志结果:
```txt
[  165.854249] megaraid_sas 0000:04:00.0: BAR:0x0  BAR's base_addr(phys):0x0x0000010000100000  mapped virt_addr:0x0000000060ab7aa0
[  165.861758] megaraid_sas 0000:04:00.0: FW now in Ready state
[  165.870982] megaraid_sas 0000:04:00.0: 63 bit DMA mask and 63 bit consistent mask
[  165.882365] megaraid_sas 0000:04:00.0: firmware supports msix        : (128)
[  165.913346] megaraid_sas 0000:04:00.0: requested/available msix: 128/128
[  165.913349] megaraid_sas 0000:04:00.0: current msix/online cpus      : (128/128)
[  165.913350] megaraid_sas 0000:04:00.0: RDPQ mode     : (enabled)
[  165.913363] megaraid_sas 0000:04:00.0: Current firmware supports maximum commands: 5101       LDIO threshold: 0
[  165.990896] megaraid_sas 0000:04:00.0: Performance mode :Latency (latency index = 1)
[  166.002264] megaraid_sas 0000:04:00.0: FW supports sync cache        : Yes
[  166.012139] megaraid_sas 0000:04:00.0: megasas_disable_intr_fusion is called outbound_intr_mask:0x40000009
[  166.288262] megaraid_sas 0000:04:00.0: FW supports atomic descriptor : Yes
[  166.328886] megaraid_sas 0000:04:00.0: FW provided supportMaxExtLDs: 1       max_lds: 240
[  166.350216] megaraid_sas 0000:04:00.0: controller type       : MR(8192MB)
[  166.440677] megaraid_sas 0000:04:00.0: Online Controller Reset(OCR)  : Enabled
[  166.440679] megaraid_sas 0000:04:00.0: Secure JBOD support   : Yes
[  166.440680] megaraid_sas 0000:04:00.0: NVMe passthru support : Yes
[  166.440682] megaraid_sas 0000:04:00.0: FW provided TM TaskAbort/Reset timeout        : 6 secs/60 secs
[  166.440688] megaraid_sas 0000:04:00.0: PCI Lane Margining support    : Yes
[  166.496220] megaraid_sas 0000:04:00.0: JBOD sequence map support     : Yes
[  166.665431] megaraid_sas 0000:04:00.0: NVME page size        : (4096)
[  166.800715] megaraid_sas 0000:04:00.0: megasas_enable_intr_fusion is called outbound_intr_mask:0x40000000
[  166.815261] megaraid_sas 0000:04:00.0: INIT adapter done
[  166.994602] megaraid_sas 0000:04:00.0: Snap dump wait time   : 15
[  167.003475] megaraid_sas 0000:04:00.0: pci id                : (0x1000)/(0x10e2)/(0x1000)/(0x4000)
[  167.014340] megaraid_sas 0000:04:00.0: unevenspan support    : no
[  167.023237] megaraid_sas 0000:04:00.0: firmware crash dump   : no
[  167.032179] megaraid_sas 0000:04:00.0: JBOD sequence map     : enabled
[  167.077265] megaraid_sas 0000:04:00.0: Max firmware commands: 5100 shared with default HW queues: 1 poll_queues: 0
```
然后将这个 HBA 直通到虚拟机中，结果为:
```txt
[    2.645289] megasas: 07.735.03.00
[    2.649725] megaraid_sas 0000:00:02.0: BAR:0x1  BAR's base_addr(phys):0x0x00000e0000040000  mapped virt_addr:0x000000009ee056c0
[    2.660357] megaraid_sas 0000:00:02.0: FW now in Ready state
[    2.661948] megaraid_sas 0000:00:02.0: 63 bit DMA mask and 32 bit consistent mask
[    2.665660] megaraid_sas 0000:00:02.0: requested/available msix: 1/1
[    2.667451] megaraid_sas 0000:00:02.0: current msix/online cpus      : (1/32)
[    2.669045] megaraid_sas 0000:00:02.0: RDPQ mode     : (disabled)
[    2.678477] megaraid_sas 0000:00:02.0: controller type       : MR(512MB)
[    2.679969] megaraid_sas 0000:00:02.0: Online Controller Reset(OCR)  : Enabled
[    2.681595] megaraid_sas 0000:00:02.0: Secure JBOD support   : No
[    2.681608] megaraid_sas 0000:00:02.0: NVMe passthru support : No
[    2.688189] megaraid_sas 0000:00:02.0: FW provided TM TaskAbort/Reset timeout        : 0 secs/0 secs
[    2.690194] megaraid_sas 0000:00:02.0: PCI Lane Margining support    : No
[    2.691813] megaraid_sas 0000:00:02.0: JBOD sequence map support     : No
[    2.691828] megaraid_sas 0000:00:02.0: megasas_init_mfi: fw_support_ieee=67108864
[    2.692015] megaraid_sas 0000:00:02.0: INIT adapter done
[    2.696596] megaraid_sas 0000:00:02.0: JBOD sequence map is disabled megasas_setup_jbod_map 6208
[    2.696643] megaraid_sas 0000:00:02.0: pci id                : (0x1000)/(0x0079)/(0x1000)/(0x9261)
[    2.696644] megaraid_sas 0000:00:02.0: unevenspan support    : no
[    2.696645] megaraid_sas 0000:00:02.0: firmware crash dump   : no
[    2.696647] megaraid_sas 0000:00:02.0: JBOD sequence map     : disabled
[    2.696650] megaraid_sas 0000:00:02.0: Max firmware commands: 1007 shared with default HW queues: 1 poll_queues: 0
[    2.696732] megaraid_sas 0000:00:02.0: Unknown command completed! [0x0]
[    2.709857] megaraid_sas 0000:00:02.0: Unknown command completed! [0x0]
[    2.709911] megaraid_sas 0000:00:02.0: Unknown command completed! [0x0]
[    2.711098] megaraid_sas 0000:00:06.0: BAR:0x0  BAR's base_addr(phys):0x0x00000e0400000000  mapped virt_addr:0x00000000c7c606dd
[    2.711117] megaraid_sas 0000:00:06.0: FW now in Ready state
[    2.711120] megaraid_sas 0000:00:06.0: 63 bit DMA mask and 63 bit consistent mask
[    2.713153] megaraid_sas 0000:00:06.0: firmware supports msix        : (128)
[    2.725307] megaraid_sas 0000:00:06.0: requested/available msix: 33/33
[    2.725310] megaraid_sas 0000:00:06.0: current msix/online cpus      : (33/32)
[    2.725311] megaraid_sas 0000:00:06.0: RDPQ mode     : (enabled)
[    2.725314] megaraid_sas 0000:00:06.0: Current firmware supports maximum commands: 5101       LDIO threshold: 0
[    2.821727] megaraid_sas 0000:00:06.0: Performance mode :Latency (latency index = 1)
[    2.823543] megaraid_sas 0000:00:06.0: FW supports sync cache        : Yes
[    2.824997] megaraid_sas 0000:00:06.0: megasas_disable_intr_fusion is called outbound_intr_mask:0x40000009
[    2.914469] megaraid_sas 0000:00:06.0: FW supports atomic descriptor : Yes
[    2.981463] megaraid_sas 0000:00:06.0: FW provided supportMaxExtLDs: 1       max_lds: 240
[    2.982794] megaraid_sas 0000:00:06.0: controller type       : MR(8192MB)
[    2.983887] megaraid_sas 0000:00:06.0: Online Controller Reset(OCR)  : Enabled
[    2.985106] megaraid_sas 0000:00:06.0: Secure JBOD support   : Yes
[    2.986162] megaraid_sas 0000:00:06.0: NVMe passthru support : Yes
[    2.987236] megaraid_sas 0000:00:06.0: FW provided TM TaskAbort/Reset timeout        : 6 secs/60 secs
[    2.988655] megaraid_sas 0000:00:06.0: PCI Lane Margining support    : Yes
[    2.989805] megaraid_sas 0000:00:06.0: JBOD sequence map support     : Yes
[    3.081751] megaraid_sas 0000:00:06.0: NVME page size        : (4096)
[    3.135205] megaraid_sas 0000:00:06.0: megasas_enable_intr_fusion is called outbound_intr_mask:0x40000000
[    3.137181] megaraid_sas 0000:00:06.0: INIT adapter done
[    3.138853] megaraid_sas 0000:00:06.0: Snap dump wait time   : 15
[    3.140134] megaraid_sas 0000:00:06.0: pci id                : (0x1000)/(0x10e2)/(0x1000)/(0x4000)
[    3.141728] megaraid_sas 0000:00:06.0: unevenspan support    : no
[    3.142979] megaraid_sas 0000:00:06.0: firmware crash dump   : no
[    3.144242] megaraid_sas 0000:00:06.0: JBOD sequence map     : enabled
[    3.146342] megaraid_sas 0000:00:06.0: Max firmware commands: 5100 shared with default HW queues: 1 poll_queues: 0
```

## 在 qemu 中 scsi-cd 可以在接入的 scsi 的机制是 megaraid 吗

答案是，当然可以，的确可以走通
```txt
root@localhost:~# ls -la /sys/class/block
总计 0
drwxr-xr-x.  2 root root 0 12月27日 21:57 .
drwxr-xr-x. 67 root root 0 12月27日 21:55 ..
lrwxrwxrwx.  1 root root 0 12月27日 21:55 dm-0 -> ../../devices/virtual/block/dm-0
lrwxrwxrwx.  1 root root 0 12月27日 21:55 sda -> ../../devices/pci0000:00/0000:00:03.0/virtio0/host2/target2:0:1/2:0:1:0/block/sda
lrwxrwxrwx.  1 root root 0 12月27日 21:57 sdb -> ../../devices/pci0000:00/0000:00:04.0/host3/target3:2:1/3:2:1:0/block/sdb
lrwxrwxrwx.  1 root root 0 12月27日 21:57 sr0 -> ../../devices/pci0000:00/0000:00:04.0/host3/target3:2:0/3:2:0:0/block/sr0
lrwxrwxrwx.  1 root root 0 12月27日 21:55 vda -> ../../devices/pci0000:00/0000:00:05.0/virtio1/block/vda
lrwxrwxrwx.  1 root root 0 12月27日 21:55 vda1 -> ../../devices/pci0000:00/0000:00:05.0/virtio1/block/vda/vda1
lrwxrwxrwx.  1 root root 0 12月27日 21:55 vda2 -> ../../devices/pci0000:00/0000:00:05.0/virtio1/block/vda/vda2
lrwxrwxrwx.  1 root root 0 12月27日 21:55 vda3 -> ../../devices/pci0000:00/0000:00:05.0/virtio1/block/vda/vda3
lrwxrwxrwx.  1 root root 0 12月27日 21:55 zram0 -> ../../devices/virtual/block/zram0
root@localhost:~# lspci -s 0000:00:04.0
00:04.0 RAID bus controller: Broadcom / LSI MegaRAID SAS 2108 [Liberator]
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
