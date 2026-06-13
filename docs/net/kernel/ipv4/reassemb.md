# IP Fragmentation and Reassembly

相当容易理解的概念了，就是如何

## 如何触发
相当容易，让 lo 网卡从默认是 65535 切换为 600 ，然后来做通信就可以了:

ip link set lo mtu 600

默认是 65535 的
```txt
ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
```

在一个中终端里面配置:
```sh
nc -u -l 9999
```
然后在另外一个终端里面配置:
```sh
python3 - << 'EOF'
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
data = b'A' * 4000
for i in range(100):
    s.sendto(data, ('127.0.0.1', 9999))
EOF
```

然后可以观察到:
```txt
@[
        ip_defrag+5
        ip_local_deliver+87
        __netif_receive_skb_one_core+133
        process_backlog+389
        __napi_poll.constprop.0+43
        net_rx_action+320
        handle_softirqs+224
        do_softirq+67
        __local_bh_enable_ip+181
        __dev_queue_xmit+687
        ip_finish_output2+1531
        ip_do_fragment+1175
        ip_output+175
        ip_send_skb+308
        udp_send_skb+409
        udp_sendmsg+2326
        __sys_sendto+472
        __x64_sys_sendto+36
        do_syscall_64+116
        entry_SYSCALL_64_after_hwframe+118
]: 700
```

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
