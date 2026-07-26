# 使用 kernel 中调试工具

## KASLR

```c
/*
 * See Documentation/x86/x86_64/mm.txt for a description of the memory map.
 *
 * Be very careful vs. KASLR when changing anything here. The KASLR address
 * range must not overlap with anything except the KASAN shadow area, which
 * is correct as KASAN disables KASLR.
 */
#define MAXMEM      (1UL << MAX_PHYSMEM_BITS)
```

## debug
> 从内核的选项来看，对于 debug 一无所知啊 !
- Extend memmap on extra space for more information on page
- Debug page memory allocations
- Track page owner
- Poison pages after freeing
- Enable tracepoint to track down page reference manipulation
- Testcase for the marking rodata read-only
- Export kernel pagetable layout to userspace via debugfs
- Debug object operations
- SLUB debugging on by default
- Enable SLUB performance statistics
- Kernel memory leak detector
- Stack utilization instrumentation
- Detect stack corruption on calls to schedule()
- Debug VM
- Debug VM translations
- Debug access to per_cpu maps

#### page owner

page owner is for the tracking about who allocated each page.


# Kernel 调试

## 基本按照这个操作
- https://sergioprado.blog/how-is-the-linux-kernel-tested/



- [disassemble with code and line](https://stackoverflow.com/questions/9970636/view-both-assembly-and-c-code)
- [如何增大 dmesg buffer 的大小](https://unix.stackexchange.com/questions/412182/how-to-increase-dmesg-buffer-size-in-centos-7-2)

# 通过插入预防错误的方法实现
```c
dump_page(page, "VM_BUG_ON_PAGE(" __stringify(cond)")");\

void dump_page(struct page *page, const char *reason)
{
	__dump_page(page, reason);
	dump_page_owner(page);
}
EXPORT_SYMBOL(dump_page);
```

> 以后再去慢慢跟踪吧!

## kernel hacking

- `pr_info`
  - 注意 `%px` 来输出指针
- `dump_stack`


- [ ] 这个没有测试过啊
调试内核模块
```sh
cat /proc/modules
objdump -dS --adjust-vma=0xffffffff85037434 vmlinux
```

- 如何 hacking 内核的官方文档:
  - https://www.kernel.org/doc/html/latest/kernel-hacking/index.html
  - https://www.kernel.org/doc/html/latest/trace/index.html#
  - https://www.kernel.org/doc/html/latest/dev-tools/index.html

## memtest
- https://github.com/memtest86plus/memtest86plus

- journalctl -t kernel 展示所有的的内核日志。

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
