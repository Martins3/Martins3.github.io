# aarch64

## 回答这个问题
https://serverfault.com/questions/1098604/how-to-check-if-kvm-nested-virtualization-is-supported-on-arm64-processor

1. 将 kvm forum 找来看看(如果有)

很遗憾，现在理解还不到位，只是理论上可以打开，但是实际上打不开。

实际上
https://developer.arm.com/documentation/102142/0100/Nested-virtualization
https://www.hisilicon.com/en/products/Kunpeng/Huawei-Kunpeng/Huawei-Kunpeng-920
https://lwn.net/Articles/928426/ : 的确是在 Mac 上测试

## nested 的确不应该有问题
https://apple.stackexchange.com/questions/466761/is-nested-virtualization-is-supported-by-apple-silicon-chips-m2-m3

默认状态下：
```txt
[root@localhost ~]# dmesg | grep kvm
[    7.067028] kvm [1]: HYP mode not available
```

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
