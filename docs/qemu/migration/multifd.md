# multifd
- `multifd_save_setup`
    - `migrate_multifd_channels` : 这个数值是从 qmp 设置的
    - `socket_send_channel_create(multifd_new_send_channel_async, p);` :

主要进行一些统计更新的工作

采用 multifd 模式，并不是多个 thread 同时遍历 dirty bitmap ，还是 migration thread 遍历，
但是会有多个 thread 来发送。

## 结构体: `MultiFDPacket_t` 和 `MultiFDPages_t`

## 基本的执行流程

初始化的基本流程:

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
        - `multifd_recv_unfill_packet` 将 package 的内容解开，但是需要进行很多判断

### 发送流程

发送有一个单独的 thread 在处理:

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_iterate
            - ram_save_iterate

- `ram_save_iterate` / `ram_save_complete`
    - `ram_find_and_save_block`
        - `ram_save_host_page`
            - `ram_save_target_page`
                - `ram_save_multifd_page`
                    - `multifd_queue_page` : 主要的线程将 dirty page 将找到的脏页内存地方存放到 `multifd_send_state` 全局变量
                        - `multifd_send` 的主要工作是遍历所有 `multifd` 线程，查找空闲的线程，将要发送的内存页地址传递给空闲线程，然后唤醒它，使其开始发送 page

### 接受流程

接受流程比想象的简单:

例如 multifd + zstd 的时候，其结果为

- __clone3
  - start_thread
    - qemu_thread_start
      - multifd_recv_thread
        - multifd_zstd_recv

在 multifd_zstd_recv 中:

```c
static int multifd_zstd_recv(MultiFDRecvParams *p, Error **errp)
{

    // ...
    for (i = 0; i < p->normal_num; i++) {
        // p->block 是 RAMBlock
        // p->normal[i] 是内存地址，所以，这一段代码的作用就是让 zstd 解压的数据直接写入到 RAMBlock 中
        ramblock_recv_bitmap_set_offset(p->block, p->normal[i]);
        z->out.dst = p->host + p->normal[i];
        z->out.size = page_size;
        z->out.pos = 0;
```

## MultiFDMethods

那么这么说，全部都是压缩相关了?
```sh
rg "MultiFDMethods.*\{"
```

```txt
migration/multifd-nocomp.c
454:static const MultiFDMethods multifd_nocomp_ops = {

migration/multifd.c
153:static const MultiFDMethods *multifd_ops[MULTIFD_COMPRESSION__MAX] = {};

migration/multifd-zlib.c
280:static const MultiFDMethods multifd_zlib_ops = {

migration/multifd-qatzip.c
381:static MultiFDMethods multifd_qatzip_ops = {

migration/multifd-uadk.c
310:static const MultiFDMethods multifd_uadk_ops = {

migration/multifd-zstd.c
268:static const MultiFDMethods multifd_zstd_ops = {

migration/multifd-qpl.c
698:static const MultiFDMethods multifd_qpl_ops = {
```

multifd 总是和压缩绑定到一起，

commit 0222111a22b2 ("migration: Remove non-multifd compression")

multifd_send_setup 中配置压缩方法，其实这个是合理的，既然可以有多个 CPU 来发送，那么就让这些 CPU 先压缩。

## 可以 multifd 和其他的观察到奇怪问题

```txt
(qemu) info migrate
Status:                 completed
Time (ms):              total=11179, setup=8, down=46
RAM info:
  Throughput (Mbps):    577.40
  Sizes:                pagesize=4 KiB, total=8.13 GiB
  Transfers:            transferred=769 MiB, remain=0 B
    Channels:           precopy=738 MiB, multifd=0 B, postcopy=0 B
    Page Types:         normal=191883, zero=2029683
  Page Rates (pps):     transfer=35400
  Others:               dirty_syncs=4, downtime_bytes=32312649
```
这里为什么 precopy=738 MiB, multifd=0 B, postcopy=0 B 三个并列的，
也就是这三个东西可以是多个不为 0 ? 答案是

## 本地热迁移经典路线
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
