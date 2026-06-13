## 也许整理到 acpi 哪里
```txt
kvm: exiting hardware virtualization
reboot: Power off not available: System halted instead
```

## 似乎符合预期，但是也感觉有点不对

在虚拟机执行，如果只是配置一个 10.0.2.15 ，也就是 user 的网卡，那么
```txt
mount.nfs4  10.0.0.2:/home/martins3/hack mnt
```
会失败

但是如果给配置了
```txt

5: ens4: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 52:54:00:00:02:61 brd ff:ff:ff:ff:ff:ff
    inet 10.0.97.0/16 brd 10.0.255.255 scope global noprefixroute ens4
       valid_lft forever preferred_lft forever
    inet6 fe80::878e:704f:ff64:bc04/64 scope link noprefixroute
       valid_lft forever preferred_lft forever
```
那么就自动可以了


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
