## ipsec 是什么

## 看看基本介绍

就可以回答 openVPN IPsec wirguard 差异是什么? 这个问题了

### [XFRM -- IPsec 协议的内核实现框架](https://segmentfault.com/a/1190000020412259)
内核代码入门。

### [什么是 IPsec？](https://info.support.huawei.com/info-finder/encyclopedia/zh/IPsec.html)

> 按照 VPN 协议分，常见的 VPN 种类有：IPsec、SSL、GRE、PPTP 和 L2TP 等。其中 IPsec 是通用性较强的一种 VPN 技术，适用于多种网络互访的场景。

最后，对比了 ssl 和 ipsec 的方式:

额外的这两个，介绍的一般:
- https://aws.amazon.com/cn/what-is/ipsec
- https://www.cloudflare.com/zh-cn/learning/network-layer/what-is-ipsec/

## xfrm
ipsec 和 xfrm 是什么关系?

虽然有几个文档，但是一般:
- https://docs.kernel.org/networking/xfrm_device.html
- https://docs.kernel.org/networking/xfrm_proc.html
- https://docs.kernel.org/networking/xfrm_sync.html
- https://docs.kernel.org/networking/xfrm_sysctl.html

```txt
ip xfrm state help
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
