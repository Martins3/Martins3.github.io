# sysfs irq
## /proc/stat

算是一个 legacy 统计接口了
```txt
cpu  51210 548 98120 18538659 919 10581 19549 47 0 0
cpu0 11068 109 14285 4653460 210 1499 904 8 0 0
cpu1 12252 234 27207 4629717 157 3606 6422 13 0 0
cpu2 15219 90 35241 4611840 192 3292 11709 15 0 0
cpu3 12669 113 21385 4643639 358 2182 512 8 0 0
intr 22062934 0 39952 3835773 0 0 0 122 0 0 0 6111010 0 0
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 1 0 0 11095 9747 10090 20049 0 5922239 0 0
 21 0 5917379 3269 0 99767 82123 0 0 0 20 21 0 0 0 0 0 0 2
56 0 0
ctxt 16649981
btime 1752935445
processes 27065
procs_running 1
procs_blocked 0
softirq 15316818 0 763086 307 11949992 0 0 2082 2091354 0 509997
```

## /proc/softirq

不言自明，就是各个 softirq 再每一个 CPU 上执行的次数

```txt
🧀  cat /proc/softirqs
                    CPU0       CPU1       CPU2       CPU3
          HI:          0          0          0          0
       TIMER:     165934     214905     234256     140886
      NET_TX:        102         41         40        123
      NET_RX:     108032    5830758    5819179      84390
       BLOCK:          0          0          0          0
    IRQ_POLL:          0          0          0          0
     TASKLET:        337        857         45        833
       SCHED:     802183     448203     529477     289775
     HRTIMER:          0          0          0          0
         RCU:     136717     122756     127614     120001
```

## /proc/irq/
- [x] 代码跟踪 : 如何利用 /proc/irq 调整 io apic 的 cpu affinity
  - 最后就是调用到 irq_chip::irq_set_affinity 的位置
  - 并且和[^3] 中间描述的 ioapic 的中断选项完全相同

> affinity :
>   - IRQ affinity on SMP. If this is an IPI related irq, then this is the mask of the CPUs to which an IPI can be sent.
> effective_affinity :
>  - The effective IRQ affinity on SMP as some irq chips do not allow multi CPU destinations. A subset of affinity.



- /proc/irq 的代码都在 kernel/irq/proc.c

### 配置中断亲和性

cat /proc/interrupts
```txt
 157:      15603      16812     976943      29310  IR-PCI-MSIX-0000:01:00.0    1-edge      enp1s0-TxRx-0
 158:     967713      17281      23919      24808  IR-PCI-MSIX-0000:01:00.0    2-edge      enp1s0-TxRx-1
 159:      24484    1708690      27964      18901  IR-PCI-MSIX-0000:01:00.0    3-edge      enp1s0-TxRx-2
 160:      40130      22905      17792      23567  IR-PCI-MSIX-0000:01:00.0    4-edge      enp1s0-TxRx-3
```
可以看到只有 enp1s0-TxRx-1 和 enp1s0-TxRx-2 被使用，这两个中断默认都是绑定一个 CPU 的

尝试修改 smp_affinity ，似乎没有结果:
```txt
🧀  echo 7 > /proc/irq/158/smp_affinity
~/.ssh 🐶
🧀  cat /proc/irq/159/smp_affinity
7
~/.ssh 🐶
🧀  cat /proc/irq/159/effective_affinity_list
1
```

effective_affinity_list 总是一个数字
```txt
@[
    apic_update_irq_cfg+1
    assign_vector_locked+176
    apic_set_affinity+96
    intel_ir_set_affinity+56
    msi_domain_set_affinity+77
    irq_do_set_affinity+448
    irq_set_affinity_locked+409
    irq_set_affinity+63
    write_irq_affinity.isra.0+239
    proc_reg_write+90
    vfs_write+245
    ksys_write+109
    do_syscall_64+130
    entry_SYSCALL_64_after_hwframe+118
]: 1
```
在 apic_update_irq_cfg 中，参数就一个 cpu ，如何从 vector 中选择的看 `assign_vector_locked`

## /sys/kernel/irq

至此，linux irq 的含义是很清晰的
```c
static struct attribute *irq_attrs[] = {
	&per_cpu_count_attr.attr,
	&chip_name_attr.attr,
	&hwirq_attr.attr,
	&type_attr.attr,
	&wakeup_attr.attr,
	&name_attr.attr,
	&actions_attr.attr,
	NULL
};
ATTRIBUTE_GROUPS(irq);
```

在 kernel/irq/irqdesc.c 中 ，使用 irq_to_desc 来获取 。

```txt
🤒  grep -r .
wakeup:disabled
hwirq:1
actions:virtio3-input.0
type:edge
chip_name:PCI-MSIX-0000:00:05.0
per_cpu_count:0,0,0,18437,0,0,0,221,0,0,8,1026968,0,0,0,0,13,0,50895,0,0,5390,0
3,148024189,0,0,0,458912,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
,0,0,0,0,0,0,0
name:edge
```
和 cat /proc/interrupts 中内容基本相同:
```txt
🧀  cat /proc/interrupts | grep virtio3-input.0
 72:          0          0          0      18437          0          0          0        221          0          0          8    1026968          0          0          0          0         13          0      50895          0          0       5390          0          3  148024189          0          0          0     466069          0          0          0  PCI-MSIX-0000:00:05.0   1-edge      virtio3-input.0
```
除了 hwirq

## /proc/interrupts
```txt
➜  irq cat /proc/interrupts
            CPU0       CPU1       CPU2       CPU3       CPU4       CPU5       CPU6       CPU7
   0:          8          0          0          0          0          0          0          0  IR-IO-APIC    2-edge      timer
   1:       2716          0          0          0          0          0          0         14  IR-IO-APIC    1-edge      i8042
   8:          0          1          0          0          0          0          0          0  IR-IO-APIC    8-edge      rtc0
   9:          4         13          0          0          0          0          0          0  IR-IO-APIC    9-fasteoi   acpi
  12:         83          0          0          0          0          0        143          0  IR-IO-APIC   12-edge      i8042
  14:     134097          0         31          0          0          0          0          0  IR-IO-APIC   14-fasteoi   INT344B:00
  16:    1381211          0          0       1125          0          0          0          0  IR-IO-APIC   16-fasteoi   idma64.0, i801_smbus, i2c_designware.0
  17:          0          0          0          0          0          0          0          0  IR-IO-APIC   17-fasteoi   idma64.1, i2c_designware.1
 120:          0          0          0          0          0          0          0          0  DMAR-MSI    0-edge      dmar0
 121:          0          0          0          0          0          0          0          0  DMAR-MSI    1-edge      dmar1
 122:          0          0          0          0          0          0          0          0  IR-PCI-MSI 458752-edge      PCIe PME, aerdrv
 123:          0          0          0          0          0          0          0          0  IR-PCI-MSI 473088-edge      PCIe PME, aerdrv
 124:          0          0          0          0          0          0          0          0  IR-PCI-MSI 475136-edge      PCIe PME, aerdrv
 125:     674114          0          0          0     705672       6110          0          0  IR-PCI-MSI 327680-edge      xhci_hcd
 126:          0         31          0          0          0          0          0          0  IR-PCI-MSI 360448-edge      mei_me
 127:     134097          0         31          0          0          0          0          0  INT344B:00   99  ETD2303:00
 128:          0          0          0         18          0          0          0          0  IR-PCI-MSI 1572864-edge      nvme0q0
 129:          0          0        250          0          0          0          0          0  IR-PCI-MSI 514048-edge      snd_hda_intel:card0
 130:          0          0          0          0      19088          0          0          0  IR-PCI-MSI 1572865-edge      nvme0q1
 131:          0       9431          0          0          0          0          0          0  IR-PCI-MSI 1572866-edge      nvme0q2
 132:          0          0       8073          0          0          0          0          0  IR-PCI-MSI 1572867-edge      nvme0q3
 133:          0          0          0       9255          0          0          0          0  IR-PCI-MSI 1572868-edge      nvme0q4
 134:          0          0          0          0          0       7961          0          0  IR-PCI-MSI 1572869-edge      nvme0q5
 135:          0          0          0          0          0          0      10062          0  IR-PCI-MSI 1572870-edge      nvme0q6
 136:       2222          0     381984          0      22229    1531873      27420       9994  IR-PCI-MSI 32768-edge      i915
 137:       1429        164         31        368       1196          0      12158      75506  IR-PCI-MSI 1048576-edge      iwlwifi
 138:         32          0         27          0          0          0         15          0  IR-PCI-MSI 524288-edge      nvkm
 139:          0          0          0          0          0          0          0       8193  IR-PCI-MSI 1572871-edge      nvme0q7
 NMI:         29         74         74         74         74         72         69         69   Non-maskable interrupts
 LOC:    1895402    1772361    1840003    1797418    1822740    2050418    1775710    1753250   Local timer interrupts
 SPU:          0          0          0          0          0          0          0          0   Spurious interrupts
 PMI:         29         74         74         74         74         72         69         69   Performance monitoring interrupts
 IWI:         93          5      22562          9       1329     107699       1915        540   IRQ work interrupts
 RTR:          0          0          0          0          0          0          0          0   APIC ICR read retries
 RES:     376738     205460     116961     102464      83042      65067      57540      53069   Rescheduling interrupts
 CAL:     171524     158205     159437     164110     160299     161635     165628     161995   Function call interrupts
 TLB:     232182     234240     233417     235103     225417     227488     230138     229758   TLB shootdowns
 TRM:          4          4          4          4          4          4          4          4   Thermal event interrupts
 THR:          0          0          0          0          0          0          0          0   Threshold APIC interrupts
 DFR:          0          0          0          0          0          0          0          0   Deferred Error APIC interrupts
 MCE:          0          0          0          0          0          0          0          0   Machine check exceptions
 MCP:         61         61         61         61         61         61         61         61   Machine check polls
 ERR:          0
 MIS:          0
 PIN:          0          0          0          0          0          0          0          0   Posted-interrupt notification event
 NPI:          0          0          0          0          0          0          0          0   Nested posted-interrupt event
 PIW:          0          0          0          0          0          0          0          0   Posted-interrupt wakeup event
```
才发现，IR-IO-APIC 后面都是添加上设备的，IR-PCI-MSI 主要是和 pcie 相关的设备，而 local apic 的名称直接被该 interrupt 的名字替代了。
- [ ] 1572869-edge : 是什么鬼

尝试分析一下 /proc/interrupts 如何生成的:
- 在 kernel/irq/proc.c::show_interrupts 中间


- /proc/interrupts : proc.c:show_interrupts() 的最后的描述，其实就是相关的 chip
```plain
            CPU0       CPU1       CPU2       CPU3       CPU4       CPU5       CPU6       CPU7
   0:          8          0          0          0          0          0          0          0  IR-IO-APIC    2-edge      timer
   1:      19619          0          0          0          0          0          0       3444  IR-IO-APIC    1-edge      i8042
   8:          0          1          0          0          0          0          0          0  IR-IO-APIC    8-edge      rtc0
   9:         46         68          0          0          0          0          0          0  IR-IO-APIC    9-fasteoi   acpi
  12:        384          0          0          0          0          0        143          0  IR-IO-APIC   12-edge      i8042
  14:     187427          0      28041          0          0          0          0          0  IR-IO-APIC   14-fasteoi   INT344B:00
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
