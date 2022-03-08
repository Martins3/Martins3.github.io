# kernel/resource.c

## TODO
1. plka 内容回顾一下
2. https://0xax.gitbooks.io/linux-insides/content/MM/linux-mm-2.html 似乎提供了关键内容啊!
4. https://lwn.net/Articles/653585/ 实际上，使用不是 mmap 而是 ioremap 完成的工作

## question
1. 动态规划的还是实现定义的
    1. 或者那些内容可以变化，那些需要实现定义一下
    2. 写入的地址是物理地址吗 ?
    3. 这些地址又什么特殊之处吗 ?

2. 利用 allocate_resource ?

## proc
1. /proc/ioports 和 /proc/iomem


```c
static int __init ioresources_init(void)
{
	proc_create_seq_data("ioports", 0, NULL, &resource_op,
			&ioport_resource);
	proc_create_seq_data("iomem", 0, NULL, &resource_op, &iomem_resource);
	return 0;
}

struct resource ioport_resource = {
	.name	= "PCI IO",
	.start	= 0,
	.end	= IO_SPACE_LIMIT,
	.flags	= IORESOURCE_IO,
};
EXPORT_SYMBOL(ioport_resource);

struct resource iomem_resource = {
	.name	= "PCI mem",
	.start	= 0,
	.end	= -1,
	.flags	= IORESOURCE_MEM,
};
EXPORT_SYMBOL(iomem_resource);

// todo 跟踪一下两个位置的初始化位置

static const struct seq_operations resource_op = {
	.start	= r_start,
	.next	= r_next,
	.stop	= r_stop,
	.show	= r_show,
};
```
1. proc 操作上，并没有什么神奇的地方，构成树形的结构，将树形的结构打印出来即可。


## API : request_resource && release_resource

```c
/**
 * request_resource - request and reserve an I/O or memory resource
 * @root: root resource descriptor
 * @new: resource descriptor desired by caller
 *
 * Returns 0 for success, negative error code on error.
 */
int request_resource(struct resource *root, struct resource *new)
{
	struct resource *conflict;

	conflict = request_resource_conflict(root, new);
	return conflict ? -EBUSY : 0;
}

/**
 * release_resource - release a previously reserved resource
 * @old: resource pointer
 */
int release_resource(struct resource *old)
{
	int retval;

	write_lock(&resource_lock);
	retval = __release_resource(old, true);
	write_unlock(&resource_lock);
	return retval;
}

/**
 * request_resource_conflict - request and reserve an I/O or memory resource
 * @root: root resource descriptor
 * @new: resource descriptor desired by caller
 *
 * Returns 0 for success, conflict resource on error.
 */
struct resource *request_resource_conflict(struct resource *root, struct resource *new)
{
	struct resource *conflict;

	write_lock(&resource_lock);
	conflict = __request_resource(root, new);
	write_unlock(&resource_lock);
	return conflict;
}

/* Return the conflict entry if you can't request it */
static struct resource * __request_resource(struct resource *root, struct resource *new)
{
	resource_size_t start = new->start;
	resource_size_t end = new->end;
	struct resource *tmp, **p;

	if (end < start)
		return root;
	if (start < root->start)
		return root;
	if (end > root->end)
		return root;
	p = &root->child;
	for (;;) {
		tmp = *p;
		if (!tmp || tmp->start > end) {
			new->sibling = tmp;
			*p = new;
			new->parent = root;
			return NULL;
		}
		p = &tmp->sibling;
		if (tmp->end < start)
			continue;
		return tmp;
	}
}
```

## devm

需要调用一些 : drivers/base/driver.c
