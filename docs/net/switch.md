# switch
## 这也太酷了

- https://news.ycombinator.com/item?id=40866442
  - https://blog.brixit.nl/making-a-linux-managed-network-switch/



## config NET_DSA
[Distributed Switch Architecture, A.K.A. DSA](https://netdevconf.info/2.1/papers/distributed-switch-architecture.pdf)

https://docs.kernel.org/networking/dsa/dsa.html

## config NET_SWITCHDEV

实际上，switchdev 比想象的复杂很多，似乎还和 ovs 有关:

- Documentation/networking/switchdev.rst
- https://lwn.net/Articles/675826/
- net/switchdev/


## qemu 的模拟
qemu/hw/net/rocker/

- [Rocker: switchdev prototyping vehicle](https://people.netfilter.org/pablo/netdev0.1/papers/Rocker-switchdev-prototyping-vehicle.pdf)

可以用 rocker 看看内核中 driver 如何管理 switch 的

需要类似 CONFIG_NET_DSA_REALTEK 吗?

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
