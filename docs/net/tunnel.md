## linux 的 tunnel 技术
<!-- 49befc97-527c-4626-93fb-16951bde33d4 -->

- [An introduction to Linux virtual interfaces: Tunnels](https://developers.redhat.com/blog/2019/05/17/an-introduction-to-linux-virtual-interfaces-tunnels/)
- [https://en.wikipedia.org/wiki/Tunneling_protocol](https://en.wikipedia.org/wiki/Tunneling_protocol)

net/ipv4/tunnel4.c 中间描述 tunnel 的通用接口吧, 证据在 tunnel4_rcv 中间的 `for_each_tunnel_rcu(tunnel4_handlers, handler)`

## 用户态的工具
- https://github.com/anderspitman/awesome-tunneling : 收集了各种
  - https://github.com/ekzhang/bore : 将本地的网络，代码量非常少，不过使用了 tokio ，不太有兴趣
- https://www.tunnelbear.com

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
