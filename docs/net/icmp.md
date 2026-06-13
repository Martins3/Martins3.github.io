## icmp
位置:
- net/ipv6/icmp.c
- net/ipv4/icmp.c

![](./img/ping.svg)

注意一下其中的:
- `ip_send_skb`
  - `ip_local_out`
    - `__ip_local_out`
      - `nf_hook`  : 调用 netfilter
    - `dst_output` : 发送位置

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
