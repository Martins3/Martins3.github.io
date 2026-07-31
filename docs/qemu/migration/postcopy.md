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

## message 机制
- `migrate_send_rp_message`

难道 `to_src_file` 是通信的 channel 吗?
```c
    qemu_put_be16(mis->to_src_file, (unsigned int)message_type);
    qemu_put_be16(mis->to_src_file, len);
    qemu_put_buffer(mis->to_src_file, data, len);
    qemu_fflush(mis->to_src_file);
```

- `migrate_send_rp_req_pages`
    - [ ] `migrate_send_rp_message_req_pages` : 这个应该就是实现 postcopy 的方法了

RP: Return Pass

```c
/* Messages sent on the return path from destination to source */
enum mig_rp_message_type {
    MIG_RP_MSG_INVALID = 0,  /* Must be 0 */
    MIG_RP_MSG_SHUT,         /* sibling will not send any more RP messages */
    MIG_RP_MSG_PONG,         /* Response to a PING; data (seq: be32 ) */

    MIG_RP_MSG_REQ_PAGES_ID, /* data (start: be64, len: be32, id: string) */
    MIG_RP_MSG_REQ_PAGES,    /* data (start: be64, len: be32) */
    MIG_RP_MSG_RECV_BITMAP,  /* send recved_bitmap back to source */
    MIG_RP_MSG_RESUME_ACK,   /* tell source that we are ready to resume */

    MIG_RP_MSG_MAX
};
```
- [ ] 每个消息都分析一下吧!

- `MIG_RP_MSG_RECV_BITMAP`
    - 仅仅使用在 `migrate_send_rp_recv_bitmap` 中

### kimi 的回答

定义在 migration/migration.c，处理逻辑主要在 migration/ram.c（postcopy 页请求）和 migration.c（return_path_*）。 整体作用

热迁移的主数据流是单向的：源端 → 目的端。但有些场景需要目的端反过来给源端发消息，这就是 return path（返回通道）——迁移建立后，QEMU 在主迁移连接
上再开一条反向通道，承载这类"控制面"消息。这个 enum 定义的就是该通道上的消息类型。

各消息的含义

- MIG_RP_MSG_INVALID：占位 0，未初始化/非法消息。
- MIG_RP_MSG_SHUT：目的端告知源端"我不会再发 RP 消息了"，用于关闭返回通道的握手，避免源端一直阻塞等消息。
- MIG_RP_MSG_PONG：PING 的应答，带一个 seq 序号。源端定期发 PING、目的端回 PONG，用来测量源→目的网络的往返延迟。这个延迟会影响 multifd 是否启 用、以及 postcopy 时的页请求节流等决策。
- MIG_RP_MSG_REQ_PAGES_ID / MIG_RP_MSG_REQ_PAGES：postcopy 迁移的核心。postcopy 下目的端 VM 先启动运行，访问到还没迁移过来的页时会触发 userfault，目的端就通过返回通道向源端发送页请求（start: be64, len: be32 表示请求的地址范围），源端收到后优先把这些页发过去。带 _ID 的版本额 外带一个 id 字符串，用于 multifd/postcopy 多通道场景下区分请求来自哪个通道/哪块 RAM。
- MIG_RP_MSG_RECV_BITMAP：要求源端把"目的端已收到页的 bitmap"发回去。用于迁移中断后恢复（postcopy recovery）——连接断了重连后，源端需要知道目的 端已经收到哪些页，避免重复发送。
- MIG_RP_MSG_RESUME_ACK：目的端告诉源端"我已准备好恢复"，用于 postcopy 中断恢复流程中双方状态机重新同步。

小结

返回通道主要支撑三件事：

1. postcopy 缺页请求（REQ_PAGES 系列）——这是它最主要的功能；
2. 延迟探测（PING/PONG）；
3. postcopy 断线恢复（RECV_BITMAP、RESUME_ACK）。

没有返回通道，postcopy 迁移就无法工作；纯 precopy 迁移基本只用到 PONG 做延迟测量。


## 发现 virtme 虚拟机是无法热迁移的
- Postcopy is not supported: vhost-user backend not capable of postcopy

似乎是由于用了 virtiofsd 导致的

## 回忆一下 postcopy 的切换过程
在当前这份 QEMU 代码中，从外部请求 precopy 切换到 postcopy 的唯一公开接口是：

- HMP：migrate_start_postcopy
- QMP：migrate-start-postcopy

两者最终都调用 qmp_migrate_start_postcopy()，因此本质是同一条路线。

但这个命令不会直接执行切换，它只是：

qatomic_set(&s->start_postcopy, true);

迁移线程随后满足以下条件时才调用内部的 postcopy_start()：

1. 迁移开始前已在两端启用 postcopy-ram capability。
2. 当前迁移仍在进行。
3. migration_can_switchover() 成立。
4. 切换预计不会违反 downtime-limit。

所以完整路径是：

migrate_set_capability postcopy-ram on
             ↓
migrate ...                       # 始终先进入 precopy
             ↓
migrate_start_postcopy            # 设置切换请求
             ↓
等待 switchover 条件满足
             ↓
postcopy_start()                  # 真正进入 postcopy

关键区别是：

- postcopy-ram on：允许并准备 postcopy，不会触发切换。
- migrate_start_postcopy：请求切换，但可能延迟生效。
- postcopy_start()：QEMU 内部真正执行状态切换，不是公开 monitor 命令。

因此，如果你说的“路线”是管理层可调用的入口，答案是“是”。QEMU 自身不会仅因为 precopy 长时间不收敛就自动请求 postcopy；通常需要 libvirt、测试脚本
或其他管理程序判断时机后调用该命令。相关实现在 migration/migration.c:1287 和 migration/migration.c:3350。


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
