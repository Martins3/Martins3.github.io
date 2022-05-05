# A TCP/IP Tutorial[^3] 阅读笔记

TCP/IP 一般不仅仅指的是 TCP IP 两个协议，还包括 UDP, ARP 和 ICMP

Ethernet frame 的地址为 MAC 地址

Ethernet uses CSMA/CD (Carrier Sense and Multiple Access with
Collision Detection).

ARP (Address Resolution Protocol) : 根据 IP 查询 MAC 地址

TCP is a sliding window protocol with time-out and retransmits.

## tcp
- [CyC2018/CS-Notes : TCP 总结的真的到位](https://github.com/CyC2018/CS-Notes/blob/master/notes/%E8%AE%A1%E7%AE%97%E6%9C%BA%E7%BD%91%E7%BB%9C%20-%20%E4%BC%A0%E8%BE%93%E5%B1%82.md)

### 三次握手
- client 端的过程 : https://github.com/liexusong/linux-source-code-analyze/blob/master/tcp-three-way-handshake-connect.md

[^1]: [TCP : rfc793](https://tools.ietf.org/html/rfc793)
[^3]: https://tools.ietf.org/html/rfc1180
[^4]: http://yuba.stanford.edu/rcp/
