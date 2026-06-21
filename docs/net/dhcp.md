# dhcp
<!-- cae96fec-4b67-4af5-80c6-d8d338908738 -->
## dhcp client 的基本操作
```sh
sudo ip addr flush ens5
sudo dhclient -r 
sudo dhclient ens5
```

sudo dhclient -r  && dhclient 如果 ifup 没用

参考 ovs 实验，将 dhcp 搭建一下吧

## dhcp server 的基本操作

参考: https://netbeez.net/blog/how-to-set-up-dns-server-dnsmasq-part-4/

1. 将 ovs 的 br-in 删除 ?
2. 添加如下的内容，也许后面几个需要取消注释?
```txt
dhcp-range=10.0.0.220,10.0.0.250,255.255.0.0,12h
# dhcp-option=option:router,10.0.0.2
# dhcp-option=option:dns-server,1.1.1.1
# dhcp-authoritative
```
3. interface 需要修改为对应的

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
