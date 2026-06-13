# Slub Tools

mm-tools 工具

sudo slabinfo -a

sudo slabinfo -v

sudo slabtop


## 解决几个基础的观测问题

- [ ] debugfs 也是可以提供 slab
  - 目前 debugfs 是空的，不知道为什么
- [ ] slab_sysfs_init


### slabtop
sudo slabtop -s c : 按照 cache size 排序

```txt
 Active / Total Objects (% used)    : 1232179 / 1366242 (90.2%)
 Active / Total Slabs (% used)      : 33611 / 33611 (100.0%)
 Active / Total Caches (% used)     : 121 / 155 (78.1%)
 Active / Total Size (% used)       : 380855.73K / 407754.00K (93.4%)
 Minimum / Average / Maximum Object : 0.01K / 0.30K / 8.31K

  OBJS ACTIVE  USE [OBJ SIZE]  SLABS [OBJ/SLAB] [CACHE SIZE] NAME
408291 313910  76%    0.10K  10469       39     41876K       buffer_head
246330 246330 100%    0.19K   5865       42     46920K       dentry
160283 154516  96%    1.09K   5527       29    176864K       ext4_inode_cache
 60180  50647  84%    0.04K    590      102      2360K       extent_status
 41468  30794  74%    0.57K   1481       28     23696K       radix_tree_node
```

### /proc/slabinfo
```txt
| name            <active_objs> <num_objs> <objsize> <objperslab> <pagesperslab> | tunables <limit> <batchcount> <sharedfactor> | slabdata <active_slabs> <num_slabs> <sharedavail> |
|--------------------------------------------------------------------------------|----------------------------------------------|---------------------------------------------------|
| ovl_inode             45        270        720        45            8          | tunables    0    0    0                      | slabdata      6      6      0                     |
| QIPCRTR                2         39        832        39            8          | tunables    0    0    0                      | slabdata      1      1      0                     |
| i915_dependency     1152       1152        128        32            1          | tunables    0    0    0                      | slabdata     36     36      0                     |
| execute_cb             0          0         64        64            1          | tunables    0    0    0                      | slabdata      0      0      0                     |
| i915_request        4794       4794        640        51            8          | tunables    0    0    0                      | slabdata     94     94      0                     |
| intel_context       1058       1058        704        46            8          | tunables    0    0    0                      | slabdata     23     23      0                     |
| fat_inode_cache       50        123        784        41            8          | tunables    0    0    0                      | slabdata      3      3      0                     |
| fat_cache              1        102         40       102            1          | tunables    0    0    0                      | slabdata      1      1      0                     |
| nf_conntrack        2073       2240        256        32            2          | tunables    0    0    0                      | slabdata     70     70      0                     |
| kvm_async_pf        1080       1080        136        30            1          | tunables    0    0    0                      | slabdata     36     36      0                     |
| dentry              913568     914214      192        42            2          | tunables    0    0    0                      | slabdata  21767  21767      0                     |
```

- active_objs: 目前使用中的 object 数量
- num_objs: 总共能够分配的 object 数量
- objsize: 每个 object 的大小
- objperslab: 每个 slab 可以有多少个 object
- pagesperslab: 每个 slab 对应几个 page

参考 [redhat : 5.2.26. /proc/slabinfo](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/4/html/reference_guide/s2-proc-slabinfo)

```c
static void cache_show(struct kmem_cache *s, struct seq_file *m)
{
	struct slabinfo sinfo;

	memset(&sinfo, 0, sizeof(sinfo));
	get_slabinfo(s, &sinfo);

	seq_printf(m, "%-17s %6lu %6lu %6u %4u %4d",
		   s->name, sinfo.active_objs, sinfo.num_objs, s->size,
		   sinfo.objects_per_slab, (1 << sinfo.cache_order));

	seq_printf(m, " : tunables %4u %4u %4u",
		   sinfo.limit, sinfo.batchcount, sinfo.shared);
	seq_printf(m, " : slabdata %6lu %6lu %6lu",
		   sinfo.active_slabs, sinfo.num_slabs, sinfo.shared_avail);
	slabinfo_show_stats(m, s);
	seq_putc(m, '\n');
}
```

```c
struct slabinfo {
	unsigned long active_objs;
	unsigned long num_objs;
	unsigned long active_slabs;
	unsigned long num_slabs;
	unsigned long shared_avail;
	unsigned int limit;
	unsigned int batchcount;
	unsigned int shared;
	unsigned int objects_per_slab;
	unsigned int cache_order;
};
```

## 如何理解 objperslab 和 pagesperslab
```c
static inline unsigned int oo_order(struct kmem_cache_order_objects x)
{
	return x.x >> OO_SHIFT;
}

static inline unsigned int oo_objects(struct kmem_cache_order_objects x)
{
	return x.x & OO_MASK;
}
```

```txt
#0  calculate_sizes (s=s@entry=0xffff888100042400) at mm/slub.c:4351
#1  0xffffffff813d989d in kmem_cache_open (flags=<optimized out>, s=s@entry=0xffff888100042400) at mm/slub.c:4494
#2  __kmem_cache_create (s=s@entry=0xffff888100042400, flags=<optimized out>) at mm/slub.c:5091
#3  0xffffffff83861eff in create_boot_cache (s=s@entry=0xffff888100042400, name=name@entry=0xffffffff82bdfc8c "kmalloc-32", size=size@entry=32, flags=
flags@entry=4096, useroffset=useroffset@entry=0, usersize=usersize@entry=32) at mm/slab_common.c:653
#4  0xffffffff83861f93 in create_kmalloc_cache (name=name@entry=0xffffffff82bdfc8c "kmalloc-32", size=size@entry=32, flags=<optimized out>, flags@entr
y=0, useroffset=useroffset@entry=0, usersize=usersize@entry=32) at mm/slab_common.c:671
#5  0xffffffff83862048 in new_kmalloc_cache (idx=idx@entry=5, type=type@entry=KMALLOC_NORMAL, flags=flags@entry=0) at mm/slab_common.c:882
#6  0xffffffff8386222c in create_kmalloc_caches (flags=flags@entry=0) at mm/slab_common.c:911
#7  0xffffffff838690aa in kmem_cache_init () at mm/slub.c:5041
#8  0xffffffff8382502f in mm_init () at init/main.c:852
#9  start_kernel () at init/main.c:1005
#10 0xffffffff81000145 in secondary_startup_64 () at arch/x86/kernel/head_64.S:358
```
一次分配 slab 可能需要多个页，数量由 oo_order 确定，而 oo_objects 就是 slab 中分配的。

## 问题 : TODO
1. 远程看看 partial 有无增加
2. chenlei 的特殊观察脚本是什么?

## 问题 2

客户 partial 为什么没有限制成功?

node partial 上限只能限制全空的，除非有大量的半空 slab 的情况

但是，当时 partial 的数量还在增加，所以，是 numa node 的原因吗?
让很多，就算可以用，也不用

## 问题 3

观察 node 4 :
objects         155968 (SO_ALL) 这里有全满的
objects_partial 143424

尤其是 node4 的 cpu 和全满的中放了 12544 个 objects ，这不对吧
需要 196 个 slab

同时发现
cpu_slabs N4 = 6
slabs_cpu_partial 的总计数量远远不止这个数量啊

## 现象
1. slabs_cpu_partial 为什么 page 和 pobject 相同 这说明每一个 page 只有一个 object
仔细看看这个  b47291ef02b0bee85ffb7efd6c336 是什么?

2. N15 的 partial 为 0 ，但是 cpu_slabs 特别大。 而 N4 的 cpu_slabs 很小，只有 6

## 问题 5
在 hygon 虚拟机中可以看到:
```txt
606336  98856  16%    0.12K   9474       64     75792K scsi_sense_cache
143360 107843  75%    0.50K   2240       64     71680K kmalloc-512
```

2025-04-23 记录一个:
```txt
[root@big2 13:37:26 scsi_sense_cache]$ grep . *
aliases:0
align:64
grep: alloc_calls: Function not implemented
cache_dma:0
cpu_partial:30
cpu_slabs:5 N0=5
destroy_by_rcu:0
grep: free_calls: Function not implemented
hwcache_align:1
min_partial:5
objects:98856 N0=6336 N1=2112 N2=2112 N3=14280 N4=6168 N5=6168 N6=6168 N7=6168 N8=6168 N9=6168 N10=6168 N11=6168 N12=6168 N13=6168 N14=6168 N15=6168
object_size:96
objects_partial:8424 N3=8136 N4=24 N5=24 N6=24 N7=24 N8=24 N9=24 N10=24 N11=24 N12=24 N13=24 N14=24 N15=24
objs_per_slab:64
order:1
partial:8061 N3=8049 N4=1 N5=1 N6=1 N7=1 N8=1 N9=1 N10=1 N11=1 N12=1 N13=1 N14=1 N15=1
poison:0
reclaim_account:0
red_zone:0
remote_node_defrag_ratio:100
sanity_checks:0
slabs:9474 N0=99 N1=33 N2=33 N3=8145 N4=97 N5=97 N6=97 N7=97 N8=97 N9=97 N10=97 N11=97 N12=97 N13=97 N14=97 N15=97
slabs_cpu_partial:0(0)
slab_size:128
store_user:0
total_objects:606336 N0=6336 N1=2112 N2=2112 N3=521280 N4=6208 N5=6208 N6=6208 N7=6208 N8=6208 N9=6208 N10=6208 N11=6208 N12=6208 N13=6208 N14=6208 N15=6208
trace:0
usersize:96
```
需要看看 parital 最近是不是有增长的。



## usersize 如何理解?

## slab 字段说明
<!-- 3e15bd54-1b41-44aa-950f-0bde4a48b118 -->

partial 相关:
- cpu_partial : kmem_cache::cpu_partial
- min_partial : kmem_cache::min_partial
- slabs_cpu_partial : 每一个 CPU 中的 objects 和 slabs ，其中 objects 没啥意义，所以只用看括号里面的就可以。

```c
enum slab_stat_type {
	SL_ALL,			/* All slabs */
	SL_PARTIAL,		/* Only partially allocated slabs */
	SL_CPU,			/* Only slabs used for cpu caches */
	SL_OBJECTS,		/* Determine allocated objects not slabs */
	SL_TOTAL		/* Determine object capacity not slabs */
};
```

| 接口              | flags                 | 特殊说明                                        |
|-------------------|-----------------------|-------------------------------------------------|
| `objects`         | SO_ALL SO_OBJECTS     |                                                 |
| `objects_partial` | SO_PARTIAL SO_OBJECTS | node partial slab 中有多少个已经被使用的 object |
| `partial`         | SO_PARTIAL            | 每一个 node 上有多个 partial slab               |
| `slabs`           | SO_ALL                |                                                 |
| `cpu_slabs`       | SO_CPU                | 一个 CPU parital 有多个 slab                    |
| `total_objects`   | SO_ALL SO_TOTAL       |

都是调用 show_slab_objects ，flags 分为两组，结果为:
```txt
SO_ALL : SO_PARTIAL + SO_CPU + 全满的
SO_PARTIAL : node partial
SO_CPU : cpu + cpu partial

SO_OBJECTS : slab->inuse
SO_TOTAL : slab->objects
两个 flag 都没有，那么就是统计 slab
```

## 全满 slab 会被谁统计到

从 SO_ALL 的统计看，就是这个字段了:
```c
static inline unsigned long node_nr_objs(struct kmem_cache_node *n)
{
	return atomic_long_read(&n->total_objects);
}
```


## 为什么 slab_size 和 object_size 是一样的

注意  slab_size 不是，而是 kmem_cache::size 和 slub 的区别只有 metadata 而已
```c
struct kmem_cache {
  // ...
	unsigned int size;		/* Object size including metadata */
	unsigned int object_size;	/* Object size without metadata */
  // ...
```

```c
static inline size_t slab_size(const struct slab *slab)
{
	return PAGE_SIZE << slab_order(slab);
}
```


## debugfs

如下函数会创建 : /sys/kernel/debug/slab

```c
static int __init slab_debugfs_init(void)
{
	struct kmem_cache *s;

	slab_debugfs_root = debugfs_create_dir("slabmmm", NULL);

	list_for_each_entry(s, &slab_caches, list)
		if (s->flags & SLAB_STORE_USER)
			debugfs_slab_add(s);

	return 0;

}
```
看上去是因为没有 SLAB_STORE_USER 的 flag ，所以 /sys/kernel/debug/slab 这个目录是空的


而 /sys/kernel/slab/ 是通过如下的内容创建
```c
#define SLAB_ATTR_RO(_name) \
	static struct slab_attribute _name##_attr = __ATTR_RO_MODE(_name, 0400)
```

## 关于 kdump 机制的

在 kfree 中都是可以解释的

### crash bt -FF 是如何知道一个指针所在 kmem_cache 的

### 为什么可以直接探测到是那个进程在使用 slab
```txt
crash> kmem -s ffff918c35453fe0
CACHE             OBJSIZE  ALLOCATED     TOTAL  SLABS  SSIZE  NAME
ffff918d34fe24c0      576        101       224      4    32k  radix_tree_node(167:systemd-journald.service)
  SLAB              MEMORY            NODE  TOTAL  ALLOCATED  FREE
  fffff2a500d51400  ffff918c35450000     0     56         17    39
  FREE / [ALLOCATED]
  [ffff918c35453fe0]
```

## [ ] 不知道为什么，qemu 都关闭了，这些 cache 也清理不掉
```txt
slabinfo - version: 2.1
# name            <active_objs> <num_objs> <objsize> <objperslab> <pagesperslab> : tunables <limit> <batchcount> <sharedfactor> : slabdata <active_slabs> <num_slabs> <sharedavail>
kvm_async_pf         930    930    136   30    1 : tunables    0    0    0 : slabdata     31     31      0
kvm_vcpu             159    171   9280    3    8 : tunables    0    0    0 : slabdata     57     57      0
kvm_mmu_page_header   5060   5236    184   44    2 : tunables    0    0    0 : slabdata    119    119      0
x86_emulator         360    360   2656   12    8 : tunables    0    0    0 : slabdata     30     30      0
```
无论如何，这里的内容分析下。

## debgufs

这里居然什么都没有
```txt
[root@nixos:/sys/kernel/debug/slab]#
```

但是居然是在这里的:
```txt
/sys/kernel/slab
```

- [ ] 这里的文件夹都是怎么产生，可以调查一下



使用 dentry 来分析:

`grep "" *`

aliases:0
align:8
cache_dma:0
destroy_by_rcu:0
failslab:0
hwcache_align:0
store_user:0
trace:0
poison:0
red_zone:0
reclaim_account:1
sanity_checks:0

remote_node_defrag_ratio:100

min_partial:5

order:0
object_size:192
slab_size:192
objs_per_slab:21

cpu_partial:120 :

cpu_slabs:63 N0=55 N1=8 # 一个 node 中所有的 cpu 中含有 slab 数量
objects:45109 N0=39842 N1=1781 N2=3486   # in use 的 objects
total_objects:45192 N0=39900 N1=1806 N2=3486 # 所有的
slabs:2152 N0=1900 N1=86 N2=166 # 乘以 21 ，和 total_objects 数值对应

objects_partial:190 N0=89 N1=101 # 每一个 node 中 partial slab 中正在使用的 objects
partial:13 N0=7 N1=6
  - kmem_cache_node::nr_partial

slabs_cpu_partial:577(55) C0=63(6) C1=73(7) C2=115(11) C3=10(1) C4=84(8) C5=84(8) C6=31(3) C7=115(11)
  - 从 kmem_cache::cpu_slab::partial::slabs 中读取出来的数值

## debug 参数
- parse_slub_debug_flags


## 测试下这个

gdb
```txt
$ lx-slabtrace
Unrecognized command
Usage: lx-slabtrace --cache_name [cache_name] [Options]
    Options:
        --alloc
            print information of allocation trace of the allocated objects
        --free
            print information of freeing trace of the allocated objects
    Example:
        lx-slabtrace --cache_name kmalloc-1k --alloc
        lx-slabtrace --cache_name kmalloc-1k --free
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
