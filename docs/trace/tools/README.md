# 计划和代办

1. 全局的 cache 工具
2. 把 cache 放到

## 为什么 funcgraph 总是需要屏蔽掉这两个函数，否则就会多出来很多东西

```sh
sudo perf ftrace -G "${entry% \[_\]}" -g 'smp_*' -g irq_enter_rcu -g __sysvec_irq_work irq_exit_rcu
```

## 看看这个东西
https://github.com/wolfpld/tracy

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
