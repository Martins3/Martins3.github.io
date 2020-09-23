# kernel/irq/irqdesc.md

感觉 : 用于支持动态的分配 irq

## TODO
1. struct module 的作用
    1. struct kobject  各种鸡巴结构体
2. 各种 CONFIG
    2. CONFIG_SPARSE_IRQ
    3. CONFIG_HANDLE_DOMAIN_IRQ
3. 

## smp 

```c
static int __init irq_affinity_setup(char *str)
static void __init init_irq_default_affinity(void)


static int alloc_masks(struct irq_desc *desc, int node)
static void free_masks(struct irq_desc *desc)
```
> cpu mask 的功能好神奇 ? 还可以实现什么功能


## alloc_desc
```c
static struct irq_desc *alloc_desc(int irq, int node, unsigned int flags,
				   const struct cpumask *affinity,
				   struct module *owner)
    static void desc_set_defaults(unsigned int irq, struct irq_desc *desc, int node,
                const struct cpumask *affinity, struct module *owner)
```

## show
1. chiq_name_show
2. type_show
3. hwirq_show
4. type_show
5. wakeup_show
6. name_show
7. action_show

## sfsfs


## radix_tree

```c
static void irq_insert_desc(unsigned int irq, struct irq_desc *desc)
{
	radix_tree_insert(&irq_desc_tree, irq, desc);
}

struct irq_desc *irq_to_desc(unsigned int irq)
{
	return radix_tree_lookup(&irq_desc_tree, irq);
}


static void delete_irq_desc(unsigned int irq)
{
	radix_tree_delete(&irq_desc_tree, irq);
}
```



## free desc && alloc desc

```c
static void free_desc(unsigned int irq)

static int alloc_descs(unsigned int start, unsigned int cnt, int node,
		       const struct cpumask *affinity, struct module *owner)
```


## misc

1. 一个简单的辅助函数
```c
/**
 * generic_handle_irq - Invoke the handler for a particular irq
 * @irq:	The irq number to handle
 *
 */
int generic_handle_irq(unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);

	if (!desc)
		return -EINVAL;
	generic_handle_irq_desc(desc);
	return 0;
}
EXPORT_SYMBOL_GPL(generic_handle_irq);
```


```c
/**
 * irq_free_descs - free irq descriptors
 * @from:	Start of descriptor range
 * @cnt:	Number of consecutive irqs to free
 */
void irq_free_descs(unsigned int from, unsigned int cnt)

/**
 * irq_alloc_descs - allocate and initialize a range of irq descriptors
 * @irq:	Allocate for specific irq number if irq >= 0
 * @from:	Start the search from this irq number
 * @cnt:	Number of consecutive irqs to allocate.
 * @node:	Preferred node on which the irq descriptor should be allocated
 * @owner:	Owning module (can be NULL)
 * @affinity:	Optional pointer to an affinity mask array of size @cnt which
 *		hints where the irq descriptors should be allocated and which
 *		default affinities to use
 *
 * Returns the first irq number or error code
 */
int __ref
__irq_alloc_descs(int irq, unsigned int from, unsigned int cnt, int node,
		  struct module *owner, const struct cpumask *affinity)
```

```c
struct irq_desc *
__irq_get_desc_lock(unsigned int irq, unsigned long *flags, bool bus,
		    unsigned int check)

void __irq_put_desc_unlock(struct irq_desc *desc, unsigned long flags, bool bus)
```

## stat

关于 irq 的统计函数

