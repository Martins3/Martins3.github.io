# backing-dev

## KeyNote
1. 当其中的 cgroup 被清理掉之后，代码的行数将会减少一般

## init

```txt
#0  wb_init (wb=wb@entry=0xffff888100f83060, bdi=bdi@entry=0xffff888100f83000, gfp=gfp@entry=3264) at mm/backing-dev.c:287
#1  0xffffffff812be511 in cgwb_bdi_init (bdi=0xffff888100f83000) at mm/backing-dev.c:614
#2  bdi_init (bdi=bdi@entry=0xffff888100f83000) at mm/backing-dev.c:789
#3  0xffffffff812be56a in bdi_alloc (node_id=node_id@entry=-1) at mm/backing-dev.c:800
#4  0xffffffff816389c7 in __alloc_disk_node (q=q@entry=0xffff888100d20710, node_id=-1, lkclass=lkclass@entry=0xffffffff83559354 <max_loop>) at block/genhd.c:1363
#5  0xffffffff816321ec in __blk_mq_alloc_disk (set=set@entry=0xffff888100fb6708, queuedata=queuedata@entry=0xffff888100fb6600, lkclass=lkclass@entry=0xffffffff83559354 <max_loop>) at block/blk-mq.c:4035
#6  0xffffffff819a3487 in loop_add (i=i@entry=2) at drivers/block/loop.c:1983
#7  0xffffffff833805c0 in loop_init () at drivers/block/loop.c:2237
#8  0xffffffff81000e7f in do_one_initcall (fn=0xffffffff833804ed <loop_init>) at init/main.c:1303
#9  0xffffffff8333a4c7 in do_initcall_level (command_line=0xffff888003e31780 "root", level=6) at init/main.c:1376
#10 do_initcalls () at init/main.c:1392
#11 do_basic_setup () at init/main.c:1411
#12 kernel_init_freeable () at init/main.c:1631
#13 0xffffffff81fa9a11 in kernel_init (unused=<optimized out>) at init/main.c:1519
#14 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

每一个 device 初始化两个 workqueue
- wb_workfn
- wb_update_bandwidth_workfn

如果想要进一步分析，看看 page-writeback 机制吧

## 回答这个问题
https://stackoverflow.com/questions/72428893/how-is-the-bdi-identifier-generated-on-linux
