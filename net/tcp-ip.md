## A TCP/IP Tutorial[^3]
The generic term "TCP/IP" usually means anything and everything
related to the specific protocols of TCP and IP. It can include
other protocols, applications, and even the network medium. A sample
of these protocols are: UDP, ARP, and ICMP. A sample of these
applications are: TELNET, FTP, and rcp. 

- [ ] What's rcp[^4]?

An Ethernet frame contains the destination address, source address,
type field, and data.

An Ethernet address is 6 bytes. Every device has its own Ethernet
address and listens for Ethernet frames with that destination
address. All devices also listen for Ethernet frames with a wild-
card destination address of "FF-FF-FF-FF-FF-FF" (in hexadecimal),
called a "broadcast" address.

Ethernet uses CSMA/CD (Carrier Sense and Multiple Access with
Collision Detection). CSMA/CD means that all devices communicate on
a single medium, that only one can transmit at a time, and that they
can all receive simultaneously. If 2 devices try to transmit at the
same instant, the transmit collision is detected, and both devices
wait a random (but short) period before trying to transmit again.

- [x] ARP

When sending out an IP packet, how is the destination Ethernet
address determined?

ARP (Address Resolution Protocol) is used to translate IP addresses
to Ethernet addresses. The translation is done only for outgoing IP
packets, because this is when the IP header and the Ethernet header
are created.

The translation is performed with a table look-up. The table, called
the ARP table, is stored in memory and contains a row for each
computer. 

Two things happen when the ARP table can not be used to translate an
address:
1. An ARP request packet with a broadcast Ethernet address is sent
out on the network to every computer.
2. The outgoing IP packet is queued.

          A      B      C      ----D----      E      F      G
          |      |      |      |   |   |      |      |      |
        --o------o------o------o-  |  -o------o------o------o--
        Ethernet 1                 |  Ethernet 2
        IP network "development"   |  IP network "accounting"
                                   |
                                   |
                                   |     H      I      J
                                   |     |      |      |
                                 --o-----o------o------o--
                                  Ethernet 3
                                  IP network "factory"

               Figure 7.  Three IP Networks; One internet


                ----------------------------
                |    network applications  |
                |                          |
                |...  \ | /  ..  \ | /  ...|
                |     -----      -----     |
                |     |TCP|      |UDP|     |
                |     -----      -----     |
                |         \      /         |
                |         --------         |
                |         |  IP  |         |
                |  -----  -*----*-  -----  |
                |  |ARP|   |    |   |ARP|  |
                |  -----   |    |   -----  |
                |      \   |    |   /      |
                |      ------  ------      |
                |      |ENET|  |ENET|      |
                |      ---@--  ---@--      |
                ----------|-------|---------
                          |       |
                          |    ---o---------------------------
                          |             Ethernet Cable 2
           ---------------o----------
             Ethernet Cable 1

             Figure 3.  TCP/IP Network Node on 2 Ethernets

Computer D is the IP-router; it is connected to
all 3 networks and therefore has 3 IP addresses and 3 Ethernet
addresses. Computer D has a TCP/IP protocol stack similar to that in
Figure 3, except that it has 3 ARP modules and 3 Ethernet drivers
instead of 2. Please note that computer D has only one IP module.

If A sends an IP packet to E, the source IP address and the source
Ethernet address are A’s. The destination IP address is E’s, but
because A’s IP module sends the IP packet to D for forwarding, the
destination Ethernet address is D’s.

One part of a 4-byte IP address is the IP network number, the other part is the IP
computer number (or host number). For the computer in table 1, with
an IP address of 223.1.2.1, the network number is 223.1.2 and the
host number is number 1.

The portion of the address that is used for network number and for
host number is defined by the upper bits in the 4-byte address. All
example IP addresses in this tutorial are of type class C, meaning
that the upper 3 bits indicate that 21 bits are the network number
and 8 bits are the host number. This allows 2,097,152 class C
networks up to 254 hosts on each network.

People refer to computers by names, not numbers. A computer called
alpha might have the IP address of 223.1.2.1. For small networks,
this name-to-address translation data is often kept on each computer
in the "hosts" file. For larger networks, this translation data file
is stored on a server and accessed across the network when needed.

The route table contains one row for each route. The primary columns
in the route table are: IP network number, direct/indirect flag,
router IP address, and interface number. This table is referred to
by IP for each outgoing IP packet.

On most computers the route table can be modified with the "route"
command. The content of the route table is defined by the network
manager, because the network manager assigns the IP addresses to the
computers.

Help is also available from certain protocols and network
applications. ICMP (Internet Control Message Protocol) can report
some routing problems. 

TCP is a sliding window protocol with time-out and retransmits.
Outgoing data must be acknowledged by the far-end TCP.
Acknowledgements can be piggybacked on data.

- [ ] I don't know I have finished it or not.

## tcp

#### retransmission


#### TCP 状态转换
- retransmission 表示 : 如果在指定时间没有返回，那么需要重新发送一下
- tc : 发送的频率

- 其实，难道我们的问题是，为什么存在这么多的选项，和 TCP / IP 的实现可以自由的选择，linux 存在多个配置

- 三次握手的问题，需要让 server 进入 established 状态之后，server 才可以接受数据。
  - server 需要收到两次数据才可以

- 结束 : 看上去很奇怪，但其实就是各自通知一下对方，让对方回复一下自己
- 开始的时候类似 : 从 server 的角度，如果接收到 client 的信号之后，发送完成 ack, 然后就认为是 established, 其实可能 client 根本没有接受到

[TCP状态转换图总结](https://zhuanlan.zhihu.com/p/78540103)

TCP close : client and server do a "active" finished and receive the corresponding ack, then they are closed, this is diagram.[^1]
![](https://benohead.com/wp-content/uploads/2013/07/TCP-CLOSE_WAIT.png)

- [ ] [rfc793](https://tools.ietf.org/html/rfc793) 也许用于参考吧 !



[^3]: https://tools.ietf.org/html/rfc1180
[^4]: http://yuba.stanford.edu/rcp/
