# tun tap

- https://jvns.ca/blog/2022/09/06/send-network-packets-python-tun-tap/
- https://zu1k.com/posts/coding/tun-mode/ : 如何利用 TUN 实现代理，利用 TUN 获取到原始的网络包，然后加以修饰，最后发送出去

[kernel doc](https://docs.kernel.org/networking/tuntap.html) 总结的很好

> Virtual network device can be viewed as a simple Point-to-Point or Ethernet device, which instead of receiving packets from a physical media, receives them from user space program and instead of sending packets via physical media sends them to the user space program.

tuntap 就是一个网卡，只是物理网卡的数据来自于网线，而 tuntap 来于用户态。
所以 tuntap 是给一个 ip 地址

[TUN/TAP 设备浅析(一) -- 原理浅析](https://www.jianshu.com/p/09f9375b7fa7) 中的这个图总结的很好

bridge / ovs 最后总是和 tuntap 设备配合使用的时候，tun 设备也是从用户态接受网络包，只是那个用户态程序是 QEMU

## 基本的调用路径
```txt
#14 [ffffa65531497ca8] tun_do_read at ffffffffc0b06c27 [tun]
#15 [ffffa65531497d38] tun_recvmsg at ffffffffc0b06e34 [tun]
#16 [ffffa65531497d68] handle_rx at ffffffffc0c5d682 [vhost_net]
#17 [ffffa65531497ed0] vhost_worker at ffffffffc0c644dc [vhost]
#18 [ffffa65531497f10] kthread at ffffffff892d2e72
#19 [ffffa65531497f50] ret_from_fork at ffffffff89c0022f
```

## 想不到 TAP 有这个依赖
```txt
  │ Symbol: TAP [=n]
  │ Type  : tristate
  │ Defined at drivers/net/Kconfig:418
  │   Depends on: NETDEVICES [=y] && NET_CORE [=y]
  │ Selected by [n]:
  │   - MACVTAP [=n] && NETDEVICES [=y] && NET_CORE [=y] && MACVLAN [=n] && INET [=y]
  │   - IPVTAP [=n] && NETDEVICES [=y] && NET_CORE [=y] && IPVLAN [=n] && INET [=y]
```

原来依赖关系是这样的:

- IPVLAN -> IPVTAP -> TAP
- MACVLAN -> MACVTAP -> TAP

`Select by [n]` 的含义 :

最后的 module 为:
```txt
CONFIG_MACVLAN=m
CONFIG_MACVTAP=m
CONFIG_IPVLAN=m
CONFIG_IPVTAP=m
```
最后的依赖也是这样的
```txt
./drivers/net/ipvlan/ipvlan.ko
./drivers/net/ipvlan/ipvtap.ko
./drivers/net/macvlan.ko
./drivers/net/macvtap.ko
./drivers/net/tap.ko
```
如果这样的，那么就有点奇怪了，在我们的系统中，其实只是需要 tap 设备就可以了，
但是现在构建内核的时候必须把 CONFIG_MACVLAN 等四个 config 都打开才可以。


## 为什么 clash 有 tun 模式
<!-- 62215c72-1da2-4282-9584-b1bf26954907 -->

https://github.com/wangrongding/clash-kit

问问 kimi ，应该是很简单的了

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
