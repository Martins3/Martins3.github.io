### kvmvapic
在 https://lists.gnu.org/archive/html/qemu-devel/2012-02/msg00519.html 描述了大致原理，使用 paravirt 减少 vm exit

guest 需要执行 pc-bios/kvmvapic.bin 的代码来实现和 host 交互
在 vapic_realize 中间，注释掉代码，不添加 kvmvapic.bin 这个 rom 那么就取消掉这个加速了。

## 有趣的 blog
https://www.dmdtech.org/2022/12/10/qemu-windows-nt-4-multiprocessor-hal-crash-fixed/

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
