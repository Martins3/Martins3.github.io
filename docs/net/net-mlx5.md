# mlx5

drivers/net/ethernet/mellanox/mlx5/core/pagealloc.c
管理 fw pages

give_pages 中:

组装命令，然后发送给硬件:
```c
	MLX5_SET(manage_pages_in, in, opcode, MLX5_CMD_OP_MANAGE_PAGES);
	MLX5_SET(manage_pages_in, in, op_mod, MLX5_PAGES_GIVE);
	MLX5_SET(manage_pages_in, in, function_id, func_id);
	MLX5_SET(manage_pages_in, in, input_num_entries, npages);
	MLX5_SET(manage_pages_in, in, embedded_cpu_function, ec_function);

	err = mlx5_cmd_do(dev, in, inlen, out, sizeof(out));
```

原来，driver 会把一些内存直接划给网卡使用，网卡然后来管理这些物理
内存，两者还可以实现回收啊
## 为什么总是 Link up 和 Link Down 的日志

正常情况下:
```txt
@[
    mlx5e_update_carrier+0
    mlx5e_open+52
    __dev_open+288
    __dev_change_flags+416
    dev_change_flags+44
    do_setlink+2020
    __rtnl_newlink+1260
    rtnl_newlink+88
    rtnetlink_rcv_msg+624
    netlink_rcv_skb+104
    rtnetlink_rcv+32
    netlink_unicast+468
    netlink_sendmsg+540
    __sock_sendmsg+76
    ____sys_sendmsg+640
    ___sys_sendmsg+140
    __sys_sendmsg+116
    __arm64_sys_sendmsg+44
    invoke_syscall+80
    el0_svc_common.constprop.0+200
    do_el0_svc+36
    el0_svc+68
    el0t_64_sync_handler+256
    el0t_64_sync+392
```

另外一个位置是在 drivers/net/ethernet/mellanox/mlx5/core/en_main.c:async_event
-> mlx5e_update_carrier_work

看来是固件的问题在发一些奇怪的消息。

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
