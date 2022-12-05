# multifd
- [ ] multifd 可以使用的 Channel IO


- [ ] `multifd_send_pages` 上的注释读读

- `multifd_save_setup`
    - `migrate_multifd_channels` : 这个数值是从 qmp 设置的
    - `socket_send_channel_create(multifd_new_send_channel_async, p);` :



- [ ] 找到从 `ram_save_iterate` 到这里的


- `multifd_send_sync_main` 的调用者:
    * `ram_save_setup`
    * `ram_save_iterate`
    * `ram_save_complete`
主要进行一些统计更新的工作

## 外部使用的接口

- `multifd_save_cleanup`


## 基本的执行流程

- `multifd_channel_connect`
    - `multifd_send_thread`
        - `multifd_send_fill_packet` : 这个东西只是描述信息
        - `qio_channel_writev_full_all` ：真正的发送，应该调查一下，ioc 是如何赋值的
    - `multifd_tls_channel_connect` ：[ ] 想要看懂这个，需要将两个 channel-tls.c 中的看看
        - `multifd_tls_handshake_thread`
            - `qio_channel_tls_handshake`
            - `multifd_tls_outgoing_handshake`
                - `multifd_channel_connect` : 看到没有，有重新调回来了

- `multifd_recv_new_channel`
    - `multifd_recv_thread`
        - `multifd_recv_unfill_packet` 将 package 的内容解开，但是需要进行很多判断，所以更加长

- `ram_save_iterate` / `ram_save_complete`
    - `ram_find_and_save_block`
        - `ram_save_host_page`
            - `ram_save_target_page`
                - `ram_save_multifd_page`
                    - `multifd_queue_page` : 主要的线程将 dirty page 将找到的脏页内存地方存放到 `multifd_send_state` 全局变量
                        - `multifd_send_pages` 的主要工作是遍历所有 `multifd` 线程，查找空闲的线程，将要发送的内存页地址传递给空闲线程，然后唤醒它，使其开始发送 page

## [ ] 如何理解两个结构体: `MultiFDPacket_t` 和 `MultiFDPages_t`
