# net zero copy
https://www.kernel.org/doc/html/latest/networking/msg_zerocopy.html

net/core/skbuff.c

测试代码在 vn/code/src/c/iouring/op-send-zc.c

```txt
@[
        loopback_xmit+5
        dev_hard_start_xmit+96
        __dev_queue_xmit+1744
        ip_finish_output2+587
        __ip_queue_xmit+873
        __tcp_transmit_skb+2326
        tcp_write_xmit+641
        __tcp_push_pending_frames+57
        tcp_sendmsg_locked+1992 在这里去调用 -> skb_zerocopy_iter_stream+68 -> io_link_skb+5
        tcp_sendmsg+47
        sock_sendmsg+262
        io_send_zc+150
        __io_issue_sqe+56
        io_issue_sqe+55
        io_submit_sqes+274
        __do_sys_io_uring_enter+516
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 1
```
也就是在组装 skb 的时候，只是使用用户态的内存来拷贝。
算是一个软件上折中优化。

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
