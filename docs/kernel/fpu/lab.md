# 测试

配套代码 code/src/hpc

## cat /proc/cpuinfo 和 fpu 相关的 cpuid 都有哪些
<!-- a74ccd7e-1adb-4678-8456-691157a97a4f -->

fpu
fxsr : fpu 相关的保存

xsave
xsaveopt
xsavec
xsaves

xgetbv1

avx / avx2 / avx_vnni

AVX
 └── AVX2（扩展整数和 gather）
      └── AVX-VNNI（在 AVX2 整数基础上加 VNNI 指令）

fma : Fused Multiply-Add

sse / sse2 / sse4_1 / sse4_2

3dnowprefetch

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
