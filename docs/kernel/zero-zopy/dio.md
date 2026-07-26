# 存储栈 zero copy

这是最经典，最常见的场景了。

## 如何判断

在 perf 中观察 copy_user_enhanced_fast_string 的占比，很容易的:

- ext4_file_write_iter
  - ext4_dio_write_iter
    - iomap_dio_rw
      - `__iomap_dio_rw`
        - blk_finish_plug
  - ext4_buffered_write_iter
    - generic_perform_write ：提交
    - generic_write_sync ： 同步

```txt
# flamegraph -c "taskset -c 5 fio 4k-read.fio" -b 5
```
可以获取到如下的结果:
![](./fio-direct.svg)
![](./fio-no-direct.svg)

验证其实是很容易的，只是需要 perf 看看就可以了

## 关键细节考虑

### pin page
```txt
@[
        pin_user_pages_fast+5
        iov_iter_extract_pages+266
        iov_iter_extract_bvecs+98
        bio_iov_iter_get_pages+144
        iomap_dio_bio_iter_one+403
        iomap_dio_bio_iter+321
        __iomap_dio_rw+1023
        iomap_dio_rw+18
        xfs_file_dio_write_aligned+223
        xfs_file_write_iter+516
        aio_write+408
        io_submit_one+310
        __x64_sys_io_submit+202
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 49

@[
        unpin_user_folio+5
        __bio_release_pages+303
        __iomap_dio_bio_end_io+368
        clone_endio+147
        blk_mq_end_request_batch+320
        nvme_irq+131
        __handle_irq_event_percpu+108
        handle_irq_event+56
        handle_edge_irq+215
        __common_interrupt+76
        common_interrupt+128
        asm_common_interrupt+38
        cpuidle_enter_state+205
        cpuidle_enter+49
        cpuidle_idle_call+271
        do_idle+156
        cpu_startup_entry+41
        start_secondary+294
        common_startup_64+318
]: 40
```

### page size 对齐

Linux O_DIRECT 通常要求：

- 用户缓冲区地址按 dio_mem_align 对齐
- 文件 offset 按 dio_offset_align 对齐
- I/O size 是 offset alignment 的整数倍，但不要求恰好等于 page size

例如设备逻辑块大小为 512 B 时，可能允许：

buffer address: 512B 对齐
offset:         512B 对齐
size:           512B、1K、4K、128K...

具体约束取决于文件系统和块设备。Linux 6.1+ 可以用：

statx(fd, "", AT_EMPTY_PATH, STATX_DIOALIGN, &stx);

stx.stx_dio_mem_align;
stx.stx_dio_offset_align;

当前内核还可能通过 STATX_DIO_READ_ALIGN 分别报告读对齐要求。

实践中使用 posix_memalign(..., 4096, ...) 和 4K 倍数通常比较保守，但 page size 不是 O_DIRECT 的通用定义。违反实际约束时，可能返回 EINVAL，部分文件系统
也可能退化为 buffered I/O。

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
