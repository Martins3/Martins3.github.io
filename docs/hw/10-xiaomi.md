# 2018 年款的小米笔记本

## 性能
测试性能就是勉强比 n100 稍微好一点

为什么编译内核需要 13 分钟了
```txt
vn on  master [+] 🐙 took 13m25s
```

在 linux-build 中 make clean 后重新测试 ，时间为 1m 25s

在 13900k 上
```txt
linux-build on  yyds via 🐍 v3.12.8 via ❄️  impure (yyds-env) 🔥 took 36s
```

不过，所以我认为这个机器有点奇怪了
2025 我发现，记得以前构建 qemu 的速度也是很快的，现在很慢，需要 5m55s

## 特点
这个机器网卡不是接入到 pci 上的，而是在 usb 上的:
```txt
🧀  lsmod | fzf
cdc_ether              24576  1 cdc_ncm
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
