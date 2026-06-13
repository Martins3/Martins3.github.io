## nvlink

nvlink 不在 kernel 的 src 中:

https://github.com/NVIDIA/open-gpu-kernel-modules/tree/main/src/common/nvlink/kernel/nvlink/core

src/common/nvlink/kernel/nvlink/core/

在有 4060ti 的机器 /proc/kallsyms 上，看到加载了一堆 nvlink 的符号:
```txt
sudo bpftrace -e "kprobe:nvlink_* { @[kstack] = count(); }"
```

## 看看这个
看看实物:
https://www.youtube.com/watch?app=desktop&v=flxBD-YwXmM

将 nvlink 直接将两个 GPU 沟通起来，而不是连 CPU 和 GPU 的。

- https://www.nvidia.com/en-us/design-visualization/nvlink-bridges/
- https://www.nvidia.com/en-us/data-center/nvlink/

原来是实现 GPU 和 GPU 直接的沟通的

## 有趣的扩展
https://www.zhihu.com/question/546809864/answer/3623310119
https://www.zhihu.com/question/546809864/answer/3410442846

所以，只是威胁 PCIe ，而不是替代 PCIe 。

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
