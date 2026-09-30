# dccp
https://www.reddit.com/r/programming/comments/5i3anm/dccp_the_socket_type_you_probably_never_heard_of/

倒不是完全没用，如果是用来替代 tcp ，这个是一个新的 socket type ，所以可以作为对比试验。

按照 blog 中的提示:

```txt
./probe.out # 看内核是否支持
./server.out
./client.out 127.0.0.1 1337 42 "fafafa" # port 和 service code 都是硬编码的
```
走通很容易的，

笑死，还没开始使用，这个东西就结束了
```txt
[19190.355270] DCCP: Activated CCID 2 (TCP-like)
[19190.356702] DCCP is deprecated and scheduled to be removed in 2025, please contact the netdev mailing list
```

原来是这个体系中的一个
```c
	int sock_fd = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP);
```
```c
enum sock_type {
	SOCK_STREAM	= 1,
	SOCK_DGRAM	= 2,
	SOCK_RAW	= 3,
	SOCK_RDM	= 4,
	SOCK_SEQPACKET	= 5,
	SOCK_DCCP	= 6,
	SOCK_PACKET	= 10,
};
```

- dccp
  - https://www.reddit.com/r/programming/comments/5i3anm/dccp_the_socket_type_you_probably_never_heard_of/
  - https://www.anmolsarma.in/post/dccp/
  - https://wiki.wireshark.org/DCCP
  - dccp 似乎是一个是 TCP UDP 的例子

已经被移除了，

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
