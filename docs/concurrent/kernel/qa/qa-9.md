## kmap 和 kmap_atomic

这个不容易:

先看看这个文档:
- Documentation/translations/zh_CN/mm/highmem.rst
- https://zhuanlan.zhihu.com/p/637695640

```c
static inline void *kmap_atomic(struct page *page)
{
    // 经过了简化的代码
    preempt_disable();
    pagefault_disable();
    return page_address(page);
}

static inline void *kmap(struct page *page)
{
	might_sleep();
	return page_address(page);
}
```

唯一的区别是此时另一个 CPU 是否可以触发 page fault:
```txt
#0  pagefault_disabled () at ./include/linux/uaccess.h:250
#1  do_user_addr_fault (regs=0xffffc900015fff58, error_code=7, address=140063241728256) at arch/x86/mm/fault.c:1292
#2  0xffffffff822a9ad3 in handle_page_fault (address=140063241728256, error_code=7, regs=0xffffc900015fff58) at arch/x86/mm/fault.c:1534
#3  exc_page_fault (regs=0xffffc900015fff58, error_code=7) at arch/x86/mm/fault.c:1590
#4  0xffffffff82401286 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```
pagefault_disabled 中:
```c
	/*
	 * If we're in an interrupt, have no user context or are running
	 * in a region with pagefaults disabled then we must not take the fault
	 */
	if (unlikely(faulthandler_disabled() || !mm)) {
		bad_area_nosemaphore(regs, error_code, address);
		return;
	}
```

通过一个简单的数值增加来:
```c
/*
 * These routines enable/disable the pagefault handler. If disabled, it will
 * not take any locks and go straight to the fixup table.
 *
 * User access methods will not sleep when called from a pagefault_disabled()
 * environment.
 */
static inline void pagefault_disable(void)
{
	pagefault_disabled_inc();
	/*
	 * make sure to have issued the store before a pagefault
	 * can hit.
	 */
	barrier();
}
```
使用 kmap_atomic 的时候，将会禁止 page fault handler

不太理解的问题是:
1. 为什么需要 kmap_atomic 的常见需要 disable page fault handler
  - 检查 pagefault_disable 的位置，还是很多的
2. pagefault_disable 中使用的 barrer 可以优化一下为函数吗? 例如 smp_read_barrier
3. 为什么 pagefault_disabled_inc 只是简单的 ++ ，而不是 atomic 的

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
