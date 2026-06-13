# 网络杂谈

## CONFIG_NETDEVSIM

- https://docs.kernel.org/networking/devlink/netdevsim.html

我靠，我太搞清楚这个东西

## CONFIG_DUMMY

sudo modprobe dummy

- https://unix.stackexchange.com/questions/335284/how-can-we-create-multiple-dummy-interfaces-on-linux
- https://askubuntu.com/questions/1050353/ubuntu-18-04-how-to-create-a-persistent-dummy-network-interface

使用 dummy 创建网桥
http://thomas.goirand.fr/blog/?p=303

说实话，还是没怎么玩清楚。

- https://www.josehu.com/technical/2023/10/28/emulating-network-env.html
  - 这里同时分析过 dummy 和 netsim

## /dev/null 的
- https://superuser.com/questions/698244/ip-address-that-is-the-equivalent-of-dev-null
- https://networkengineering.stackexchange.com/questions/7456/what-is-null-0-interface

## netem
https://man7.org/linux/man-pages/man8/tc-netem.8.html

具体实现的位置: net/sched/sch_netem.c

```sh
tc qdisc add dev eth0 root netem delay 100ms
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
