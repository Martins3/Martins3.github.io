# rpc


## 基本理解
- [5.3 Remote Procedure Call](https://book.systemsapproach.org/e2e/rpc.html#rpc-fundamentals)

> These three examples represent interesting alternative design choices in the RPC solution space,
> but lest you think they are the only options,
> we describe three other RPC-like mechanisms (WSDL, SOAP, and REST) in the context of web services in Chapter 9.


## gRPC
- gRPC rust : https://github.com/hyperium/tonic

- gRPC 和 sunRPC 的关系是什么？

- [](https://news.ycombinator.com/item?id=40798740)
https://kmcd.dev/posts/grpc-the-good-parts/

## sunRPC
- 和 sunrpc 啥关系 https://docs.oracle.com/cd/E36784_01/html/E36875/xdr-free-3nsl.html
    - 而且，才发现 sunrpc 在 glibc 中实现了，但是 musl 中没有

通过 libvirt 的 meson.build 可以找到:
```meson
    xdr_dep = dependency('libtirpc', required: get_option('driver_remote'))
```

只能说，用的人不多，至少是在用户态。

kernel 中的直接依赖于 SUNRPC 的模块，似乎也不对
```txt
config NET_HANDSHAKE
	bool
	depends on SUNRPC || NVME_TARGET_TCP || NVME_TCP
	default y
```

## kernel 中的 sunrpc 模块测试一下
```txt
/sys/kernel/sunrpc
├── rpc-clients
│   ├── clnt-0
│   │   └── switch -> ../../xprt-switches/switch-0
│   ├── clnt-1
│   │   └── switch -> ../../xprt-switches/switch-0
│   └── clnt-3
│       └── switch -> ../../xprt-switches/switch-1
└── xprt-switches
    ├── switch-0
    │   ├── xprt-0-local
    │   │   ├── dstaddr
    │   │   ├── srcaddr
    │   │   ├── xprt_info
    │   │   └── xprt_state
    │   └── xprt_switch_info
    └── switch-1
        ├── xprt-2-tcp
        │   ├── dstaddr
        │   ├── srcaddr
        │   ├── xprt_info
        │   └── xprt_state
        └── xprt_switch_info
```
用户态的测试 demo 在这里

## 其他
docs/virtio/gvisor.md 中的 urpc 做啥的？

/proc/ rpc 有好多发内容

## 看看这个吧
https://github.com/yaakapp/app

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
