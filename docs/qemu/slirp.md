# qemu slirp
<!-- 4088a268-059b-4272-a14e-84d57be08c5a -->

## 关键文档
- docs/system/devices/net.rst
- https://passt.top/

## 默认模式下，QEMU 是如何保证给一个分配的 10.0.2.15 的

```txt
2: ens5: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 52:54:00:12:34:56 brd ff:ff:ff:ff:ff:ff
    inet 10.0.2.15/24 brd 10.0.2.255 scope global dynamic ens5
       valid_lft 86447sec preferred_lft 86447sec
    inet6 fec0::5054:ff:fe12:3456/64 scope site dynamic mngtmpaddr
       valid_lft 86388sec preferred_lft 14388sec
    inet6 fe80::5054:ff:fe12:3456/64 scope link
       valid_lft forever preferred_lft forever
```

slirp 中自己需要实现一个 nat ，将从 10.0.2.15 的请求进行装换为就像是本地的程序一样.

在 qemu 中 net/slirp.c 可以看到:
```c
    struct in_addr dhcp = { .s_addr = htonl(0x0a00020f) }; /* 10.0.2.15 */
```

## 似乎符合预期，但是也感觉有点不对

在虚拟机执行，如果只是配置一个 10.0.2.15 ，也就是 user 的网卡，那么
```txt
mount.nfs4  10.0.0.2:/home/martins3/hack mnt
```
会失败

但是如果给配置了
```txt

5: ens4: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 52:54:00:00:02:61 brd ff:ff:ff:ff:ff:ff
    inet 10.0.97.0/16 brd 10.0.255.255 scope global noprefixroute ens4
       valid_lft forever preferred_lft forever
    inet6 fe80::878e:704f:ff64:bc04/64 scope link noprefixroute
       valid_lft forever preferred_lft forever
```
那么就自动可以了

## `hostname=` 和 `smb=` 到底是什么

QEMU 文档里把 `hostname=` 和 `smb=` 都定义在 `-netdev user` 上，而不是
`-device virtio-net-*` 上。这说明它们属于 user network backend，也就是 slirp
后端的功能:

```txt
-netdev user,id=id[,option][,option][,...]
```

`qemu-options.hx` 中说明:

- `net=` 默认是 `10.0.2.0/24`
- `host=` 默认是 guest 可见的第 2 个地址，也就是 `10.0.2.2`
- `hostname=` 是内置 DHCP server 报告给 guest 的 client hostname
- `smb=dir[,smbserver=addr]` 会启用一个 SMB server，默认使用 guest 网段里的第 4 个地址，也就是 `10.0.2.4`

`net/slirp.c` 里对应的默认值更直接:

```c
struct in_addr net  = { .s_addr = htonl(0x0a000200) }; /* 10.0.2.0 */
struct in_addr mask = { .s_addr = htonl(0xffffff00) }; /* 255.255.255.0 */
struct in_addr host = { .s_addr = htonl(0x0a000202) }; /* 10.0.2.2 */
struct in_addr dhcp = { .s_addr = htonl(0x0a00020f) }; /* 10.0.2.15 */
struct in_addr dns  = { .s_addr = htonl(0x0a000203) }; /* 10.0.2.3 */
```

`smb=` 的实现不在 guest 网卡设备里，而是在 `net/slirp.c:slirp_smb()`:

- 先检查编译时配置的 `CONFIG_SMBD_COMMAND`
- 检查 `smb=` 指定目录是否可读可进入
- 创建临时 `smb.conf`
- share 名固定是 `[qemu]`
- `path=` 是 `smb=` 传入的 host 目录
- `guest ok=yes`
- 通过 `slirp_add_exec()` 把 server 地址的 139 和 445 端口接到 `smbd`

### smb guest 侧测试

```txt
mount -t cifs //10.0.2.4/qemu /tmp/qemu-slirp-smb-test -o guest,ro,vers=3.0

TARGET                   SOURCE          FSTYPE OPTIONS
/tmp/qemu-slirp-smb-test //10.0.2.4/qemu cifs   ro,relatime,vers=3.0,...
```

挂载后可以看到 host 的 `/home/martins3/` 内容，随后已卸载:

```txt
mount ok
umount ok
```

## passt
<!-- b7c00708-423a-4661-ac6a-9f6061627395 -->

  QEMU: sv[0] ←→ sv[1]: passt fd 3

  参见 net/passt.c:298 和 net/passt.c:243。

- 启动并管理 passt 子进程：
    - 创建临时 PID 文件
    - 执行 passt --pid ... --fd 3
    - 记录 daemon PID
    - QEMU 退出时发送 SIGTERM 并删除 PID 文件

  清理逻辑在 net/passt.c:83。

- 默认的 stream 模式中，在 QEMU 网络包和 socket 数据之间转发。包采用“4 字节网络序长度 + Ethernet frame”的 framing，实际通用收发逻辑在 net/
  stream_data.c。

- 如果 socket 断开，会杀掉旧的 passt 并尝试重新启动，见 net/passt.c:121。
- 可选支持 vhost-user=on。这时要求前端必须是 virtio-net-*，由 passt 通过 vhost-user 直接处理 virtqueue，减少经过 QEMU 数据面的开销。初始化入口是 net/
  passt.c:436。

所以，net/passt.c 本身并不实现 TCP/IP/NAT 协议栈；真正的网络协议转换由外部 passt 进程完成。这个文件主要负责 QEMU 与 passt 之间的配置转换、进程生命周
期、数据通道和 vhost-user 集成。QEMU 文档将它描述为无需特权、IPv6 支持更完整，并且网络进程与 QEMU 隔离的 SLIRP 替代方案

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
