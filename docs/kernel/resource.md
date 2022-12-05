# kernel/resource.c

## TODO
1. plka 内容回顾一下
2. https://0xax.gitbooks.io/linux-insides/content/MM/linux-mm-2.html 似乎提供了关键内容啊!

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

## 如何准确的获取 kernel 中 RAM 的空间

/proc/iomem
demicode

最好是，将 NUMA 的 boundary 找出来

ARM 虚拟机中的内容:
```txt
😀  sudo cat /proc/iomem
00000000-03ffffff : LNRO0015:00
04000000-07ffffff : LNRO0015:01
09000000-09000fff : ARMH0011:00
  09000000-09000fff : ARMH0011:00 ARMH0011:00
09020000-09020017 : QEMU0002:00
  09020000-09020017 : fw_cfg_mem
09030000-09030fff : ARMH0061:00
  09030000-09030fff : ARMH0061:00 ARMH0061:00
0a000000-0a0001ff : LNRO0005:00
0a000200-0a0003ff : LNRO0005:01
0a000400-0a0005ff : LNRO0005:02
0a000600-0a0007ff : LNRO0005:03
0a000800-0a0009ff : LNRO0005:04
0a000a00-0a000bff : LNRO0005:05
0a000c00-0a000dff : LNRO0005:06
0a000e00-0a000fff : LNRO0005:07
0a001000-0a0011ff : LNRO0005:08
0a001200-0a0013ff : LNRO0005:09
0a001400-0a0015ff : LNRO0005:0a
0a001600-0a0017ff : LNRO0005:0b
0a001800-0a0019ff : LNRO0005:0c
0a001a00-0a001bff : LNRO0005:0d
0a001c00-0a001dff : LNRO0005:0e
0a001e00-0a001fff : LNRO0005:0f
0a002000-0a0021ff : LNRO0005:10
0a002200-0a0023ff : LNRO0005:11
0a002400-0a0025ff : LNRO0005:12
0a002600-0a0027ff : LNRO0005:13
0a002800-0a0029ff : LNRO0005:14
0a002a00-0a002bff : LNRO0005:15
0a002c00-0a002dff : LNRO0005:16
0a002e00-0a002fff : LNRO0005:17
0a003000-0a0031ff : LNRO0005:18
0a003200-0a0033ff : LNRO0005:19
0a003400-0a0035ff : LNRO0005:1a
0a003600-0a0037ff : LNRO0005:1b
0a003800-0a0039ff : LNRO0005:1c
0a003a00-0a003bff : LNRO0005:1d
0a003c00-0a003dff : LNRO0005:1e
0a003e00-0a003fff : LNRO0005:1f
10000000-3efeffff : PCI Bus 0000:00
  10000000-101fffff : PCI Bus 0000:01
    10000000-10000fff : 0000:01:00.0
  10200000-103fffff : PCI Bus 0000:02
    10200000-10200fff : 0000:02:00.0
  10400000-105fffff : PCI Bus 0000:03
    10400000-10403fff : 0000:03:00.0
      10400000-10403fff : xhci-hcd
  10600000-107fffff : PCI Bus 0000:04
    10600000-10600fff : 0000:04:00.0
  10800000-109fffff : PCI Bus 0000:05
    10800000-10800fff : 0000:05:00.0
  10a00000-10bfffff : PCI Bus 0000:06
  10c00000-10dfffff : PCI Bus 0000:07
    10c00000-10c00fff : 0000:07:00.0
  10e00000-10ffffff : PCI Bus 0000:08
  11000000-111fffff : PCI Bus 0000:09
  11200000-113fffff : PCI Bus 0000:0a
  11400000-115fffff : PCI Bus 0000:0b
  11600000-117fffff : PCI Bus 0000:0c
  11800000-119fffff : PCI Bus 0000:0d
  11a00000-11bfffff : PCI Bus 0000:0e
  11c00000-11dfffff : PCI Bus 0000:0f
  11e00000-11e00fff : 0000:00:01.0
  11e01000-11e01fff : 0000:00:01.1
  11e02000-11e02fff : 0000:00:01.2
  11e03000-11e03fff : 0000:00:01.3
  11e04000-11e04fff : 0000:00:01.4
  11e05000-11e05fff : 0000:00:01.5
  11e06000-11e06fff : 0000:00:01.6
  11e07000-11e07fff : 0000:00:01.7
  11e08000-11e08fff : 0000:00:02.0
  11e09000-11e09fff : 0000:00:02.1
  11e0a000-11e0afff : 0000:00:02.2
  11e0b000-11e0bfff : 0000:00:02.3
  11e0c000-11e0cfff : 0000:00:02.4
  11e0d000-11e0dfff : 0000:00:02.5
  11e0e000-11e0efff : 0000:00:02.6
3f000000-3fffffff : PCI ECAM
40000000-83c12ffff : System RAM
  fa000000-ffffffff : reserved
  100231000-100231fff : reserved
  100270000-10027ffff : reserved
  100280000-10028ffff : reserved
  100290000-10029ffff : reserved
  1002a0000-1002affff : reserved
  1002b0000-1002bffff : reserved
  1002c0000-1002cffff : reserved
  1002d0000-1002dffff : reserved
  1002e0000-1002effff : reserved
  1002f0000-1002fffff : reserved
  100300000-10030ffff : reserved
  100310000-10031ffff : reserved
  100320000-10032ffff : reserved
  100330000-10033ffff : reserved
  100340000-10034ffff : reserved
  100350000-10035ffff : reserved
  100360000-10036ffff : reserved
  100370000-10037ffff : reserved
  100380000-10038ffff : reserved
  100390000-10039ffff : reserved
  1003a0000-1003affff : reserved
  1003b0000-1003bffff : reserved
  1003c0000-1003cffff : reserved
  1003d0000-1003dffff : reserved
  1003e0000-1003effff : reserved
  1003f0000-1003fffff : reserved
  100400000-10040ffff : reserved
  100410000-10041ffff : reserved
  100420000-10042ffff : reserved
  100430000-10043ffff : reserved
  100440000-10044ffff : reserved
  100450000-10045ffff : reserved
  100460000-10046ffff : reserved
  100470000-10047ffff : reserved
  7cbc50000-7cbc50fff : reserved
  7cbe70000-7cdceffff : Kernel code
  7cdcf0000-7ce63ffff : reserved
  7ce640000-7cebeffff : Kernel data
  7cebf1000-7d4f3efff : reserved
  818580000-83b9fffff : reserved
  83ba36000-83ba37fff : reserved
  83ba38000-83c12ffff : reserved
83c130000-83c44ffff : reserved
83c450000-83f72ffff : System RAM
  83c450000-83c450fff : reserved
  83c451000-83c451fff : reserved
  83c452000-83e740fff : reserved
  83e741000-83e741fff : reserved
  83e742000-83e742fff : reserved
  83e743000-83f72ffff : reserved
83f730000-83f7bffff : reserved
83f7c0000-83f7cffff : System RAM
  83f7c0000-83f7cffff : reserved
83f7d0000-83f8effff : reserved
83f8f0000-83fffffff : System RAM
  83f8f0000-83fffffff : reserved
8000000000-ffffffffff : PCI Bus 0000:00
  8000000000-80001fffff : PCI Bus 0000:01
    8000000000-8000003fff : 0000:01:00.0
      8000000000-8000003fff : virtio-pci-modern
  8000200000-80003fffff : PCI Bus 0000:02
    8000200000-8000203fff : 0000:02:00.0
      8000200000-8000203fff : virtio-pci-modern
  8000400000-80005fffff : PCI Bus 0000:03
  8000600000-80007fffff : PCI Bus 0000:04
    8000600000-8000603fff : 0000:04:00.0
      8000600000-8000603fff : virtio-pci-modern
  8000800000-80009fffff : PCI Bus 0000:05
    8000800000-8000803fff : 0000:05:00.0
      8000800000-8000803fff : virtio-pci-modern
  8000a00000-8000bfffff : PCI Bus 0000:06
    8000a00000-8000a03fff : 0000:06:00.0
      8000a00000-8000a03fff : virtio-pci-modern
  8000c00000-8000dfffff : PCI Bus 0000:07
    8000c00000-8000c03fff : 0000:07:00.0
      8000c00000-8000c03fff : virtio-pci-modern
  8000e00000-8000ffffff : PCI Bus 0000:08
  8001000000-80011fffff : PCI Bus 0000:09
  8001200000-80013fffff : PCI Bus 0000:0a
  8001400000-80015fffff : PCI Bus 0000:0b
  8001600000-80017fffff : PCI Bus 0000:0c
  8001800000-80019fffff : PCI Bus 0000:0d
  8001a00000-8001bfffff : PCI Bus 0000:0e
  8001c00000-8001dfffff : PCI Bus 0000:0f
```
