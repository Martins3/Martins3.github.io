# post copy


主要的文件:
- postcopy-ram.c

无论是 post copy 还是 pre copy 总是只有一个 CPU 在运行的：

- [ ] 是不是，首先打开 precopy ，然后打开 post copy

- `ram_postcopy_send_discard_bitmap` ：如果是在 precopy 中，对于 dirty 的 page 是需要重新发送的，但是 post copy 只是将新的 dirty bitmap 发送过去就可以了。
    - 这有意义吗? 问题是修改的 page 还是需要发送


## 核心流程
- des 中的 guest 的 userfault ，发送，src 接受的过程

- [ ] 发送接受是需要一个额外的通道吗?

## userfault 的 feature 有什么

## [ ] blocktime 是什么概念

## [ ] 为什么会和 vhost 有关
`vhost_user_postcopy_advise`

- `postcopy_ram_incoming_setup`
    - `postcopy_ram_fault_thread`
        - `mark_postcopy_blocktime_begin`
        - `postcopy_request_page`
            - `postcopy_request_page`
                - `migrate_send_rp_req_pages`
                    - 会在 gtree 上首先查询一下，其原理并不是很懂
                    - `migrate_send_rp_message_req_pages`
        - `postcopy_pause_fault_thread`
