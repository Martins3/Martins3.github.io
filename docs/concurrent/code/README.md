## store-buffer

使用的 CPU 越多，效果越差，其中只有两个 cpu 的时候，
```txt
🧀  make && ./main.out
cc -Wall -g -O3 -c -o main.o main.c
cc -o main.out atomic.S store-buffer.S main.o
Total time = 2.642300 seconds
Diff = 5933
Total time = 2.663892 seconds
Diff = 6055
code/module/concurrent on  master [!+] via C v14.1.1-gcc 🍍 took 2s
🧀  make && ./main.out
cc -Wall -g -O3 -c -o main.o main.c
cc -o main.out atomic.S store-buffer.S main.o
Total time = 2.708534 seconds
Diff = 799
Total time = 3.550406 seconds
Diff = 1850
Total time = 3.684349 seconds
Diff = 1568
code/module/concurrent on  master [!+] via C v14.1.1-gcc 🍍 took 3s
🧀  make && ./main.out
cc -Wall -g -O3 -c -o main.o main.c
cc -o main.out atomic.S store-buffer.S main.o
Total time = 3.962023 seconds
Diff = 1327
Total time = 4.002732 seconds
Diff = 1305
Total time = 4.555141 seconds
Diff = 2685
Total time = 4.623208 seconds
Diff = 2300
```

diff address 的时候，似乎可以看到超线程是有影响的:
```txt
🧀  make && ./main.out
cc -Wall -g -O3 -c -o main.o main.c
cc -o main.out atomic.S store-buffer.S main.o
Total time = 2.685286 seconds
Diff = 0
Total time = 2.685398 seconds
Diff = 0
Total time = 2.687121 seconds
Diff = 0
Total time = 2.693727 seconds
Diff = 0
Total time = 3.382875 seconds
Diff = 0
Total time = 3.390215 seconds
Diff = 0
Total time = 3.391218 seconds
Diff = 0
Total time = 3.393844 seconds
Diff = 0
```

强制绑定 core 之后，看来 cache coherency 的效果是有的:
```txt
🧀  make && ./main.out
cc -Wall -g -O3 -c -o main.o main.c
cc -o main.out atomic.S store-buffer.S main.o
Total time = 7.170165 seconds
Diff = 483
Total time = 7.173837 seconds
Diff = 530
code/module/concurrent on  master [!+] via C v14.1.1-gcc 🍍 took 7s
🧀  make && ./main.out
cc -Wall -g -O3 -c -o main.o main.c
cc -o main.out atomic.S store-buffer.S main.o
Total time = 3.598489 seconds
Diff = 0
Total time = 3.606004 seconds
Diff = 0
```
正好快一倍，是巧合吗?

1. 统计 store 之后马上加载，也可以发现改地址上会有不同的数值，但是比例非常小
    说明，load 会优先从 store-buffer 中优先获取。
2. 但是速度的确是有区别的，可能是提交给 cache 是需要花费时间的

### 使用 13900k 的环境测试

1. 同样的，store 之后立刻加载，大多数时候，加载的内容是本地的内容
2. 相同的 address 和不同的 address 差不明显
3. SMT 的影响不明显


## 测试 barrier 对于性能的影响

### 先看这两个
https://www.visualstudio-staging.com/oldnewthing/20220811-00/?p=106963
https://www.visualstudio-staging.com/oldnewthing/20220812-00/?p=106968

## 看看 ifso 的内容

`__sync_add_and_fetch` 在 aarch64 的环境中反汇编的结果:
```txt
Dump of assembler code for function __aarch64_ldadd8_sync:
   0x00000000004105f0 <+0>:     adrp    x16, 0x440000 <__libc_start_main@got.plt>
   0x00000000004105f4 <+4>:     ldrb    w16, [x16, #136]
   0x00000000004105f8 <+8>:     cbz     w16, 0x410604 <__aarch64_ldadd8_sync+20>
   0x00000000004105fc <+12>:    ldaddal x0, x0, [x1]
   0x0000000000410600 <+16>:    ret
   0x0000000000410604 <+20>:    mov     x16, x0             # x16 为增加的数值
   0x0000000000410608 <+24>:    ldxr    x0, [x1]            # exclusive 的加载 x1 地址的内存
   0x000000000041060c <+28>:    add     x17, x0, x16        # 加上去
   0x0000000000410610 <+32>:    stlxr   w15, x17, [x1]      # store 操作
   0x0000000000410614 <+36>:    cbnz    w15, 0x410608 <__aarch64_ldadd8_sync+24>
   0x0000000000410618 <+40>:    dmb     ish
   0x000000000041061c <+44>:    ret
```

__sync_add_and_fetch 为什么需要 dmb ish ?

从反汇编来看，几乎没有，如果是 8 core，自己写汇编性能会好很多
```txt
Total time = 0.321859 seconds
Total time = 0.501751 seconds
Total time = 0.550507 seconds
Total time = 0.735480 seconds
Total time = 0.852522 seconds
Total time = 0.915451 seconds
Total time = 0.936996 seconds
Total time = 0.961242 seconds
v : 8000000
```
而且这个先到先得太明显了:

```txt
Total time = 3.519091 seconds
Total time = 3.595664 seconds
Total time = 4.475999 seconds
Total time = 4.482764 seconds
Total time = 4.574267 seconds
Total time = 4.583031 seconds
Total time = 4.643181 seconds
Total time = 4.646560 seconds
```

gcc, 2 thread
```txt
🧀  make && ./main.out
cc -Wall -g -O3 -c -o main.o main.c
cc -o main.out atomic.S store-buffer.S main.o
Total time = 4.797001 seconds
Total time = 5.591603 seconds
v : 20000000
```
martins3_atomic_add, 2 thread
```txt
🧀  make && ./main.out
cc -Wall -g -O3 -c -o main.o main.c
cc -o main.out atomic.S store-buffer.S main.o
Total time = 1.695877 seconds
Total time = 2.001450 seconds
v : 20000000
```

gcc , 2 core

```txt
🧀  make && ./main.out
cc -Wall -g -O3 -c -o main.o main.c
cc -o main.out atomic.S store-buffer.S main.o
Total time = 10.780676 seconds
Total time = 10.787335 seconds
v : 20000000
```

martins3_atomic_add, 2 core
```txt
cc -Wall -g -O3 -c -o main.o main.c
cc -o main.out atomic.S store-buffer.S main.o
Total time = 5.713539 seconds
Total time = 6.949093 seconds
```

1. 参与的 CPU 的数量的提升会导致一个 thread 完成的性能大幅下降
2. 不知道为什么自己写的要比 gcc 的快很多
3. 再一次，如果是一个 core 的两个 thread，会快很多

如果只是单核测试
```txt
# martins3_add
🧀  make && ./main.out
Total time = 0.667947 seconds
v : 10000000

# martins3_atomic_add
🧀  make && ./main.out
Total time = 1.965802 seconds
v : 10000000

# gcc
🧀  make && ./main.out
Total time = 2.790660 seconds
v : 10000000
```
### x86
martins3_atomic_add
```txt
   0x0000000000401688 <+216>:   lock incq 0x29e8(%rip)        # 0x404078 <v>
```
如果这样的话，是不是 aarch64 的 gcc 有点 bug 了。

atomic 要比 unatomic 的慢 10 倍:
```txt
🧀  make && ./main.out
clang -Wall -g -O3 -c -o main.o main.c
clang -z noexecstack -no-pie -o main.out x86_64/atomic.S x86_64/store-buffer.S main.o
Total time = 3.314085 seconds
Total time = 3.323297 seconds
v : 20000000
code/module/concurrent on  master [$!+] via C v17.0.6-clang via ❄️  impure (yyds-env) 13900k took 3s
🧀  make && ./main.out
clang -Wall -g -O3 -c -o main.o main.c
clang -z noexecstack -no-pie -o main.out x86_64/atomic.S x86_64/store-buffer.S main.o
Total time = 0.373542 seconds
Total time = 0.374252 seconds
v : 1007c091
```


## 看看这里指向的内容
https://developer.arm.com/documentation/100076/0100/A64-Instruction-Set-Reference/A64-Data-Transfer-Instructions/LDXR?lang=en

## acquire release 问题
使用 `cpp-std/memory-model/ordering/2-acquire-release.cpp` 来测试:

- https://stackoverflow.com/questions/65466840/arm-stlr-memory-ordering-semantics
- https://stackoverflow.com/questions/41858540/whats-are-practical-example-where-acquire-release-memory-order-differs-from-seq

- 为什么 intel 上立刻错误，而 aarch64 反而无法复现?
    - 为什么 aarch64 要去调用函数啊

```txt
   0x00000000004102ac <+0>:     stp     x29, x30, [sp, #-48]!
   0x00000000004102b0 <+4>:     stp     x22, x21, [sp, #16]
   0x00000000004102b4 <+8>:     stp     x20, x19, [sp, #32]
   0x00000000004102b8 <+12>:    mov     x29, sp
   0x00000000004102bc <+16>:    adrp    x8, 0x440000 <_ZSt9terminatev@got.plt>
   0x00000000004102c0 <+20>:    add     x8, x8, #0x7dj
   0x00000000004102c4 <+24>:    adrp    x9, 0x440000 <_ZSt9terminatev@got.plt>
   0x00000000004102c8 <+28>:    add     x9, x9, #0x7c
   0x00000000004102cc <+32>:    cmp     w0, #0x1
   0x00000000004102d0 <+36>:    mov     w22, #0x1                       // #1
   0x00000000004102d4 <+40>:    csel    x20, x9, x8, eq // eq = none
   0x00000000004102d8 <+44>:    csel    x21, x8, x9, eq // eq = none
   0x00000000004102dc <+48>:    adrp    x19, 0x440000 <_ZSt9terminatev@got.plt>
   0x00000000004102e0 <+52>:    add     x19, x19, #0x80
   0x00000000004102e4 <+56>:    b       0x4102ec <_Z4busyi+64>
   0x00000000004102e8 <+60>:    stlrb   wzr, [x20]
   0x00000000004102ec <+64>:    stlrb   w22, [x20]
   0x00000000004102f0 <+68>:    ldarb   w8, [x21]
   0x00000000004102f4 <+72>:    tbnz    w8, #0, 0x4102e8 <_Z4busyi+60>
   0x00000000004102f8 <+76>:    mov     w0, #0x1                        // #1
   0x00000000004102fc <+80>:    mov     x1, x19
   0x0000000000410300 <+84>:    bl      0x4104e0 <__aarch64_ldadd4_relax> <-------- 默认走函数调用了
   0x0000000000410304 <+88>:    cbnz    w0, 0x41033c <_Z4busyi+144>
   0x0000000000410308 <+92>:    mov     w0, #0xffffffff                 // #-1
   0x000000000041030c <+96>:    mov     x1, x19
   0x0000000000410310 <+100>:   bl      0x4104e0 <__aarch64_ldadd4_relax> <---------
   0x0000000000410314 <+104>:   cmp     w0, #0x1
   0x0000000000410318 <+108>:   b.eq    0x4102e8 <_Z4busyi+60>  // b.none
   0x000000000041031c <+112>:   adrp    x0, 0x420000 <_IO_stdin_used>
   0x0000000000410320 <+116>:   add     x0, x0, #0x3c
   0x0000000000410324 <+120>:   adrp    x1, 0x420000 <_IO_stdin_used>
   0x0000000000410328 <+124>:   add     x1, x1, #0x19
   0x000000000041032c <+128>:   adrp    x3, 0x420000 <_IO_stdin_used>
   0x0000000000410330 <+132>:   add     x3, x3, #0x2d
   0x0000000000410334 <+136>:   mov     w2, #0x29                       // #41
   0x0000000000410338 <+140>:   bl      0x410060 <__assert_fail@plt>
   0x000000000041033c <+144>:   adrp    x0, 0x420000 <_IO_stdin_used>
   0x0000000000410340 <+148>:   add     x0, x0, #0x10
   0x0000000000410344 <+152>:   adrp    x1, 0x420000 <_IO_stdin_used>
   0x0000000000410348 <+156>:   add     x1, x1, #0x19
   0x000000000041034c <+160>:   adrp    x3, 0x420000 <_IO_stdin_used>
   0x0000000000410350 <+164>:   add     x3, x3, #0x2d
   0x0000000000410354 <+168>:   mov     w2, #0x23                       // #35
   0x0000000000410358 <+172>:   bl      0x410060 <__assert_fail@plt>
```

如果只有将 arch 调整为 armv8.3-a 才可以，不然前面都是调用库中实现:
```sh
g++ -march=armv8.3-a  -O3 -g cpp-std/memory-model/ordering/2-acquire-release.cpp && ./a.out
clang++ -march=armv8.3-a  -O3 -g cpp-std/memory-model/ordering/2-acquire-release.cpp && ./a.out
```

之后可以得到:
```txt
   0x0000000000410340 <+64>:    mov     w1, #0x1                        // #1
   0x0000000000410344 <+68>:    stlrb   w1, [x2]
   0x0000000000410348 <+72>:    ldaprb  w1, [x3]
   0x000000000041034c <+76>:    tst     w1, #0xff
   0x0000000000410350 <+80>:    b.ne    0x410368 <_Z4busyi+104>  // b.any
   0x0000000000410354 <+84>:    ldadd   w4, w1, [x0]
   0x0000000000410358 <+88>:    cbnz    w1, 0x410370 <_Z4busyi+112>
   0x000000000041035c <+92>:    ldadd   w5, w1, [x0]
   0x0000000000410360 <+96>:    cmp     w1, #0x1
   0x0000000000410364 <+100>:   b.ne    0x410390 <_Z4busyi+144>  // b.any
   0x0000000000410368 <+104>:   stlrb   wzr, [x2]
   0x000000000041036c <+108>:   b       0x410340 <_Z4busyi+64>
```

## atomic 指令的性能影响

ldadda 的结果
```txt
🧀  make
cc -Wall -g -O3 -c -o main.o main.c
cc -march=armv8.3-a -z noexecstack -no-pie -o main.out aarch64/atomic.S aarch64/store-buffer.S main.o
code/module/concurrent on  master [!+] via C v14.2.1-gcc 🍍
```

ldaddl 的结果
```txt
🧀  make && ./main.out
cc -Wall -g -O3 -c -o main.o main.c
cc -march=armv8.3-a -z noexecstack -no-pie -o main.out aarch64/atomic.S aarch64/store-buffer.S main.o
Total time = 5.351535 seconds
Total time = 5.353934 seconds
v : 20000000
```

martins3_atomic_add ，也就是用 ldxr 和 stlxr 来实现的:
```txt
🧀  ./main.out
Total time = 5.557944 seconds
Total time = 6.873546 seconds
v : 20000000
```

ldadd 结果:
```txt
🧀  make && ./main.out
cc -Wall -g -O3 -c -o main.o main.c
cc -march=armv8.3-a -z noexecstack -no-pie -o main.out aarch64/atomic.S aarch64/store-buffer.S main.o
Total time = 2.607980 seconds
Total time = 3.045793 seconds
v : 20000000
```

如果是 gcc
```c
__atomic_add_fetch(&v, 1, __ATOMIC_RELAXED);
```
```txt
Total time = 3.211857 seconds
Total time = 3.213051 seconds
v : 20000000
```

## 这里的是需要看看的
- https://en.cppreference.com/w/cpp/atomic/memory_order#Release-Acquire_ordering

- https://gcc.gnu.org/onlinedocs/gcc/AArch64-Options.html : 有趣的东西

## 理解这个东西

如果是 std::memory_order_release 的时候:


如果是:
```c
static const auto store_ordering = std::memory_order_seq_cst;
static const auto load_ordering = std::memory_order_seq_cst;
```

gdb 中 `disass /m busy` 中实际上没有什么用，但是可以很快定位到:
```txt
			me.store(true, store_ordering);
   0x0000000000410264 <+56>:    stlrb   wzr, [x9] # 设置为 true
   0x0000000000410268 <+60>:    stlrb   w8, [x9]  #

			if (him.load(load_ordering) == false) {

   0x000000000041026c <+64>:    ldarb   w13, [x10]
   0x0000000000410270 <+68>:    tbnz    w13, #0, 0x410264 <_Z4busyi+56>
```

如果是这个:
```c
static const auto store_ordering = std::memory_order_release;
static const auto load_ordering = std::memory_order_acquire;
```

```txt
   0x0000000000410264 <+56>:    stlrb   wzr, [x9]
   0x0000000000410268 <+60>:    stlrb   w8, [x9]

   0x000000000041026c <+64>:    ldaprb  w13, [x10]
   0x0000000000410270 <+68>:    tbnz    w13, #0, 0x410264 <_Z4busyi+56>
```

如果是:
```c
static const auto store_ordering = std::memory_order_relaxed;
static const auto load_ordering = std::memory_order_relaxed;
```

```txt
   0x0000000000410264 <+56>:    strb    wzr, [x9]
   0x0000000000410268 <+60>:    strb    w8, [x9]

   0x000000000041026c <+64>:    ldrb    w13, [x10]
   0x0000000000410270 <+68>:    tbnz    w13, #0, 0x410264 <_Z4busyi+56>
```

- https://developer.arm.com/documentation/dui0801/g/A64-Data-Transfer-Instructions/LDAPRB
    - https://github.com/dotnet/runtime/issues/67374

- https://community.arm.com/arm-community-blogs/b/tools-software-ides-blog/posts/enabling-rcpc-in-gcc-and-llvm

实验已经差不多了。

## 思考的关键问题

cpp 的 memory_model 的定义可以覆盖现在所有的架构吗?

## 将 mm-ll.cpp 在用户态写一波

## cache line 会有影响吗?
如果测试 memory model 的两个指令都是
在相邻的两个字节，但是 cache 的 size 是 64 字节的，这个会有影响吗?
是理论上的影响，还是说导致出现问题的概率不同?

的确是两个
https://stackoverflow.com/questions/65336409/what-does-memory-order-consume-really-do

### compare and exchange 为什么是这样设计的

这个就是 if else 也是使用 memory model 的，好家好啊
https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
https://en.cppreference.com/w/cpp/atomic/atomic_compare_exchange

https://stackoverflow.com/questions/25199838/understanding-stdatomiccompare-exchange-weak-in-c11

这个如何理解
atomic_compare_exchange_weak
cpp-std/atomic/rmw-cas/atomic_compare_exchange.cpp

https://stackoverflow.com/questions/60624189/atomic-compare-exchange-strong-explicit-what-do-the-various-combinations

https://en.cppreference.com/w/cpp/atomic/kill_dependency
https://en.cppreference.com/w/cpp/language/attributes/carries_dependency

https://en.cppreference.com/w/cpp/atomic/atomic_thread_fence

还是先把这个基础东西搞清楚吧:
- https://en.cppreference.com/w/cpp/atomic/memory_order

Sequentially-consistent ordering 的例子为什么可以描述 cst，不是应该用

- https://en.cppreference.com/w/cpp/atomic/atomic_compare_exchange

atomic_exchange

cas

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
