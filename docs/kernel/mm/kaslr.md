# kaslr

我的问题:
1. 攻击的方法是什么，为什么固定内核虚拟地址就可以避免攻击?
2. 为什么 gdb 调试要求这个不是固定的?
3. 对于那些符号的地址有调整

## Links
- [ ] https://unix.stackexchange.com/questions/469016/do-the-virtual-address-spaces-of-all-the-processes-have-the-same-content-in-thei
  - [ ] https://en.wikipedia.org/wiki/Kernel_page-table_isolation
  - [ ] https://lwn.net/Articles/738975/

- [ ] https://bneuburg.github.io/
  - [ ] he has writen three post about it

- [ ] https://lwn.net/Articles/569635/


- [ ] Sometimes /proc/$pid/maps show text address start at 0x400000, sometimes 0x055555555xxx,
maybe because of user space address randomization
    - [  ] https://www.theurbanpenguin.com/aslr-address-space-layout-randomization/

## 随机结果

地址随机化不是简单的二选一，而是分层决定：

- 地址空间的大框架主要在编译期确定：例如 x86_64 的 4-level/5-level 候选布局、__START_KERNEL_map、__PAGE_OFFSET_BASE_L4/L5、内核映像窗口大小等，定义在
  arch/x86/include/asm/page_64_types.h:33。

- 实际使用 4-level 还是 5-level 是启动期决定的：由 CPU 的 LA57 能力以及启动配置决定，__VIRTUAL_MASK_SHIFT 也因此是运行时表达式。
- KASLR 会改变部分实际基址，但不会重新设计整个 canonical address space 的分区。

x86_64 里要区分三类随机化：

1. Kernel image KASLR

   压缩内核启动时选择一个随机的物理加载地址，然后做 relocation。内核链接虚拟地址 __START_KERNEL_map = 0xffffffff80000000 仍是固定的，通常变化的是内核映
   像对应的物理地址。

2. Memory KASLR

   如果启用 CONFIG_RANDOMIZE_MEMORY，启动早期会随机设置：
    - page_offset_base：direct map
    - vmalloc_base：vmalloc/ioremap
    - vmemmap_base：vmemmap

   代码见 arch/x86/mm/kaslr.c:43，文档也明确说这些区域的顺序保持不变，但 base 会在启动时偏移 Documentation/arch/x86/x86_64/mm.rst:171。

3. 用户进程 ASLR

   每个进程的 mmap、栈、堆、动态库等还会有独立的进程级随机化，这和内核地址空间 KASLR 是另一层。

我用当前内核和 collei virtme 实测了这一点。当前 .config 中：

CONFIG_RANDOMIZE_BASE=y
CONFIG_RANDOMIZE_MEMORY=y

virtme 默认会强制加入 nokaslr，此时通过 guest 内的 drgn 读到：

page_offset_base = 0xffff888000000000
vmalloc_base     = 0xffffc90000000000
vmemmap_base     = 0xffffea0000000000

临时移除 nokaslr 后启动，第一次是：

page_offset_base = 0xffff9b4340000000
vmalloc_base     = 0xffffb88ac0000000
vmemmap_base     = 0xfffffbc3c0000000

再次重启又变成：

page_offset_base = 0xffff905600000000
vmalloc_base     = 0xffffaac640000000
vmemmap_base     = 0xffffcdf700000000

所以结论是：

> 虚拟地址空间的分区规则和边界主要由编译期常量、配置和 4/5-level 模式确定；实际的若干区域基址会在启动期受到 KASLR 影响。

另外，CONFIG_RANDOMIZE_BASE 还会影响 KERNEL_IMAGE_SIZE 的编译期取值：启用时默认 1 GiB，禁用时 512 MiB，见 arch/x86/include/asm/page_64_types.h:72。


sudo drgn a.py 就可以做这些观测了:
```txt
print("page_offset_base =", hex(int(prog["page_offset_base"])))
print("vmalloc_base     =", hex(int(prog["vmalloc_base"])))
print("vmemmap_base     =", hex(int(prog["vmemmap_base"])))
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
