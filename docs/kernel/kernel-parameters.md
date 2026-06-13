# 总结常用的 kernel cmdline

官方文档: https://www.kernel.org/doc/html/latest/admin-guide/kernel-parameters.html


- https://askubuntu.com/questions/716957/what-do-the-nomodeset-quiet-and-splash-kernel-parameters-mean
- [ ] http://happyseeker.github.io/kernel/2018/05/31/about-spectre_v2-boot-parameters.html
- [ ] quiet
- [ ] idle=poll
- [ ] ro

```txt
noibrs noibpb nopti nospectre_v2 nospectre_v1 l1tf=off nospec_store_bypass_disable no_stf_barrier mds=off tsx=on tsx_async_abort=off mitigations=off
```
- 一些解释https://linuxreviews.org/HOWTO_make_Linux_run_blazing_fast_(again)_on_Intel_CPUs
- nomsi

## [ ] kernel cmdline 中 ro 是什么意思

## kernel cmdline 的基本解析过程

启动参数中添加 kvm.foo=1 ，不会影响 kvm 的启动:

```txt
[    1.095583] kvm: unknown parameter 'foo' ignored
```

nvme 中也是如此的:
```txt
[    1.401694] nvme_core: unknown parameter 'abc' ignored
```

但是老版本内核不会容忍。

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
