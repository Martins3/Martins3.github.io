# spp

• 你看到的 SPP-related，通常指 Sub-Page Write Permissions（子页写权限）相关事件。SPP 是 Intel 的虚拟化功能，让 VMM 能以 128 字节为粒度控制 guest
  内存的写权限；普通 EPT 的最小粒度则是 4 KiB。Intel SDM §29.3.4 (https://cdrdv2-public.intel.com/812396/326019-sdm-vol-3c.pdf#page=142)

  举个例子：guest 的一个 4 KiB 页面里，只有某个 128 字节区域存放需要保护的数据，其余区域需要正常写入。

  • 只用普通 EPT：将整页设为不可写，写入该页其他区域也会触发 EPT violation。
  • 使用 SPP：将页面分成 32 个 128 字节子区域，分别设置写权限，让其他区域的正常写入直接通过。

  它的用途就是细粒度内存保护、监控等，避免整页写保护带来的额外拦截。

  具体机制是：VMM 启用 SPP，将目标页配置为 EPT 不可写、4 KiB 映射、EPT PTE 的 bit 61 为 1。对于符合条件的写访问，CPU 再查询单独的 SPP
  权限表，决定对应子区域能否写入。Intel SDM §29.3.4.1–2 (https://cdrdv2-public.intel.com/812396/326019-sdm-vol-3c.pdf#page=143)

  手册里的 “SPP-related event” 有更具体的含义，指以下两类导致 VM exit 的事件：

   事件                    含义
  ━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   SPP miss                查询 SPP 表时，遇到 valid 位为 0 的中间表项
  ──────────────────────  ─────────────────────────────────────────────
   SPP misconfiguration    SPP 表项或权限向量中，保留位被错误置位

  因此，SPP 权限正常查到、但禁止本次写入时，属于 EPT violation；“SPP-related event” 特指上述 SPP 查表缺项或配置错误。Intel SDM §29.3.4
  (https://cdrdv2-public.intel.com/812396/326019-sdm-vol-3c.pdf#page=144)

  另外，这项功能已经走向弃用：Intel 在 2024 年公告中建议 VMM 停用 SPP，并表示未来处理器将移除它。Intel 公告
  (https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/advisory-guidance/sub-page-permission.html)


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
