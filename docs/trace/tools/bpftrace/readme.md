## ./ip-change.bt (qwen)

想要捕获 ip 变化的
```txt
2025-11-12 11:15:00: IFINDEX 1 DEL IP 127.0.0.1
/home/martins3/data/vn/code/trace/bpftrace/ip-change.bt:24:5-31: WARNING: Can't delete map element because it does not exist.
Additional Info - helper: map_delete_elem, retcode: -2
    delete(@last_ip[$ifindex]);
    ~~~~~~~~~~~~~~~~~~~~~~~~~~
2025-11-12 11:38:16: IFINDEX 1 ADD IP 127.0.0.1
2025-11-12 11:38:16: IFINDEX 1 CHANGE IP 127.0.0.1 -> 127.0.0.1
2025-11-12 11:38:59: IFINDEX 1 DEL IP 127.0.0.1
2025-11-12 12:29:27: IFINDEX 2 DEL IP 192.168.2.167
/home/martins3/data/vn/code/trace/bpftrace/ip-change.bt:24:5-31: WARNING: Can't delete map element because it does not exist.
Additional Info - helper: map_delete_elem, retcode: -2
    delete(@last_ip[$ifindex]);
    ~~~~~~~~~~~~~~~~~~~~~~~~~~
2025-11-12 12:29:27: IFINDEX 2 ADD IP 192.168.2.122
2025-11-12 12:35:36: IFINDEX 1 ADD IP 127.0.0.1
2025-11-12 12:35:36: IFINDEX 1 CHANGE IP 127.0.0.1 -> 127.0.0.1
2025-11-12 12:36:07: IFINDEX 1 DEL IP 127.0.0.1
2025-11-12 12:36:24: IFINDEX 1 ADD IP 127.0.0.1
2025-11-12 12:36:24: IFINDEX 1 CHANGE IP 127.0.0.1 -> 127.0.0.1
2025-11-12 12:36:54: IFINDEX 1 DEL IP 127.0.0.1
```
成功观察到 192.168.2.167 变为 192.168.2.122 ，也就是无线网卡的 ip 变化了。

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
