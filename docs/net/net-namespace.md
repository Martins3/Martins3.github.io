## 网络的 namespace

简单浏览了下 net/core/net_namespace.c，并不是很清楚 namespace 如何可以(优雅的)实现，只是

这里面只能看到 rtnl 相关的代码，但是一个进程发起网络操作，如何知道
在网络栈中体现出来 namespace ，没有搞懂

provide isolation of the system resources associated with networking. Thus, each network namespace has its own network devices, IP addresses, IP routing tables, /proc/net directory, port numbers, and so on.[^1]

[^1]: https://lwn.net/Articles/531114/

### net/core/net_namespace.c
- [ ] network namespace 中到底隔离了什么东西 ?

- https://www.cnblogs.com/bakari/p/10613710.html
  - 这个文章从直接相连，bridge 和 ovs 三个方面说明 network namespaces 的连接

> Network  namespaces  provide  isolation of the system resources associated with networking: network devices, IPv4 and IPv6 protocol
> stacks, IP routing tables, firewall rules, the /proc/net directory (which is a symbolic link to /proc/PID/net), the  /sys/class/net
> directory,  various files under /proc/sys/net, port numbers (sockets), and so on.  In addition, network namespaces isolate the UNIX
> domain abstract socket namespace (see unix(7)).
>
> from `man(2) network_namespaces`

- [ ] 虽然可以找到 `struct netns_ipv4`，但是暂时不能理解 namespace 可以隔离 IPv4

> Linux 的实现中，网络命名空间结构体定义如代码片段 15.3所示。如果
当一个子进程被创建时需要将其放入一个新的网络命名空间内，内核首先会调
用net_alloc创建一个新的net实例，然后调用setup_net初始化该实例中
的字段，包括遍历所有的网络模块进行初始化，比如对 loopback 设备进行初
始化、配置默认路由表等。新分配的网络命名空间中默认只包含一个 loopback
设备，而内核会将后续新分配的网络设备绑定到当前网络命名空间，使得不同
网络命名空间中的进程无法互相访问对方的网络设备。如果一个进程想要改
变自己所在的网络命名空间，需要修改自己所绑定的字段，主要流程与上文一
致。


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
