# HugeTLB Surplus Bug

## bug 2

这不用依赖 bug 1

不是
只要让 node 1 中:
1. 分配 100
2. reserve 100
3. echo 0 到 node 1 的  nr_hugepages

然后 node 2 中分配 200

然后向 global nr_hugepages 中 ehco 0 的时候，
那么就可以触发

也就是说，不去踩 hugepage :
```txt
                          Node 0          Node 1            Total
HugePages_Total           200.00          400.00           600.00
HugePages_Free            200.00          400.00           600.00
HugePages_Surp            200.00            0.00           200.00
0
                          Node 0          Node 1            Total
HugePages_Total           100.00          300.00           400.00
HugePages_Free            100.00          300.00           400.00
HugePages_Surp            200.00          200.00           400.00
```

Node 0 上的数值 Srup 比 Total 都要大。
中间的状态为
```txt
HugePages_Total           100.00          300.00           400.00
HugePages_Free            100.00          300.00           400.00
HugePages_Surp            200.00            0.00           200.00
```

## 经过论证，还是不要修改接口吧
说实话，找不到这样的一个证据，现在的问题是?

- https://lore.kernel.org/lkml/1455827801-13082-1-git-send-email-hannes@cmpxchg.org/

- 为什么引入了这个错误的?

- nr_hugepages_show_common
- hugetlb_sysctl_handler_common

从存在这个机制的时候，这个 bug 就存在了，靠！

在 a3437870160cf2caaac6bdd76c7377a5a4145a8c 中:

- hugetlb_sysctl_handler
- nr_hugepages_show

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
