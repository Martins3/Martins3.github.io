## 关于 clash 代理的两个基本问题
<!-- beb92ea7-3927-4784-a906-a3ff81ca23e7 -->


### 系统代理到底是如何实现的?

它来自 @mihomo-party/sysproxy。lib/sysproxy.js:1
也就是说，这个项目自己不直接写 macOS/Windows/Linux 的系统设置细节，而是把“改系统
代理”这件事外包给这个跨平台库。

也就是基本实现是在这里的:
https://github.com/mihomo-party-org/sysproxy-node


1. macOS
   sysproxy-rs 用的是系统命令 networksetup，对每个 network service 执行：

- -setwebproxy
- -setsecurewebproxy
- -setsocksfirewallproxy
- -set...state on/off
- -setproxybypassdomains

也就是说，它本质是在改 macOS 网络偏好设置，不是抓包，不是 VPN。
来源：https://raw.githubusercontent.com/mihomo-party-org/sysproxy-rs/master/src/
macos.rs
(https://raw.githubusercontent.com/mihomo-party-org/sysproxy-rs/master/src/macos.rs)

2. Windows
   sysproxy-rs 用的是 WinInet 的 InternetSetOptionW，设置的是：

- INTERNET_PER_CONN_FLAGS
- INTERNET_PER_CONN_PROXY_SERVER
- INTERNET_PER_CONN_PROXY_BYPASS

同时读状态时会看注册表 HKCU\Software\Microsoft\Windows\CurrentVersion\Internet
Settings。
所以本质上是改 Windows 的 Internet Settings。
来源：https://raw.githubusercontent.com/mihomo-party-org/sysproxy-rs/master/src/
windows.rs
(https://raw.githubusercontent.com/mihomo-party-org/sysproxy-rs/master/src/windows.rs)

3. Linux
   sysproxy-rs 分桌面环境：

- GNOME 等：调用 gsettings
- KDE：改 kioslaverc，通过 kreadconfig/kwriteconfig

所以它也不是统一的“Linux 内核级代理”，而是桌面环境级的系统代理配置。
来源：https://raw.githubusercontent.com/mihomo-party-org/sysproxy-rs/master/src/
linux.rs
(https://raw.githubusercontent.com/mihomo-party-org/sysproxy-rs/master/src/linux.rs)



### clash 模式
clash 的 tun 模式，本质上是：

在系统里创建一个虚拟网卡，把原本要发到真实网卡的 IP 包先截进 Clash，再由 Clash
决定这些流量怎么转发。

可以从 4 层来理解。

1. TUN 是什么
TUN 是操作系统提供的一种三层虚拟设备，处理的是 IP 包，不是二层以太网帧。

系统把它当成一张网卡，比如 utun、tun0。
一旦路由表把流量指向这张虚拟网卡，应用发出的 TCP/UDP 包就会先进这个设备，而不是
直接出去。

2. Clash 在做什么
Clash 开启 tun 后，通常会做两件事：

1. 创建 TUN 虚拟网卡
2. 修改系统路由 / 策略路由，让流量进入这张网卡

于是路径变成：

应用程序 -> 内核协议栈 -> TUN 虚拟网卡 -> Clash -> 代理节点 / 直连 -> 目标网站

也就是说，Clash 不再只是“等应用主动连本地 socks/http 代理”，而是主动接管系统流
量。

3. 为什么它比普通代理更“透明”
普通 http/socks 模式要求应用自己支持代理。
但很多程序不认代理，或者只认部分代理。

tun 模式下，应用根本不知道自己在走代理，它只是正常发包；
是系统把包送进 TUN，Clash 再“接住”这些包并做代理决策。

所以 tun 模式的核心价值是：

- 不依赖应用是否支持代理
- TCP/UDP 都更容易统一接管
- 对游戏、命令行工具、部分系统组件更有效

4. Clash 怎么把“包”变成“代理连接”
TUN 收到的是原始 IP 包，但代理节点通常需要的是连接语义，比如：

- 目标 IP/端口
- 是 TCP 还是 UDP
- 域名是什么

Clash 会做这些事：

- 解析 IP 包头，拿到目标地址和端口
- 对 TCP：重建/接管连接，再通过代理协议转发
- 对 UDP：做 UDP 转发，或者按代理协议封装
- 如有需要，结合 DNS 映射，把目标 IP 反查回域名用于规则匹配

所以你可以把它理解成：

TUN 负责“截流”，Clash 负责“理解流量并按规则转发”。

5. TUN 和 redir / tproxy 的区别
常见区别是：

- tun：从“虚拟网卡”层接管，看到的是 IP 包，兼容性通常更强
- redir：靠防火墙把 TCP 重定向到本地端口，更多是端口级劫持
- tproxy：更适合透明代理 UDP/TCP，但配置更依赖内核和防火墙能力

简化理解：

- http/socks：应用主动配合
- redir/tproxy：防火墙截流
- tun：虚拟网卡截流

6. 为什么 tun 模式经常还要配 DNS
因为规则常常按域名匹配，比如 google.com 走代理，intranet.local 直连。
但 TUN 拿到的往往只是目标 IP。

所以 Clash 常配合自己的 DNS 系统：

- 接管 DNS 请求
- 记录“某个域名解析成了哪个 IP”
- 后续看到发往这个 IP 的连接时，就知道原始域名是谁

这就是很多实现里所谓的 fake-ip / enhanced mode 背后的目的之一。

7. 一句话总结
Clash 的 tun 模式不是“代理端口”，而是：

通过虚拟网卡接管系统 IP 流量，再把这些流量按规则转成直连或代理转发。

### 当前机器上的实际路由变化

在这台机器上开启 Clash TUN 之后，最重要的变化不是直接改写 `main` 主路由表，而是：

1. 创建 `Meta` 设备
```txt
25: Meta: <POINTOPOINT,MULTICAST,NOARP,UP,LOWER_UP> mtu 9000 qdisc fq_codel state UNKNOWN group default qlen 500
    link/none
    inet 198.18.0.1/30 brd 198.18.0.3 scope global Meta
       valid_lft forever preferred_lft forever
    inet6 fdfe:dcba:9876::1/126 scope global
       valid_lft forever preferred_lft forever
    inet6 fe80::f165:8e27:69c5:eea2/64 scope link stable-privacy proto kernel_ll
       valid_lft forever preferred_lft forever
```
2. 新增策略路由规则 `ip rule`
3. 新增单独的路由表 `table 2022`

也就是说，真实出口网卡和主路由通常还保留，但大部分流量会被策略路由提前导到 `Meta`。

先看主路由表和 TUN 路由表的差别：

```txt
$ ip route show table main
default via 192.168.2.1 dev wlo1 proto dhcp src 192.168.2.41 metric 600
...

$ ip route show table 2022
default via 198.18.0.2 dev Meta
```

这说明：

- 原本的真实默认路由还在，仍然走 `wlo1`
- Clash 额外建立了 `table 2022`
- `table 2022` 的默认下一跳是 `198.18.0.2 dev Meta`

所以它不是简单地把“系统默认路由”从 `wlo1` 改成 `Meta`，而是通过策略路由把流量送进 `table 2022`。

对应的 `ip rule` 可以看到这种“提前分流”的逻辑：

```txt
$ ip rule show
0:      from all lookup local
...
5270:   from all lookup 52
9000:   from all to 198.18.0.0/30 lookup 2022
9001:   not from all dport 53 lookup main suppress_prefixlength 0
9001:   from all iif Meta goto 9010
9002:   not from all iif lo lookup 2022
9002:   from 0.0.0.0 iif lo lookup 2022
9002:   from 198.18.0.0/30 iif lo lookup 2022
9010:   from all nop
32766:  from all lookup main
32767:  from all lookup default
```

可以把它粗略理解成：

- 大部分普通流量会优先匹配到 Clash 增加的规则，再查 `table 2022`
- 查到 `table 2022` 后，默认下一跳就是 `Meta`
- 来自 `Meta` 的流量会跳过这套规则，避免再次送回 `Meta` 形成回环
- DNS 和本地直连相关流量有额外例外规则

从最终结果看会更直观：

```txt
$ ip route get 1.1.1.1
1.1.1.1 via 198.18.0.2 dev Meta table 2022 src 198.18.0.1 uid 1000
    cache

$ ip -6 route get 2606:4700:4700::1111
2606:4700:4700::1111 from :: via fdfe:dcba:9876::2 dev Meta table 2022 src fdfe:dcba:9876::1 metric 1024 pref medium
```

这说明对于普通公网目标，最终选路已经不是：

`应用 -> main -> wlo1`

而是：

`应用 -> ip rule -> table 2022 -> Meta -> Clash -> 真实出口`

补充一点：当前机器里还能看到 `table 52` 和 `fwmark 0x80000/0xff0000` 相关规则，这看起来是 `tailscale` 注入的策略路由，不是 Clash TUN 本身的核心规则。

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
