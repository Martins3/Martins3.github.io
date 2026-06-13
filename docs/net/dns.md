# DNS
才知道 8.8.8.8 是 google 提供的
- https://developers.google.com/speed/public-dns?hl=zh-cn
- https://stackoverflow.com/questions/24821521/wget-unable-to-resolve-host-address-http

## Dnsmasq
https://en.wikipedia.org/wiki/Dnsmasq

https://www.dns.toys/

## /etc/hosts 文件是做什么的
dns ，具体参考

## /etc/resolv.conf 的修改会立刻生效吗?

dns 信息都是放到哪里的?

错误的:
```txt
[martins3]$ nslookup www.google.com
Server:         80.80.80.80
Address:        80.80.80.80#53

** server can't find www.google.com: SERVFAIL
```

正确的:
```txt
🧀  nslookup www.google.com
Server:         180.184.1.1
Address:        180.184.1.1#53

Name:   www.google.com
Address: 142.250.198.196
```

```txt
[martins3@6788 ~]$ dig www.baidu.com

; <<>> DiG 9.16.23 <<>> www.baidu.com
;; global options: +cmd
;; Got answer:
;; ->>HEADER<<- opcode: QUERY, status: SERVFAIL, id: 6391
;; flags: qr rd ra; QUERY: 1, ANSWER: 0, AUTHORITY: 0, ADDITIONAL: 0

;; QUESTION SECTION:
;www.baidu.com.                 IN      A

;; Query time: 97 msec
;; SERVER: 8.8.8.8#53(8.8.8.8)
;; WHEN: Tue Feb 04 22:38:55 CST 2025
;; MSG SIZE  rcvd: 31
```

正常的机器结果:
```txt
🧀  dig www.baidu.com

; <<>> DiG 9.18.28 <<>> www.baidu.com
;; global options: +cmd
;; Got answer:
;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 20731
;; flags: qr aa rd ra; QUERY: 1, ANSWER: 3, AUTHORITY: 0, ADDITIONAL: 1

;; OPT PSEUDOSECTION:
; EDNS: version: 0, flags:; udp: 1232
;; QUESTION SECTION:
;www.baidu.com.                 IN      A

;; ANSWER SECTION:
www.baidu.com.          802     IN      CNAME   www.a.shifen.com.
www.a.shifen.com.       99      IN      A       110.242.69.21
www.a.shifen.com.       99      IN      A       110.242.70.57

;; Query time: 8 msec
;; SERVER: 180.184.1.1#53(180.184.1.1) (UDP)
;; WHEN: Tue Feb 04 22:41:28 CST 2025
;; MSG SIZE  rcvd: 101
```

更加负载的做法
dig @223.5.5.5 www.baidu.com A

## DNSSEC
- https://www.cloudflare.com/zh-cn/learning/dns/dnssec/how-dnssec-works/

https://serverfault.com/questions/848442/how-to-remove-dnssec-support-from-a-domain

## [ ] 可以看看 c 库如何提供 namespace 到

例如在 musl 中的 src/network/lookup_name.c

是如何和 systemd-resolve --status 沟通的

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
