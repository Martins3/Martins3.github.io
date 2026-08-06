# multifd

## MultiFDMethods 的数据传输主要是压缩

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

## 基本流程

初始化的基本流程:

- multifd_channel_connect
    - multifd_send_thread
        - multifd_send_fill_packet : 这个东西只是描述信息
        - qio_channel_writev_full_all ：真正的发送，应该调查一下，ioc 是如何赋值的
    - multifd_tls_channel_connect ：[ ] 想要看懂这个，需要将两个 channel-tls.c 中的看看
        - multifd_tls_handshake_thread
            - qio_channel_tls_handshake
            - multifd_tls_outgoing_handshake
                - multifd_channel_connect : 看到没有，有重新调回来了

- multifd_recv_new_channel
    - multifd_recv_thread
        - multifd_recv_unfill_packet 将 package 的内容解开，但是需要进行很多判断

- multifd_save_setup
    - migrate_multifd_channels : 这个数值是从 qmp 设置的
    - socket_send_channel_create(multifd_new_send_channel_async, p); :

采用 multifd 模式，并不是多个 thread 同时遍历 dirty bitmap ，还是 migration thread 遍历，
但是会有多个 thread 来发送。对接流程在

## 关键结构体
- MultiFDPacket_t
- MultiFDPages_t

### migration thread 发送流程

dirty page iteration 还是在 migration thread 中进行的:

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_iterate
            - ram_save_iterate

找到了 dirty page 之后，将其传递给 multifd 机制:
- ram_save_iterate / ram_save_complete
    - ram_find_and_save_block
        - ram_save_host_page
            - ram_save_target_page
                - ram_save_multifd_page
                    - multifd_queue_page : 主要的线程将 dirty page 将找到的脏页内存地方存放到 multifd_send_state 全局变量
                        - multifd_send 的主要工作是遍历所有 multifd 线程，查找空闲的线程，将要发送的内存页地址传递给空闲线程，然后唤醒它，使其开始发送 page

### source multifd thread

- __clone3
  - start_thread
    - qemu_thread_start
      - multifd_send_thread
	- MultiFDMethods::send_prepare 对于数据进行压缩之类的

### target multifd thread

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

## 队列深度等价为  1

每个 thread 是一个 depth 为 1 的队列。 每个 MultiFDSendParams 只有一个 pending_job 标志 + 一个 p->data 数据槽（multifd_send() 里直接
swap 指针，migration/multifd.c:400-408）。主线程给一个 thread 派完任务后，必须等该 thread 写完网络并 qatomic_store_release(&p->pending_job,
false) 之后才能再次使用该 channel。

所以 channels_ready 这个 semaphore 的计数实际上就等于"当前空闲 channel 数"。N 个 channel 全忙时，semaphore 耗尽，主线程阻塞在那里，直到某个
thread 完成一轮发送回到循环顶部 post 一次。

但要补充一下：
1. 卡住的是 multifd 这一层的派发者，而调用方（RAM 保存路径）本来就是攒满一批页（multifd_queue_page 按 channel 各自的 buffer 攒 batch）才调一次
   multifd_send() 做一次指针 swap，派发本身是零拷贝的，所以"depth 1"的代价被 batching 摊薄了——每个 depth-1 slot 装的是一批页而不是一页。

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
