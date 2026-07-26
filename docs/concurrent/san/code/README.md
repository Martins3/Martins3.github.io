# sanitizer 实验代码

注意: nix 环境的 clang 缺 `libclang_rt.tsan.a`, 链接 TSan 会失败, 所以这里统一用 gcc。

## 结论 1: ASan 和 TSan 不能同时打开

`make asan-tsan`, 两个编译器都在编译期直接报错:

```txt
== gcc ==
cc1: error: ‘-fsanitize=thread’ is incompatible with ‘-fsanitize=address’
== clang ==
clang: error: invalid argument '-fsanitize=address' not allowed with '-fsanitize=thread'
```

原因: 两者的 shadow memory 布局互相冲突 (ASan 用地址高位缩放映射影子内存,
TSan 需要保留若干段地址区间给影子状态), 内存模型不兼容, 只能分别编译两份二进制各跑各的。
同理 `-fsanitize=memory` 也和它们互斥, 只有 `-fsanitize=undefined` 可以和 ASan 叠加。

## 结论 2: TSan 能测到什么

`make tsan` (代码 race-counter.c, 从 ../code/tools/drd.c 拷贝):

- 不加锁运行时, TSan 报告 `shared_counter` 上的 data race, 精确到文件行号、
  读写类型、线程创建栈:

```txt
WARNING: ThreadSanitizer: data race
  Write of size 4 ... by thread T2:
    #0 worker race-counter.c:29
  Previous write of size 4 ... by thread T1:
    #0 worker race-counter.c:29
  Location is global 'shared_counter' of size 4
SUMMARY: ThreadSanitizer: data race race-counter.c:29 in worker
```

- 注意这次运行最终计数值恰好等于 2000000, TSan 依然报告。
  它基于 happens-before 关系判断, 不依赖时序恰好撞上, 所以结果"碰巧对了"也逃不掉。
- 加锁 (`./race-counter.tsan.out protect`) 后 TSan 干净退出, 说明它认识
  pthread mutex 建立的 happens-before 边, 不会误报。

## 结论 3: TSan 测不到什么

`make missed`:

- missed-asm.c: 两个线程用不带 lock 前缀的内联汇编 `incl` 改共享变量。
  race 真实发生 (最终计数值 1249376 < 2000000), 但 TSan 一声不吭。
  TSan 靠编译器对 C/C++ 内存访问插桩, 内联汇编、未插桩的外部库、
  syscall 内部的内存访问对它不可见。
  反过来, 手写汇编自旋锁保护的临界区会被 TSan 误报 —— "看不到同步原语"
  既造成漏报也造成误报。
- missed-coverage.c: race 藏在只有传 `racy` 参数才执行的分支里。
  不带参数运行时 TSan 安静退出; 带 `racy` 运行同一份代码立刻报告。
  TSan 是动态分析, 只能发现本次运行实际执行到的 race,
  报告干净只说明"覆盖到的路径上没有 race", 结论可信度取决于测试覆盖率。

此外 TSan 报告 race 时进程退出码非 0 (默认 66), Makefile 里用 `-` 前缀忽略。

## 结论 4: TSan 的死锁检测

`make deadlock`。两个前提:

- 必须加 `TSAN_OPTIONS="detect_deadlocks=1"`;
- 必须用 clang 的 runtime。gcc 的 TSan 不支持死锁检测 (实测 gcc 15:
  ABBA 用例直接挂死, 无任何报告)。又因为 nix 的 clang 没把 compiler-rt
  挂到默认路径, Makefile 里手动找到 `libclang_rt.tsan-x86_64.a` 并用
  `--whole-archive` 链接 —— 只按需链接时 runtime 的 preinit 初始化代码
  不会被链入, 实测连 data race 都不报。

能检测: 互斥锁获取顺序不一致造成的潜在死锁 (ABBA)。deadlock-abba.c
让两个线程按相反顺序拿锁, 但时间上错开、程序正常结束, TSan 依然报告:

```txt
WARNING: ThreadSanitizer: lock-order-inversion (potential deadlock)
  Cycle in lock order graph: M0 => M1 => M0
```

原理: TSan 在"持有一把锁时又成功拿到另一把锁"时向锁顺序图加边,
发现环就报告 —— 所以它报告的是运行中观察到的危险顺序, 不要求死锁真发生。

不能检测 (`make deadlock` 里后三个用例都是挂死且无报告, 退出码 124
是 timeout 杀掉的):

- deadlock-real.c: 真实发生的死锁。同样的 ABBA, 只是让两个线程时间上
  重叠, 双方永远卡在第二次 lock 上 —— 边只在"成功拿到嵌套锁"时添加,
  边没建成, 检测器无从发现环, TSan 随进程一起卡住, 一片安静。
- deadlock-semaphore.c: 信号量版 ABBA。死锁检测只跟踪互斥锁, 不跟踪
  信号量 (自旋锁同理; 读写锁支持也很有限), 挂死且无报告。
- deadlock-condvar.c: 等一个永远不被 signal 的条件变量。与锁顺序无关,
  不在检测范围内, 挂死且无报告。

## 结论 5: 像 KASAN 一样, 自己实现 ASan 的 callback

`make myasan` (代码 myasan.c + myasan-demo.c)。

`-fsanitize=kernel-address` 下编译器只负责插桩, 不链接任何 runtime ——
这正是内核用 ASan 的方式: shadow memory、poison 编码、越界判定全由内核自己的
mm/kasan/ 提供。这个 demo 在用户态复刻了同样的结构:

- 用 `-mllvm -asan-instrumentation-with-call-threshold=0` (相当于 KASAN_OUTLINE)
  让每次内存访问都变成 `__asan_load/storeN_noabort()` 调用, 这些函数在
  myasan.c 里手写实现, 内部查 shadow。
- shadow 映射 `(addr >> 3) + 0x7fff8000`, mmap 预留 16TB 虚拟空间
  (`MAP_NORESERVE`, 不碰不占物理内存); 编码照搬 KASAN: 0 可访问、1~7 部分
  可访问、0xfa global redzone、0xfd freed、0xfe heap redzone。
- `__asan_register_globals()` 由编译器生成的 `asan.module_ctor` 在 main
  之前调用, 给每个全局变量围 redzone —— 和 KASAN 处理全局变量的路径一致。
- myasan_malloc/myasan_free 扮演 "调用 kasan_poison 的 kmalloc/kfree":
  分配时围 redzone, free 时整个 poison 成 0xfd 且不真释放 (quarantine)。

一次运行抓出三类错误 (报告完继续跑, 对应 `*_noabort` 语义):

```txt
==myasan== ERROR: WRITE of size 4 ... shadow byte: 0xfe (heap-buffer-overflow)
==myasan== ERROR: WRITE of size 1 ... shadow byte: 0xfd (use-after-free)
==myasan== ERROR: WRITE of size 4 ... shadow byte: 0xfa (global redzone)
```

而 myasan-demo.c 里 `p[12] = 'a'` (13 字节对象的最后一个合法字节) 安静通过,
说明 partial granule (1~7) 的判定没有误报。

注意点: runtime (myasan.c) 必须不带 `-fsanitize` 单独编译, 否则自己插桩
自己, 无限递归; KASAN 内核也是靠 `KASAN_SANITIZE_xxx.o := n` 把自己摘出来的。

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
