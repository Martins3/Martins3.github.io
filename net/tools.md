# Network tools internals
使用 [Julia Evans](https://wizardzines.com/networking-tools-poster/) 作为基准

- [ ] 在我的印象中，一直都是 ip 和 ifconfig 是不是存在一些冲突啊


## TODO
- [ ] 测试延迟

## iperf
- 测试带宽

## hping3
https://linux.die.net/man/8/hping3

## wrk
测试 http 的性能
https://github.com/wg/wrk

## tcpdump
- [tcpdump](https://jvns.ca/tcpdump-zine.pdf)
- [ ] https://blog.cloudflare.com/bpf-the-forgotten-bytecode/ : bpf, the kernel counterpart of tcpdump

- loopback interface
  - `sudo tcpdump -i lo` : print out many message
- [ ] tcpdump 如何工作的 ?

## nc

https://www.kawabangga.com/posts/4515 中间提到 nc -l  9999 的操作可以了解一下 nc 的含义

## traceroute
- [ ] traceroute

## mtr

主要参考 [使用 mtr 检查网络问题，以及注意事项](https://www.kawabangga.com/posts/4275)

## nslookup

## wireshark
主要参考[这里](https://gaia.cs.umass.edu/kurose_ross/wireshark.php)

- [ ] https://www.kawabangga.com/posts/4794 : 写的真好啊  [ddf]
  - 之前是没有想到 wireshark 还可以宏观的处理网络问题

## ip

## ifconfig

## arp

- 为什么感觉 arp 和 dhcp 存在一些冲突啊?
  - 如果一个网络中，加入一个新的机器的流程
    - 为了获取一个 ip addr, 使用 udp broadcast，此时 destination mac address 是 ff.ff.ff.ff.ff.ff
    - 和 dhcp server 可以获取 ip addr，但是无法知道局域网中每一台机器的 mac addr

## iproute2
https://github.com/shemminger/iproute2

## 网络测试工具
- https://github.com/google/packetdrill : 甚至还有相关的资源

## smap
https://github.com/s0md3v/Smap

## netstat

> This program is obsolete. Replacement for netstat is ss. Replacement for netstat -r is ip route. Replacement for netstat -i is ip -s link. Replacement for netstat -g is ip maddr.

## [ ] dropwatch

## [ ] ip

## [ ] ethtool

## netdata
- https://github.com/netdata/netdata

## tuned-adm
- https://linux.die.net/man/1/tuned-adm

## ICMP IP 和 Traceroute
https://news.ycombinator.com/item?id=32257852

## packetdrill

## 检查 gateway
route -n
ip r

setup gateway:
```txt
🧀  netstat -rn
Kernel IP routing table
Destination     Gateway         Genmask         Flags   MSS Window  irtt Iface
0.0.0.0         192.168.8.1     0.0.0.0         UG        0 0          0 wlan0
10.0.0.0        0.0.0.0         255.255.255.240 U         0 0          0 eth0
172.17.0.0      0.0.0.0         255.255.0.0     U         0 0          0 docker0
192.168.8.0     0.0.0.0         255.255.252.0   U         0 0          0 wlan0
```
