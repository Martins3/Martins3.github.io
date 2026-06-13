# Cache Coherence 机制

- ccNUMA 中如何设计的?
- 只有一个 NUMA 节点的时候的设计。

## 和 cache coherence 的关系是什么
- [ ] 可以周的总结搞过来看看

- [ ] cache 中的三级 cache 同步的时候，需要每一个层级都同步吗?
- [ ] 可不可以将 load queue / store queue 也是作为 3 级 cache 中的一种方法

- 浅谈多核系统的缓存一致性协议与非均一缓存访问 : https://zhuanlan.zhihu.com/p/162099300

## 其实，cache coherence 到底提供的保证是什么?

## 忽然意识到: cache coherence 关于设备是两个问题: mmio 和 DMA

## 简单思考一个问题
中断的时候，需要把 write buffer 都 flush 掉吗?
似乎也不需要

一个 CPU 在执行

a = 1
b = 1

如果中断发生在该 CPU ，那就是顺序执行了，
如果中断发生在其他 CPU ，显然还是可以观察到乱序的


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
