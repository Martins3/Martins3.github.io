tracer 只能在内核中直接修改

这些修改直接放到
```txt
git apply trace/extra.diff
cp trace/trace_martins3.c kernel/trace/trace_martins3.c
```

## 基本实验
cat available_tracers

```txt
mytracer blk function_graph function nop
```

echo mytracer > current_tracer

cat trace
```txt
# tracer: mytracer
#
# My Tracer Output
# =================
```

## 最终目的

1. 为什么 blktrace 需要一个单独的模块?

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
