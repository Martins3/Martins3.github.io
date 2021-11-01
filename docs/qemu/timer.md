## how to port clock
实际上，timer 的监听就是靠 mian loop 中处理的

- [ ] 为什么制作出来了 timer_list_group 的概念
  - timerlist_run_timers
  - [ ] timerlist_notify : ???
- [ ] 全局变量的 rtc_clock 正确初始化，其中影响是什么?
  - 在 rtc_configure 的地方初始化的
- 为什么搞出来四种 clock 的呀
  - QEMU_CLOCK_VIRTUAL_RT 和 icount 有关，暂时不管了
  - [ ] 主要是 QEMU_CLOCK_HOST 和 QEMU_CLOCK_REALTIME

- [ ] 似乎是 TimersState 主要处理什么的
  - [ ] cpu_get_clock 和 cpu_get_ticks cpu_get_icount 是一个高度对称的

- [ ] QEMUClock 和 QEMUTimer 的关系是什么?


提交之前:
- [ ] 写一个 blog
- [ ] 清理掉所有的 PORT_RTC

## 构建的层次关系
```c
struct QEMUTimerListGroup {
    QEMUTimerList *tl[QEMU_CLOCK_MAX];
};
```

### 实际上，这些 timer
```c
QEMUTimerListGroup main_loop_tlg;
static QEMUClock qemu_clocks[QEMU_CLOCK_MAX];
```
- [ ] 是不是一共就是只有 4 个 clock 的
  - [ ] 对于 rr 的 tcg 是如何模拟多个 cpu 的，这个时钟有点控制啊

timer_init_full 默认使用 main_loop_tlg 的，但是下面的调用路径下，还有一种可能的:
```c
/*
#1  timer_init_full (ts=ts@entry=0x55555669a480, timer_list_group=timer_list_group@entry=0x5555565d6688, type=type@entry=QEMU_CLOCK_VIRTUAL, scale=scale@entry=1000000,
 attributes=attributes@entry=0, cb=cb@entry=0x555555bdf250 <cache_clean_timer_cb>, opaque=0x555556815350) at /home/maritns3/core/xqm/util/qemu-timer.c:368
#2  0x0000555555bdf1f3 in timer_new_full (opaque=0x555556815350, cb=0x555555bdf250 <cache_clean_timer_cb>, attributes=0, scale=1000000, type=QEMU_CLOCK_VIRTUAL, timer_
list_group=0x5555565d6688) at /home/maritns3/core/xqm/include/qemu/timer.h:531
#3  aio_timer_new (opaque=0x555556815350, cb=0x555555bdf250 <cache_clean_timer_cb>, scale=1000000, type=QEMU_CLOCK_VIRTUAL, ctx=0x5555565d65b0) at /home/maritns3/core/
xqm/include/block/aio.h:432
#4  cache_clean_timer_init (bs=0x555556815350, context=0x5555565d65b0) at /home/maritns3/core/xqm/block/qcow2.c:828
#5  0x0000555555be1a02 in qcow2_update_options_commit (bs=bs@entry=0x555556815350, r=r@entry=0x7fffe841fe20) at /home/maritns3/core/xqm/block/qcow2.c:1208
#6  0x0000555555be3e5c in qcow2_update_options (bs=bs@entry=0x555556815350, options=options@entry=0x555556bd0c00, flags=flags@entry=139266, errp=errp@entry=0x7fffffffc
cc0) at /home/maritns3/core/xqm/block/qcow2.c:1235
#7  0x0000555555be5602 in qcow2_do_open (bs=0x555556815350, options=0x555556bd0c00, flags=139266, errp=0x7fffffffccc0) at /home/maritns3/core/xqm/block/qcow2.c:1513
#8  0x0000555555be6246 in qcow2_open_entry (opaque=0x7fffffffcc60) at /home/maritns3/core/xqm/block/qcow2.c:1792
#9  0x0000555555cc6d53 in coroutine_trampoline (i0=<optimized out>, i1=<optimized out>) at /home/maritns3/core/xqm/util/coroutine-ucontext.c:115
#10 0x00007ffff5a6f660 in __start_context () at ../sysdeps/unix/sysv/linux/x86_64/__start_context.S:91
```

```c
AioContext *qemu_get_aio_context(void)
{
    return qemu_aio_context;
}
```

## 如何触发
使用 rtc 来作为例子
