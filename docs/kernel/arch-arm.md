# ARM 内核相关的
## TODO
- [ ] linux/arch/arm64/kernel/fpsimd.c : 有 2000 多行，但是在 kernel 下一共就是 2w 行，如果理解一下 boom 中 SIMD 的设计，也许对于这个问题有更加深刻的理解吧

## Note
Take advantage of raspberry, qemu and [visual](https://salmanarif.bitbucket.io/visual/index.html)

https://people.kernel.org/linusw/how-the-arm32-kernel-starts : 启动

在 x86 上使用 acpi, 在 arm 上使用 dtb 来描述设备的逻辑关系[^2]

## 可以大致浏览一下这个: https://www.kernel.org/doc/html/latest/arm64/index.html

### https://www.kernel.org/doc/html/latest/arm64/memory.html
- 这个 memory 分布中，其 PCI mmio 的大小相比 x86 小几个数量级了

## GIC-v2
- [ ] [^1]P86 启动一个 arm 操作系统看看

```c
static const struct irq_domain_ops gic_irq_domain_hierarchy_ops = {
	.translate = gic_irq_domain_translate,
	.alloc = gic_irq_domain_alloc,
	.free = irq_domain_free_irqs_top,
};

static const struct irq_domain_ops gic_irq_domain_ops = {
	.map = gic_irq_domain_map,
	.unmap = gic_irq_domain_unmap,
};

/**
 * struct irq_fwspec - generic IRQ specifier structure
 *
 * @fwnode:		Pointer to a firmware-specific descriptor
 * @param_count:	Number of device-specific parameters
 * @param:		Device-specific parameters
 *
 * This structure, directly modeled after of_phandle_args, is used to
 * pass a device-specific description of an interrupt.
 */
struct irq_fwspec {
	struct fwnode_handle *fwnode;
	int param_count;
	u32 param[IRQ_DOMAIN_IRQ_SPEC_PARAMS];
};
```

gic_irq_domain_translate : 通过设备树节点 和 DTS 中的中断信息解码出硬件的中断号 和 中断触发类型

> 猜测 irq_fwspec 描述 device tree 中间中断的描述

比如 [^1]P89 描述了 `interrupt = <0x00 0x01 0x04>`
三者分别代表 中断类型，中断 id, 触发类型, 这三者在 gic_irq_domain_translate 分别得到了解析。
translate 做的事情是将中断 id 转换为 hwirq.

- [ ] 如果 irq domain 是存在嵌套的，irq domain 可以翻译 hwirq 到 virq, 也就是通过 irq domain 实现多级翻译


对称的 acpi 也定义了一套
```c
const struct fwnode_operations of_fwnode_ops = {
	.get = of_fwnode_get,
	.put = of_fwnode_put,
	.device_is_available = of_fwnode_device_is_available,
	.device_get_match_data = of_fwnode_device_get_match_data,
	.property_present = of_fwnode_property_present,
	.property_read_int_array = of_fwnode_property_read_int_array,
	.property_read_string_array = of_fwnode_property_read_string_array,
	.get_name = of_fwnode_get_name,
	.get_name_prefix = of_fwnode_get_name_prefix,
	.get_parent = of_fwnode_get_parent,
	.get_next_child_node = of_fwnode_get_next_child_node,
	.get_named_child_node = of_fwnode_get_named_child_node,
	.get_reference_args = of_fwnode_get_reference_args,
	.graph_get_next_endpoint = of_fwnode_graph_get_next_endpoint,
	.graph_get_remote_endpoint = of_fwnode_graph_get_remote_endpoint,
	.graph_get_port_parent = of_fwnode_graph_get_port_parent,
	.graph_parse_endpoint = of_fwnode_graph_parse_endpoint,
	.add_links = of_fwnode_add_links,
};

static const char *of_fwnode_get_name(const struct fwnode_handle *fwnode)
{
	return kbasename(to_of_node(fwnode)->full_name);
}

struct device_node {
	const char *name;
	phandle phandle;
	const char *full_name;
	struct fwnode_handle fwnode;

	struct	property *properties;
	struct	property *deadprops;	/* removed properties */
	struct	device_node *parent;
	struct	device_node *child;
	struct	device_node *sibling;
#if defined(CONFIG_OF_KOBJ)
	struct	kobject kobj;
#endif
	unsigned long _flags;
	void	*data;
#if defined(CONFIG_SPARC)
	unsigned int unique_id;
	struct of_irq_controller *irq_trans;
#endif
};
```
fwnode 只是一个通用的部分，而 device_node 则是 dtb 的具体的实现。

## code trace

> 调用者是 amba 之类的，在启动期间，通过分析 dbt 来枚举中断映射

- irq_of_parse_and_map
  - of_irq_parse_one : 解析 DTS 文件的中设备属性
  - irq_create_of_mapping
    - of_phandle_args_to_fwspec
    - irq_create_fwspec_mapping
      - irq_domain_alloc_irqs :
        - `__irq_domain_alloc_irqs`
            - irq_domain_alloc_irqs_hierarchy
              - gic_irq_domain_alloc : 调用 gic 的回调函数
                - gic_irq_domain_map
      - irq_create_mapping : 如果所在的 domain 不在 IRQ_DOMAIN_FLAG_HIERARCHY

> - [ ] 从这里分析，可以理解其中的 irq 的映射过程，但是，无法理解层级的概念，或者，多个中断控制器是如何工作的 ?

[^1]: 奔跑吧 linux 内核 第二版 卷 2
[^2]: https://stackoverflow.com/questions/58577825/what-does-fwnode-in-struct-device-do-in-linux-kernel
