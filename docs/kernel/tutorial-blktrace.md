# blktrace 基本使用

多年之前，有人一脸震惊的对我说:"你做存储，连 blktrace 都不知道!"
从此，blktrace 成为我心中的梦魇。

## 如何理解 raid1_log

```c
#define raid1_log(md, fmt, args...)				\
	do { if ((md)->queue) blk_add_trace_msg((md)->queue, "raid1 " fmt, ##args); } while (0)
```

- `__blk_trace_note_message`


## 源码
https://git.kernel.org/pub/scm/linux/kernel/git/axboe/blktrace.git/about/

从 Makefile 看，是一个工具集合:
```txt
INSTALL_ALL = $(ALL) btt/btt btreplay/btrecord btreplay/btreplay \
      btt/bno_plot.py iowatcher/iowatcher
```

## [ ] 长久无法理解的问题，ftrace 和 blktrace 的关系是什么?


## blktrace
- https://developer.aliyun.com/article/698568

- call_bio_endio 中最后会调用到 `bio_end_io_acct`，是给 blktrace 来处理的吗?

## CONFIG_BLK_DEV_IO_TRACE
`config BLK_DEV_IO_TRACE` 已经告诉了如何使用

  echo 1 > /sys/block/vdb/vdb2/trace/enable
  echo blk > /sys/kernel/tracing/current_tracer
  cat /sys/kernel/tracing/trace_pipe

我超，实现的位置居然就是 kernel/trace/blktrace.c
