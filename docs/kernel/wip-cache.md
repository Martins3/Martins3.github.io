# 总结一下 cache 技术

基本的考虑点

## cache 容量，查询，替换算法，一致性

https://news.ycombinator.com/item?id=44414630

https://github.com/foyer-rs/foyer

数据库中有没有 cache

## 哪些 cache

1. CPU 中的 L1 L2 L3
  - TLB
2. 硬盘中的 FTL
3. linux 内核中的
  - page cahce
  - swap
  - bacahefs 之类的
  - nfs 的本地 cache
4. 使用 ssd 作为 hdd 的 cache
5. CDN (system admin) ，这就不懂了
  - https://news.ycombinator.com/item?id=44352430
  - https://github.com/binhnguyennus/awesome-scalability
  - https://leetsys.dev/
6. 数据库中的 cache ，不懂
7. 编译器中的 cache
  - qemu 二进制翻译中生成的代码
  - 我估计其他的 jit 技术中应该有类似的技术才对

类似这种的论文合集应该很多吧
《cache替换策略》 - ppo丶n的文章 - 知乎
https://zhuanlan.zhihu.com/p/1897326750079377898

连量化的章节中的东西都没看懂，就在这里班门弄斧，可笑可笑

## 关于 tlb 我知道的所有的东西

vhost 中 iotlb

iommu

tlb flush 的问题就到 docs/kvm/mmu/tlb-flush.md

## shadow page table 也是一种 cache 了

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
