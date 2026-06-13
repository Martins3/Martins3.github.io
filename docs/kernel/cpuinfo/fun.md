# 趣事
## 为什么忽然嵌套虚拟化不能用了

物理机内核为 6.17.7-00001-gfd23f075a322

嵌套打开，但是在虚拟机中 modprobe kvm ，依旧有错误

```txt
[101280.425497] kvm_intel: VMX not supported by CPU 6
[101288.739279] kvm_intel: VMX not supported by CPU 13
[101563.099477] kvm_intel: VMX not supported by CPU 2
```

```txt
[   49.403051] kvm_intel: VMX not supported by CPU 31
```

原来是 qemu 启动参数这个导致的:
```sh
    model="-cpu core2duo"
```

笑死

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
