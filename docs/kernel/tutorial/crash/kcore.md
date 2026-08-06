# 基于 kcore 的几种内核调试办法

## 物理机

1. crash
2. drgn

## 处理虚拟机
- kvm-dmesg
	- 获取 Guest 内存，然后直接
	- 让 ai 实现了一个 aarch64 的支持: https://github.com/rayylee/kvm-dmesg/pull/6
- qemu :
	- gdb kernel
	- dump 内核，然后使用 crash 分析
- perf kvm

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
