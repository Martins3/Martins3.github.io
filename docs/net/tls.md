## tls
- https://tls13.xargs.org/

## 为什么 tls 的内核中有一份

在 net/tls 下面

```txt
config TLS
	tristate "Transport Layer Security support"
	depends on INET
	select CRYPTO
	select CRYPTO_AES
	select CRYPTO_GCM
	select STREAM_PARSER
	select NET_SOCK_MSG
	default n
	help
	Enable kernel support for TLS protocol. This allows symmetric
	encryption handling of the TLS protocol to be done in-kernel.

	If unsure, say N.

config TLS_DEVICE
	bool "Transport Layer Security HW offload"
	depends on TLS
	select SKB_DECRYPTED
	select SOCK_VALIDATE_XMIT
	select SOCK_RX_QUEUE_MAPPING
	default n
	help
	Enable kernel support for HW offload of the TLS protocol.

	If unsure, say N.
```

似乎默认不用，是不是 nvme over tcp 是依赖的?


## 如果想要开始，从这里启动
- https://tls13.xargs.org/ : openssl 超级详细解释
  - 在 UEFI 的安装启动的过程中，发现 SSL 之类的完全不懂，各种证书

- https://dev.to/techschoolguru/a-complete-overview-of-ssl-tls-and-its-cryptographic-system-36pd
  - ssl 和 tls 的基本介绍

## toy tls
- https://jvns.ca/blog/2017/01/31/whats-tls/
- https://jvns.ca/blog/2022/03/23/a-toy-version-of-tls/
  - https://tls13.xargs.org/

- https://www.nginx.com/blog/improving-nginx-performance-with-kernel-tls/

- https://news.ycombinator.com/item?id=23241934 : ssh-agent 的工作原理是什么 ?

## 又是这个教科书
https://book.systemsapproach.org/security.html

## tls 证书
https://www.pixelstech.net/article/1722045726-All-I-Know-About-Certificates----Certificate-Authority

https://0x00.cl/blog/2024/exploring-tls-certs/

## 资源
https://github.com/rustls/rustls

## 这里看看
https://stackoverflow.com/questions/10175812/how-to-generate-a-self-signed-ssl-certificate-using-openssl/10176685#10176685

## 用户态
### [写给开发人员的实用密码学](https://thiscute.world/posts/practical-cryptography-basics-1/)

- [ ] 散列消息认证码 HMAC
- [ ] 密钥派生函数 KDF，如 Scrypt
- [ ] 密钥交换算法，如 Diffie-Hellman 密钥交换协议
- [ ] 数字签名算法，如 ECDSA

安全的密钥交换算法 DHE/ECDHE

## 操作一下这个吧
https://github.com/zapstar/two-way-ssl-c/blob/master/README.md

## 这个
https://www.kawabangga.com/posts/5330

这里的操作也许搞一下。

## 这个才是需要的
https://github.com/zapstar/two-way-ssl-c/blob/master/README.md

什么叫做 two way

## 思考一个问题

1. 如果 dns server 提供错误的内容，会如何?

## 这个?
https://tls12.xargs.org/

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
