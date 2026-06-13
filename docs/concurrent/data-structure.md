# 并发数据结构 readerwriterqueue 分析
https://github.com/cameron314/readerwriterqueue
https://github.com/cameron314/concurrentqueue
  - https://moodycamel.com/blog

## 各种数据结果的实现
https://github.com/XiangpengHao/congee
开发中:
https://github.com/greg7mdp/parallel-hashmap

## ring buffer
- https://news.ycombinator.com/item?id=20096946
  - The design and implementation of a lock-free ring-buffer with contiguous reservations
  - 好像 dpdk 中也是提到过这个事情的

和 queue 算是同一个东西吗? 显然不是一个东西了。


## stack
https://en.wikipedia.org/wiki/Treiber_stack

http://www.inf.ufsc.br/~joao.dovicchi/pos-ed/pos/artigos/p206-hendler.pdf

## queue
Michael-Scott 无锁队列

## 那些数据结构是可以并行的，那些是数据结构没办法?

内核中那些数据结构是并行的，那些不是?
似乎一般都不是的

1. 红黑树可以并行访问吗?
应该是不行的

例如这个东西:
```c
static struct rb_root bdi_tree = RB_ROOT;
```

## bitmap 库
https://github.com/RoaringBitmap/CRoaring?tab=readme-ov-file#quick-start

但是对我来说，这个实在是太复杂了

## 继续这里的东西
https://quant67.com/post/linux/lockfree-lies.html : 如果有价值，这里的问题基本是最高优先级了

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
