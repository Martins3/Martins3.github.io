## GVE

在 drivers/net/ethernet/google/ 中可以看到 GCP 下的网卡驱动:

- https://github.com/GoogleCloudPlatform/compute-virtual-ethernet-linux
- https://cloud.google.com/compute/docs/networking/using-gvnic?hl=zh-cn : 基本使用
- https://doc.dpdk.org/guides/nics/gve.html : GVE 甚至可以在 dpdk 中运行

## mana
drivers/net/ethernet/microsoft/mana/

https://learn.microsoft.com/en-us/azure/virtual-network/accelerated-networking-mana-overview

不太确定，是不是可以用物理网卡，还是和 GVE 一样。

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
