# A TCP/IP Tutorial[^3] 阅读笔记

TCP/IP 一般不仅仅指的是 TCP IP 两个协议，还包括 UDP, ARP 和 ICMP

Ethernet frame 的地址为 MAC 地址

Ethernet uses CSMA/CD (Carrier Sense and Multiple Access with
Collision Detection).

ARP (Address Resolution Protocol) : 根据 IP 查询 MAC 地址

TCP is a sliding window protocol with time-out and retransmits.

## 9293
[最官方的文档](https://www.rfc-editor.org/rfc/rfc9293#name-introduction)

## 三次握手
- client 端的过程 : https://github.com/liexusong/linux-source-code-analyze/blob/master/tcp-three-way-handshake-connect.md

[^1]: [TCP : rfc793](https://tools.ietf.org/html/rfc793)
[^3]: https://tools.ietf.org/html/rfc1180
[^4]: http://yuba.stanford.edu/rcp/
