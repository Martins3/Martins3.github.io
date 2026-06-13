# 先从 debugfs 仔细看看内核的东西

- 这个东西如何理解?


```c
static void irq_domain_debug_show_one(struct seq_file *m, struct irq_domain *d, int ind)
{
	seq_printf(m, "%*sname:   %s\n", ind, "", d->name);
	seq_printf(m, "%*ssize:   %u\n", ind + 1, "", d->revmap_size);
	seq_printf(m, "%*smapped: %u\n", ind + 1, "", d->mapcount);
	seq_printf(m, "%*sflags:  0x%08x\n", ind +1 , "", d->flags);
	irq_debug_show_bits(m, ind, d->flags, irqdomain_flags, ARRAY_SIZE(irqdomain_flags));
	if (d->ops && d->ops->debug_show)
		d->ops->debug_show(m, d, NULL, ind + 1);
#ifdef	CONFIG_IRQ_DOMAIN_HIERARCHY
	if (!d->parent)
		return;
	seq_printf(m, "%*sparent: %s\n", ind + 1, "", d->parent->name);
	irq_domain_debug_show_one(m, d->parent, ind + 4);
  // 这里看上去是递归的，但是实际上只有在
  // x86_vector_domain_ops 中注册 x86_vector_debug_show 提供注册了。
#endif
}
```

所以，只有这个头有用而已:
```txt
name:   IO-APIC-0
 size:   24
 mapped: 15
 flags:  0x00000003
            IRQ_DOMAIN_FLAG_HIERARCHY
            IRQ_DOMAIN_NAME_ALLOCATED
```

```txt
name:   PCI-MSIX-0000:00:02.0-12
 size:   0
 mapped: 5
 flags:  0x00000213
            IRQ_DOMAIN_FLAG_HIERARCHY
            IRQ_DOMAIN_NAME_ALLOCATED
            IRQ_DOMAIN_FLAG_MSI
            IRQ_DOMAIN_FLAG_MSI_DEVICE
```

暂时无法理解这里都是什么东西。

## 仔细解析这个东西

实际上真的起作用的是:
irq_matrix_debug_show

```txt
Online bitmaps:       32
Global available:   6414
Global reserved:      11
Total allocated:      20
System: 38: 0-19,21,50,236,240-244,246-255
 | CPU | avl | man | mac | act | vectors
     0   196     1     1    6  33-37,39
     1   193     1     1    9  32-38,40-41
     2   201     1     0    0
     3   201     1     0    0
     4   201     1     0    0
     5   201     1     0    0
     6   201     1     0    0
     7   201     1     0    0
     8   201     1     0    0
     9   200     1     0    1  33
    10   201     1     0    0
    11   201     1     0    0
    12   201     1     0    0
    13   201     1     0    0
    14   201     1     0    0
    15   201     1     0    0
    16   200     1     0    1  33
    17   201     1     0    0
    18   200     1     0    1  33
    19   201     1     0    0
    20   201     1     0    0
    21   200     1     0    1  33
    22   201     1     0    0
    23   201     1     0    0
    24   201     1     0    0
    25   201     1     0    0
    26   200     1     0    1  33
    27   201     1     0    0
    28   201     1     0    0
    29   201     1     0    0
    30   201     1     0    0
    31   201     1     0    0
```

但是这个为什么不能绑定所有的中断啊
```txt
@[
    matrix_alloc_area.constprop.0+1
    irq_matrix_alloc+163
    assign_vector_locked+156
    apic_set_affinity+96
    msi_set_affinity+117
    irq_do_set_affinity+206
    irq_move_masked_irq+151
    __irq_move_irq+63
    apic_ack_edge+87
    handle_edge_irq+125
    __common_interrupt+62
    common_interrupt+128
    asm_common_interrupt+38
    pv_native_safe_halt+15
    default_idle+19
    default_idle_call+48
    do_idle+437
    cpu_startup_entry+41
    start_secondary+247
    common_startup_64+318
]: 4
```

```sh
for i in /proc/irq/* ; do
        echo 11 | sudo tee $i/smp_affinity_list
done
```

cat /sys/kernel/debug/irq/domains/VECTOR

但是真的有 12 个中断绑定到这里了:
```txt
sudo cat /proc/irq/*/smp_affinity_list | grep 11 | wc -l
12
```

那么如何理解?

使用 /sys/kernel/debug/irq/irqs 对照
1. grep  Vector * 发现恰好有一个是没有 Vector 的
2. 有好几个的 Vector 都是 0 ，他们在 /proc/interrupts 中没有出现
过的
```txt
handler:  handle_level_irq
device:   (null)
status:   0x00000100
istate:   0x00004000
ddepth:   1
wdepth:   0
dstate:   0x00032000
            IRQD_LEVEL
            IRQD_IRQ_DISABLED
            IRQD_IRQ_MASKED
node:     0
affinity: 0-31
effectiv:
pending:
domain:
 hwirq:   0x0
 chip:    XT-PIC
  flags:   0x0
```
所以就是这样的，如果可以修改，那么就是必须可以修改

## 使用 CONFIG_GENERIC_IRQ_DEBUGFS 来观察

感觉相当有趣: /sys/kernel/debug/irq/irqs


## 问题
- DMAR-MSI 是什么鬼?
- 似乎没有看到 lapic 在 /sys/kernel/debug/irq/irqs 中
- docs/kernel/int-overview.md 中提到的 matrix ，似乎现在是完全看不到了
- domain 名称中 : PCI-MSIX-0000:00:0b.0-12 中的 -12 是什么意思?

## domain 中的 default
```c
static struct irq_domain *irq_default_domain;
```
在 x86 中，这个其实就是 vector domain
irq_matrix_debug_show


## 通过 debugfs 可以直接触发的

测试一下吧
```c
static ssize_t irq_debug_write(struct file *file, const char __user *user_buf,
			       size_t count, loff_t *ppos)
```

## 通过 irq debugfs 是可以手动注入中断
<!-- d6371db9-5c04-4dc6-b62a-ec8654d118ae -->

这个就是通过硬件触发的 CONFIG_GENERIC_IRQ_INJECTION ，会走 idt 通道的

这个需要中断控制器的配合，这么想的话，现在可以有灵活的方法
来控制软中断（注册 timer）和硬中断（如何注册还是一个小问题）

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
