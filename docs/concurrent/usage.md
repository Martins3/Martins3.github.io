# 并行编程实践记录

1. 热迁移模式?
  - 丢失了，不用怕，由于第二轮可以接受到
2. bitmap 的访问

3. 内部保持一致性，那么 reader 不用关系有多少个 writer


## 猜测 page table 的上锁

madvise(REMOVE) 的时候，如果页面被多个 page table 映射，
当第一个地址空间的 page table 依旧被拆掉了，第二个 page table 可以继续访问，这个是预期的吗?

## 用户态的 bitmap lock 在哪里?

wait_on_bit 内核如何实现的?

用户态该如何实现? 就是上锁，查询 bit 。

需要分配一大堆的 mutex ，然后使用 hash 来分配，真的惨

## 没来得及仔细思考的东西
- A B 共享，A MADV_REMOVE ，如果 B 正好在写
也就是 A B 不去共享 vma 的，但是 A 在MADV_REMOVE 的时候，会去删掉 B 的 page table ，
这里的问题在于什么?

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
