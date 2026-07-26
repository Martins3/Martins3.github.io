# msharefs
<!-- 4ca649d3-99c9-4472-b0bb-faa4be1b2e0d -->

通过 msharefs 来实现多个 process 共享地址空间，现在的系统调用实现约束太大了
需要让共享的 process 都是从同一个 parent clone 出来的:
https://mp.weixin.qq.com/s/OavFbBFanLrLiHQI3aAGow

## 动机

### 映射 1T 需要使用多少页表

仅调用 `mmap()` 只会创建 VMA，不会为整个 1 TiB 地址范围建立页表。测试程序
`mmap-pgtable.c` 使用以下方式只建立页表，避免分配 1 TiB 匿名内存：

1. 使用 `PROT_READ | MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE` 映射；
2. 使用 `MADV_NOHUGEPAGE`，强制按 4 KiB PTE 建表；
3. 使用 `MADV_POPULATE_READ` 触发只读匿名缺页。只读缺页映射共享 zero page，
   不为每个虚拟页分配独立数据页；
4. 以 `/proc/self/status` 中的 `VmPTE` 增量作为进程页表内存的主要结果，
   并用 `/proc/meminfo` 的 `PageTables` 增量辅助校验。

先用小范围验证：

```bash
gcc -O2 -Wall -Wextra -Werror mmap-pgtable.c -o mmap-pgtable.out
./mmap-pgtable.out 4G
```

再运行 1 TiB 测试：

```bash
./mmap-pgtable.out 1T --hold 0
```

`--hold 0` 会在建表后等待回车，便于从其他终端检查 `/proc/<pid>/status`。
如果无需外部检查，省略该参数，程序打印统计后立即解除映射。

在 4 KiB 基页、x86-64 四级页表上，不考虑映射起始地址未对齐带来的少量边界页，
1 TiB 范围需要约 2 GiB PTE 页、4 MiB PMD 页和 8 KiB PUD 页，预期
`VmPTE` 增量约为 2052 MiB。`VmRSS`/`RssAnon` 不应随 1 TiB 映射线性增长。

本机（x86-64、4 KiB 页、Linux 7.0.13）实测结果：

```text
delta  VmRSS=    +136 KiB  RssAnon=      +4 KiB
       VmPTE=+2101256 KiB  system_PageTables=+2100932 KiB
elapsed=18.059 seconds
```

即该 1 TiB 映射实际增加 `2101256 KiB / 1024 = 2052.01 MiB` 页表内存，
与理论值一致。系统级 `PageTables` 会受到其他进程并发建表、释放页表的影响，
因此最终以当前进程的 `VmPTE` 增量为准。

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
