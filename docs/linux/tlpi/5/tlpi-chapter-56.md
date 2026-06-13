# Sockets: Introduction

## 56.1 Overview
Sockets exist in a communication domain, which determines:
1. the method of identifying a socket(ipv4 / ipv6)
2. the range of communication (ipv4 / unix domain)

Socket I/O can be performed using the conventional `read()` and `write()` system calls,
or using a range of socket-specific system calls (e.g., `send()`, `recv()`, `sendto()`, and `recvfrom()`).
By default, these system calls block if the I/O operation can’t be completed immediately. Nonblocking I/O is also possible, by using the fcntl() F_SETFL
operation (Section 5.3) to enable the O_NONBLOCK open file status flag.


In the Internet domain, datagram sockets employ the User Datagram Protocol
(UDP), and stream sockets (usually) employ the Transmission Control Protocol (TCP).
Instead of using the terms Internet domain datagram socket and Internet domain stream
socket, we’ll often just use the terms UDP socket and TCP socket, respectively

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
