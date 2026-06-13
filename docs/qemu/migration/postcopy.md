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


## TODO
1. 把 poll_fault_page 的 backtrace 搞出来
2. 如果热迁移到文件中，还可以执行 postcopy 吗?

## 关于 non coperative 的讨论有趣的
https://blog.linuxplumbersconf.org/2017/ocw/system/presentations/4699/original/userfaultfd_%20post-copy%20VM%20migration%20and%20beyond.pdf

## 很多路径都是 postcopy 独有的
- ram_save_host_page_urgent

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
