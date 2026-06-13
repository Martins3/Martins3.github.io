# HugeTLB Bug List

## 分析下参数解析的过程
```c
/* for command line parsing */
static struct hstate * __initdata parsed_hstate; // 当 hugepagesz= 的时候，参数指向其
static unsigned long __initdata default_hstate_max_huge_pages; // 如果直接遇到了一个 hugepages 没有遇到 hugepagesz，那么会记录数值在这里
static unsigned int default_hugepages_in_node[MAX_NUMNODES] __initdata; // 和 default_hstate_max_huge_pages 配对的，用于记录没有指定 hugepagesz 的时候，大页的大小的
static bool __initdata parsed_valid_hugepagesz = true;
static bool __initdata parsed_default_hugepagesz; // 记录是否存在参数 default_hugepagesz= ，如果没有，那么 hugetlb_init 中将会使用默认数值
```

`default_hstate_max_huge_pages` 和 `default_hugepages_in_node` 的存在是因为之前不知道到底 default sz 是什么，所以只能暂存起来。

1. set_max_huge_pages
2. /proc/sys/vm/nr_hugepages 的逻辑
- [ ] default_hugepagesz_setup 中没有必要处理 parsed_valid_hugepagesz 吧

## 正在确认
hugepages_setup

1. default_hugepagesz 是可以放到各种参数中间的
	- 叠加上 default_hugepagesz_setup 中是可以处理 hugepages 的，感觉问题很大

## hugepagesz_setup 的 check 是做什么的？

```c
		/*
		 * hstate for this size already exists.  This is normally
		 * an error, but is allowed if the existing hstate is the
		 * default hstate.  More specifically, it is only allowed if
		 * the number of huge pages for the default hstate was not
		 * previously specified.
		 */
		if (!parsed_default_hugepagesz ||  h != &default_hstate ||
		    default_hstate.max_huge_pages) {
			pr_warn("HugeTLB: hugepagesz=%s specified twice, ignoring\n", s);
			return 1;
		}
```
主要目的: 相同的 size 不要设置两次。

但是可以豁免的是:

More specifically, it is only allowed if the number of huge pages for the default hstate was not previously specified.

更加简单的逻辑，不警告需要:
1. parsed_default_hugepagesz : 已经设置了 default_hugepagesz
2. h 就是 default_hugesz
3. 但是 default_hstat.max_huge_pages 为 0j

为什么需要如此，因为 `hugetlb_add_hstate` 的时候会增加上 hstate 的。

## hugetlb_init

这个判断好奇怪啊，为什么 or default_hstate_max_huge_pages
```c
	if (!hugepages_supported()) {
		if (hugetlb_max_hstate || default_hstate_max_huge_pages)
			pr_warn("HugeTLB: huge pages not supported, ignoring associated command-line parameters\n");
		return 0;
	}
```

## 理解下 hugepages_clear_pages_in_node 的行为

```c
static void __init hugepages_clear_pages_in_node(void)
{
	if (!hugetlb_max_hstate) {
		default_hstate_max_huge_pages = 0;
		memset(default_hugepages_in_node, 0,
			sizeof(default_hugepages_in_node));
	} else {
		parsed_hstate->max_huge_pages = 0;
		memset(parsed_hstate->max_huge_pages_node, 0,
			sizeof(parsed_hstate->max_huge_pages_node));
	}
}
```

## default_hstate_max_huge_pages 的含义指的是
如果完全没有出现过 hugepagesz= 的时候，hugepages= 参数中数值就是记录在
 default_hstate_max_huge_pages 中。

所以在函数 default_hugepagesz_setup 中需要查询一下，如果发现
default_hstate_max_huge_pages 不为 0，那么说明前面已经有了 hugepages=


## 思考一下模式
假设 A : hugepagesz  B : hugepages
X : default_hugepagesz

正常的: A B A B A B

现在将 B' 和 X 到处放，可以存在如下的组合情况:
- 实际上，B' 只能放到最前面的
- [x] B' A B A B : 就是前面已经指定了 hugepagesz 的，后面重新指定
	- 不行的
- [x] A B B' A B : 不可以和上一个相同
	- hugepagesz=1G hugepages=1 hugepages=200 会如何?
- [x] B' A X B : 这种场景如何考虑
	- 意义不大
- [x] B' A B : A 其实就是 default 的，如何?
	- hugetlb_init 中检查的

## [ ] 考虑还有一个问题，gigantic 的初始化必须在参数解析的时候处理，而不是等到之后处理
但是 gigantic 就是 hugepage 的，那么如何?


## [x] 如果我头从到尾都没有设置 hugepagesz ，那么是最后还是存在 default 是如何设置的

hugetlb_init

```c
	/*
	 * Make sure HPAGE_SIZE (HUGETLB_PAGE_ORDER) hstate exists.  Some
	 * architectures depend on setup being done here.
	 */
	hugetlb_add_hstate(HUGETLB_PAGE_ORDER);
```

hugetlb_init 中设置 default_hstate_idx 默认数值，以及在 default_hugepagesz_setup 中设置。


## 不能使用 nr_hugepages 来替代 max_huge_pages

因为 nr_hugepages 是一个一个增加的，但是至少可以放到 `__initdata` 中的，但是让代码的修改很大。

```txt
#0  __prep_account_new_huge_page (nid=0, h=0xffffffff83a3cf60 <hstates>) at mm/hugetlb.c:1923
#1  prep_new_hugetlb_folio (nid=0, folio=0xffffea00000f8000, h=0xffffffff83a3cf60 <hstates>) at mm/hugetlb.c:1941
#2  alloc_fresh_hugetlb_folio (h=h@entry=0xffffffff83a3cf60 <hstates>, gfp_mask=gfp_mask@entry=3149002, nid=0, nmask=nmask@entry=0xffffffff82f5d178 <node_states+24>, node_alloc_noretry=node_alloc_noretry@entry=0xffff888140826000) at mm/hugetlb.c:2201
#3  0xffffffff813d8c22 in alloc_pool_huge_page (h=h@entry=0xffffffff83a3cf60 <hstates>, nodes_allowed=0xffffffff82f5d178 <node_states+24>, node_alloc_noretry=node_alloc_noretry@entry=0xffff888140826000) at mm/hugetlb.c:2218
#4  0xffffffff835db1e7 in hugetlb_hstate_alloc_pages (h=0xffffffff83a3cf60 <hstates>) at mm/hugetlb.c:3299
#5  0xffffffff835dbb64 in hugetlb_init_hstates () at mm/hugetlb.c:3323
#6  hugetlb_init () at mm/hugetlb.c:4261
#7  0xffffffff81001b7a in do_one_initcall (fn=0xffffffff835db9a0 <hugetlb_init>) at init/main.c:1232
#8  0xffffffff835976ff in do_initcall_level (command_line=0xffff8880049cd600 "root", level=4) at init/main.c:1294
#9  do_initcalls () at init/main.c:1310
#10 do_basic_setup () at init/main.c:1329
#11 kernel_init_freeable () at init/main.c:1546
#12 0xffffffff82164c8a in kernel_init (unused=<optimized out>) at init/main.c:1437
#13 0xffffffff810025b9 in ret_from_fork () at arch/x86/entry/entry_64.S:308
#14 0x0000000000000000 in ?? ()
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
