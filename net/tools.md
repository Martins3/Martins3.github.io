# Network tools internals
使用 [Julia Evans](https://wizardzines.com/networking-tools-poster/) 作为基准

- [ ] 在我的印象中，一直都是 ip 和 ifconfig 是不是存在一些冲突啊

## tcpdump
- [tcpdump](https://jvns.ca/tcpdump-zine.pdf)
- [ ] https://blog.cloudflare.com/bpf-the-forgotten-bytecode/ : bpf, the kernel counterpart of tcpdump

- loopback interface
  - `sudo tcpdump -i lo` : print out many message
- [ ] tcpdump 如何工作的 ?

## nc

https://www.kawabangga.com/posts/4515 中间提到 nc -l  9999 的操作可以了解一下 nc 的含义

## netstat

## nslookup

## wireshark
主要参考[这里](https://gaia.cs.umass.edu/kurose_ross/wireshark.php)

## ip

## ifconfig

## arp

- 为什么感觉 arp 和 dhcp 存在一些冲突啊?
  - 如果一个网络中，加入一个新的机器的流程
    - 为了获取一个 ip addr, 使用 udp broadcast，此时 destination mac address 是 ff.ff.ff.ff.ff.ff
    - 和 dhcp server 可以获取 ip addr，但是无法知道局域网中每一台机器的 mac addr
