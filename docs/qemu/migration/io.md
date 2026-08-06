# io 后端

## 具体代码分布
- migration/tls.c
- migration/fd.c
- migration/exec.c
- migration/socket.c


他们是拥有完全相同的结构的:
- fd
    - `fd_outgoing_migration`
    - `fd_incoming_migration`
- exec
    - `exec_outgoing_migration`
    - `exec_incoming_migration`
- tls
    - `migration_tls_outgoing_handshake`
    - `migration_tls_incoming_handshake`
- socket
    - `socket_incoming_migration`
    - `socket_outgoing_migration`

- migration/qemu-file.c

## 大端小端问题

migration/qemu-file.c 里字节序只在一种情况下真正需要考虑：往迁移流里写多字节
整数的元数据/控制字段时。

具体说：

- 流格式固定为大端。qemu_put_be16/32/64 和
  qemu_get_be16/32/64(migration/qemu-file.c:780-824）逐字节移位读写，与宿主
  机自身的字节序无关。所以无论跑在 x86（小端）还是 s390/ppc BE 主机上，写到
  流里的长度、标志、section id 等控制信息布局完全一致。调用者只要用这组
  API，就不用再操心字节序——这正是它们存在的原因。

- 不能用 qemu_put_buffer/qemu_get_buffer 直接搬运多字节整数。这两个函数（以
  及 qemu_put_byte）是原样字节拷贝，不做任何转换。如果把一个 uint32_t 的地址
  直接 qemu_put_buffer(f, &len, 4) 写出去，写到流里的就是宿主机字节序，换一
  台不同字节序的机器读就错了。这就是为什么 savevm/migration 代码里所有头字段
  都走 _be* 接口。

- guest RAM 页内容反而不用考虑字节序。内存页就是通过 buffer 原样传输的，格式
  由 guest 自己解释，QEMU 不做转换。推论是：跨字节序的迁移（比如 BE guest/主
  机迁到 LE）QEMU 本来就不支持——流里的控制数据没问题，但内存和设备状态都是按
  guest 端序原样搬的。

一句话总结：写迁移流的结构化字段时必须用 qemu_put_be*/qemu_get_be* 保持大端格式；传原始字节块时无需也无法做字节序处理。



## caller of `migration_channel_connect`

- The `migration_channel_process_incoming` have same caller.


## kimi 梳理

### QEMUFile 不是"文件"

`struct QEMUFile`（migration/qemu-file.c:45）的全部内容：

- `QIOChannel *ioc`：底层真正的 IO 对象
- 32KB 缓冲区（`IO_BUF_SIZE`）
- `iovec` 数组：zero-copy 批量写
- sticky 错误状态（`last_error` / `last_error_obj`）
- 待传递的 fd 队列（`can_pass_fd` + `FdEntry`）

即 QEMUFile 是一个带缓冲的字节流抽象（decorator），历史遗留名字（2003 年
savevm 写磁盘文件的抽象，QIOChannel 约 2015 年才出现）。

- file 可以指远程文件吗：QEMUFile 本身没有本地/远程概念，远程与否由底层
  channel 决定（socket channel 数据就走网络）。另有 `file:` transport 写本地
  文件（离线快照式迁移）。不存在"远程文件"。
- 为什么考虑字节序：migration stream 是线上协议，跨主机/跨架构必须固定字节
  序，所以只有 `qemu_put_be16/32/64`（include/migration/qemu-file-types.h），
  一律大端。
- `rate_limit`：旧版在 QEMUFile 内部；现在挪到了
  migration/migration-stats.c 的 `migration_rate_exceeded()`，基于 `mig_stats`
  统计三类已传输字节（qemu_file + multifd + rdma），`migrate_set_speed` 设的
  限速按 `BUFFER_DELAY` 周期检查。
- 和 channel 的关系：一对一包装。`qemu_file_new_input/output(QIOChannel *)`
  建立，`qemu_file_get_ioc()` 取回。QEMUFile = channel + 缓冲 + 字节序
  helper + fd 传递 + 错误状态。

### io/ 模块

- coroutine 能力：`qio_channel_writev_full_all`（io/channel.c:245）遇到
  `QIO_CHANNEL_ERR_BLOCK` 时调 `qio_channel_wait_cond()`——coroutine 上下文
  中这是 yield（等 fd 就绪再调度回来），非 coroutine 上下文中是 aio_poll 阻
  塞等。函数标 `coroutine_mixed_fn`：ram.c 的发送跑在 migration 线程（非
  coroutine），multifd 收端跑在 coroutine，同一套代码两种行为。
- 屏蔽 OS 接口：io/channel-* 把 socket/file/tls/websock/command 统一成
  readv/writev，Windows 兼容（cmd.exe 分支）靠这层。

`qio_task`：把异步操作（socket connect、TLS 握手）封装成"干活 + 主循环回
调"。`qio_task_run_in_thread` 把可能阻塞的活（TLS 握手要读证书、做密码学运
算）扔到 thread pool，完成后回主循环调 callback。

### 为什么需要 QEMUFile

1. 历史（早于 QIOChannel）。
2. 实际价值：缓冲减少 syscall；字节序/类型化 put/get helper（vmstate 序列化
   全靠它）；sticky 错误（`qemu_file_shutdown` 的注释：必须先置错误再
   shutdown，否则收端可能装全零页导致 guest crash）；fd 传递；zero-copy
   iov；`file:` 迁移需要的 seek/offset（`qemu_put_buffer_at`）。

`save_zero_page` 即典型用法：ram.c 只管往 `QEMUFile*` 写协议字段，不关心底
下是 tcp/unix/tls/fd。

### migration channel 的 transport

入口：migration/channel.c 的 `migration_connect_outgoing/incoming` 和
`migrate_uri_parse`。URI scheme 决定 transport：`tcp:` `unix:` `vsock:` `fd:`
（都走 socket 类）、`rdma:`、`exec:`、`file:`。

- socket/tls 为什么代码长：全是异步的。`socket_connect_outgoing` →
  `qio_channel_socket_connect_async` → 连上后回调
  `socket_outgoing_migration` → 才调 `migration_channel_connect_outgoing`。
  fd/exec/file 是同步拿到 channel 直接调，所以短。
- fd：别人（典型是 libvirt）通过 monitor `getfd` / SCM_RIGHTS 传进来的已打
  开的 fd，fd.c 校验必须是 socket 或 pipe（普通文件被拒，提示用 `file:`）。
  QEMU 不关心 fd 怎么打开的。不是 live upgrade。
- exec：`exec:cmd` → `qio_channel_command_new_spawn("/bin/sh", "-c", cmd)`，
  QEMU 通过子进程 stdin/stdout 收发迁移数据。经典用法：源端
  `migrate "exec:ssh dest socat - TCP:localhost:4444"` 走 ssh 隧道，或
  `exec:cat > state` 存状态文件。与 live upgrade 无关（live upgrade 是
  cpr-exec.c 那套）。
- tls：不是独立 transport，而是 channel 建立后的 upgrade。
  `migration_channel_connect_outgoing` 里
  `migrate_channel_requires_tls_upgrade()` 判断后走
  `migration_tls_channel_connect` → `qio_channel_tls_handshake`（QIOTask 异
  步），握手完成后重新回调进 `migration_channel_connect_outgoing`。
- 纠正：unix domain 是支持的（`migrate_uri_parse`，channel.c:378 处理
  `unix:`），同一机器两个 QEMU 迁移可以用 `unix:/tmp/mig.sock`。tls 和
  unix 不冲突，tls 是包裹层。

收端 `migration_channel_identify`（channel.c:142）用
`QIO_CHANNEL_FEATURE_READ_MSG_PEEK` 偷看前 4 字节 magic 区分主通道
（`QEMU_VM_FILE_MAGIC`）和 multifd 通道（`MULTIFD_MAGIC`），因为多通道可能
乱序到达；TLS 握手时已确定身份所以不需要 peek。

## qemu-file.c : kimi 的再次分析

qemu-file.c 本身只是迁移流的缓冲/发送层，内存页的数据是在 ram.c 里组织好后交给它的。发送内存的整体链路大致是：

上层：ram.c 组织要发的页

ram_save_iterate() → ram_find_and_save_block() → ram_save_page()，对每一页决定怎么发：

- 零页：只发页头（RAM_SAVE_FLAG_ZERO，不携带数据）
- 普通页：save_normal_page()（migration/ram.c:1266）
    1. save_page_header() 先写页头：flag + RAMBlock idstr + 页内 offset
    2. 再写 4K 页数据本身，两条路：
        - qemu_put_buffer() —— 同步拷贝路径
        - qemu_put_buffer_async() —— 零拷贝路径（默认 preCopy 用这个）

multifd 场景每个页被分发到某个 multifd 通道的 pss_channel，逻辑相同，只是 QEMUFile 不同。

qemu-file.c：QEMUFile 的发送机制

struct QEMUFile（qemu-file.c:45）核心是两级缓冲：

- buf[32KB]：小数据（页头、控制信息）先 memcpy 进来的内部缓冲
- iov[64] + may_free 位图：待发送的 iovec 数组

写入路径分两类：

qemu_put_buffer()（qemu-file.c:511）——有拷贝：
数据 memcpy 进 buf，buf 写满后 add_buf_to_iovec() 把这块 buf 挂进 iovec 数组。

qemu_put_buffer_async()（qemu-file.c:501）——零拷贝：
不拷贝，直接 add_to_iovec() 把 guest 物理内存的 host 虚拟地址挂进 iovec。add_to_iovec()（qemu-file.c:461）会把地址相邻
的项合并成一个大 iov，减少 writev 段数。如果调用时 may_free=true（postcopy 且开了 release-ram），对应位会记进 may_free
位图。

qemu_fflush()（qemu-file.c:281）——真正的发送点：
- qio_channel_writev_all(f->ioc, f->iov, f->iovcnt, ...)：一次 writev 把攒下的所有 iovec 发到 socket/channel
- 之后 qemu_iovec_release_ram()（qemu-file.c:235）：把 may_free 标记的连续 guest 内存段做 madvise(MADV_DONTNEED)，迁移 完后源端释放这些物理页
- 累计 mig_stats.qemu_file_transferred

触发 flush 的时机：iovec 攒满 64 项自动 flush（add_to_iovec 里），或上层显式调用（比如一轮迭代结束、发 EOS 快照前）。

错误处理是惰性的：任何一次 IO 失败后错误记在 f->last_error，后续所有 put 操作直接 no-op，由上层在合适时机用
qemu_file_get_error() 检查。

两条旁路

- multifd 不经过这套缓冲：multifd-nocomp.c 自己组 packet 头 + iovec 数组指向 guest 页，qio_channel_writev_all() 直接发 ，每个 multifd 线程一个独立 socket，绕开了 QEMUFile 的 32KB buf 机制。
- mapped-ram（迁移到文件而非网络流）：save_normal_page() 走 qemu_put_buffer_at()（qemu-file.c:534），pwrite 按文件偏移 写，不用 iovec 缓冲。

简单画一下主线：

```
  ram_save_page()
    ├─ save_page_header()  → qemu_put_buffer()   (页头, memcpy 进 buf)
    └─ save_normal_page()
         └─ qemu_put_buffer_async()              (4K 页, 零拷贝挂 iovec)
              └─ add_to_iovec()  (相邻合并; 满 64 项 → qemu_fflush)
                   └─ qemu_fflush()
                        ├─ qio_channel_writev_all()   → socket
                        └─ madvise(DONTNEED)          (may_free 的页)
```

所以 qemu-file.c 的角色可以概括为：把上层零散的"写页头、写页数据"聚合成尽量少的 writev 系统调用，
并在发送后处理源端内存释放。

例如，我们可以观察到 qemu_put_buffer 的调用位置如此:

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - qemu_savevm_state_header
          - qemu_savevm_send_configuration
            - vmstate_save_vmsd_v
              - vmstate_save_field_with_vmdesc
                - save_buffer
                  - qemu_put_buffer

## 经典调用路线
```txt
@[
        qio_channel_socket_writev+0
        qio_channel_writev_full_all+205
        qio_channel_writev_all+22
        qemu_fflush.part.0+150
        qemu_put_buffer.part.0+169
        virtio_gpu_save+263
        vmstate_save_state_v+1046
        vmstate_save+226
        qemu_savevm_state_complete_precopy_non_iterable+153
        qemu_savevm_state_complete_precopy+39
        migration_thread+3088
        qemu_thread_start+161
        start_thread+682
        __clone3+44
]: 125
@[
        qio_channel_socket_writev+0
        qio_channel_writev_full_all+205
        multifd_send_thread+574
        qemu_thread_start+161
        start_thread+682
        __clone3+44
]: 16415
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
