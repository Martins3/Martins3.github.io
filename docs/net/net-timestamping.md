# network timestamping

## 从网络的角度分析时间
```txt
Tracing 1 functions for "__sock_tx_timestamp"... Hit Ctrl-C to end.
^C
  b'__sock_tx_timestamp'
  b'ip_make_skb'
  b'udp_sendmsg'
  b'inet_sendmsg'
  b'sock_sendmsg'
  b'____sys_sendmsg'
  b'___sys_sendmsg'
  b'__sys_sendmsg'
  b'__arm64_sys_sendmsg'
  b'el0_svc_common'
  b'el0_svc_handler'
  b'el0_svc'
```

chronyd 的时间:
```txt
Jun 20 15:48:33 node1960 chronyd[5856]: Enabled HW timestamping on enp129s0f0np0
Jun 20 15:48:33 node1960 chronyd[5856]: Enabled HW timestamping on enp129s0f1np1
Jun 20 15:48:33 node1960 chronyd[5856]: Enabled HW timestamping on enp130s0f0np0
Jun 20 15:48:33 node1960 chronyd[5856]: Enabled HW timestamping on enp130s0f1np1
```

具体细节分析 : Documentation/networking/timestamping.txt



- ip_append_data
- ip_make_skb
  - ip_setup_cork
    - sock_tx_timestamp

- tcp_sendmsg_locked -> tcp_tx_timestamp
  - sock_tx_timestamp

- sock_tx_timestamp
- skb_setup_tx_timestamp : raw 和 af 使用
  - _sock_tx_timestamp

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
