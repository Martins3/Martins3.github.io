## 简单看看 aarch64 的指令支持

### 三组 atomic 指令
- https://developer.arm.com/documentation/dui0801/g/A64-Data-Transfer-Instructions/LDADDA--LDADDAL--LDADD--LDADDL--LDADDAL--LDADD--LDADDL
```txt
LDADDA Xs, Xt, [Xn|SP] ; 64-bit, acquire general registers
LDADDAL Xs, Xt, [Xn|SP] ; 64-bit, acquire and release general registers
LDADD Xs, Xt, [Xn|SP] ; 64-bit, no memory ordering general registers
LDADDL Xs, Xt, [Xn|SP] ; 64-bit, release general registers
```

- https://developer.arm.com/documentation/dui0801/g/A64-Data-Transfer-Instructions/CASA--CASAL--CAS--CASL--CASAL--CAS--CASL
```txt
CASA Ws, Wt, [Xn|SP{,#0}] ; 32-bit, acquire general registers
CASAL Ws, Wt, [Xn|SP{,#0}] ; 32-bit, acquire and release general registers
CAS Ws, Wt, [Xn|SP{,#0}] ; 32-bit, no memory ordering general registers
CASL Ws, Wt, [Xn|SP{,#0}] ; 32-bit, release general registers

CASA Xs, Xt, [Xn|SP{,#0}] ; 64-bit, acquire general registers
CASAL Xs, Xt, [Xn|SP{,#0}] ; 64-bit, acquire and release general registers
CAS Xs, Xt, [Xn|SP{,#0}] ; 64-bit, no memory ordering general registers
CASL Xs, Xt, [Xn|SP{,#0}] ; 64-bit, release general registers
```

- https://developer.arm.com/documentation/dui0801/g/A64-Data-Transfer-Instructions/SWPA--SWPAL--SWP--SWPL--SWPAL--SWP--SWPL
  - Swap word or doubleword in memory.
```txt
SWPA Ws, Wt, [Xn|SP] ; 32-bit, acquire general registers
SWPAL Ws, Wt, [Xn|SP] ; 32-bit, acquire and release general registers
SWP Ws, Wt, [Xn|SP] ; 32-bit, no memory ordering general registers
SWPL Ws, Wt, [Xn|SP] ; 32-bit, release general registers

SWPA Xs, Xt, [Xn|SP] ; 64-bit, acquire general registers
SWPAL Xs, Xt, [Xn|SP] ; 64-bit, acquire and release general registers
SWP Xs, Xt, [Xn|SP] ; 64-bit, no memory ordering general registers
SWPL Xs, Xt, [Xn|SP] ; 64-bit, release general registers
```
### 几组特殊 load / store 指令
两个特殊的 : LDNP / LDTR

关于 release 和 exclusive 的，简单可以总结如下，当然不完整:
```txt
LD{A}{X}P
LD{A}{X}R
LD{A}{X}R{B,H}
```
- LDP 显示没有 byte 和 half word 的变种

完整的看这里:
https://developer.arm.com/documentation/100076/0100/A64-Instruction-Set-Reference/A64-Data-Transfer-Instructions/A64-data-transfer-instructions-in-alphabetical-order

可以看看这里的几个例子:
- https://developer.arm.com/documentation/100076/0100/A64-Instruction-Set-Reference/A64-Data-Transfer-Instructions/LDAXP
    - Load-Acquire Exclusive Pair of Registers.
- https://developer.arm.com/documentation/100076/0100/A64-Instruction-Set-Reference/A64-Data-Transfer-Instructions/LDXP
    - Load Exclusive Pair of Registers.
- https://developer.arm.com/documentation/100076/0100/A64-Instruction-Set-Reference/A64-Data-Transfer-Instructions/LDXR
    - Load Exclusive Register.

这他喵的在说什么的? https://stackoverflow.com/questions/75406033/how-is-aarch64-atomic-instructions-of-large-system-extensions-lse-implemented

重新看看这个:
- [ ] https://community.arm.com/arm-community-blogs/b/tools-software-ides-blog/posts/enabling-rcpc-in-gcc-and-llvm
- [ ] https://stackoverflow.com/questions/21535058/arm64-ldxr-stxr-vs-ldaxr-stlxr

## RCpc

不过，LDAPR 很新:

- https://developer.arm.com/documentation/100076/0100/A64-Instruction-Set-Reference/A64-Data-Transfer-Instructions/LDAPR
    - Load-Acquire RCpc Register.
    - 这里的 p 和 pair 是没有关系的


https://stackoverflow.com/questions/68676666/armv8-3-meaning-of-rcpc

> When there is a STLR followed by a LDAR to a different address,
then these 2 can't be reordered and hence it is called RCsc (release consistent sequential consistent).

> When there is a STLR followed by a LDAPR to a different address,
then these 2 can be reordered. This is called RCpc (release consistent processor consistent).

> In practice, STLR / LDAPR gives C++ std::memory_order_acq_rel, as opposed to SC.

- https://stackoverflow.com/questions/65466840/arm-stlr-memory-ordering-semantics


一般来说，说 STLR 和 LDAR ，其中的 L 和 A 分别表示 release 和 acquire 。

- https://developer.arm.com/documentation/dui0801/g/A64-Data-Transfer-Instructions/LDAPR?lang=en

这个回答太好了: https://stackoverflow.com/questions/12340773/how-do-memory-order-seq-cst-and-memory-order-acq-rel-differ

在看看 kernel 中的内容:

> A read-modify-write operation with this memory order is both an acquire operation and a release operation.
> No memory reads or writes in the current thread can be reordered before the load, nor after the store.
> All writes in other threads that release the same atomic variable are visible before the modification and
> the modification is visible in other threads that acquire the same atomic variable.

原来都是 no reads and writes in the current thread can be reordered

- https://github.com/utmapp/UTM/issues/2366

## arm 对应的 msr 寄存器是什么样子的？

## [The AArch64 processor (aka arm64), part 14: Barriers](https://devblogs.microsoft.com/oldnewthing/20220812-00/?p=106968)

```txt
    dmb     ish     ; data memory barrier
    dsb     ish     ; data synchronization barrier
    isb     sy      ; instruction synchronization barrier
```

```txt
The data memory barrier ensures that all preceding writes are issued before any subsequent memory operations (including speculative memory access). In acquire/release terms, it is a full barrier. The instruction does not stall execution; it just tells the memory controller to preserve externally-visible ordering. This is probably the only barrier you will ever seen in user-mode code.

The data synchronization barrier is a data memory barrier, but with the additional behavior of stalling until all outstanding writes have completed. This is typically used before changing memory mappings, such as during context switches, to ensure that any outstanding writes complete to the original memory before it gets unmapped.

The instruction synchronization barrier flushes instruction prefetch. This is typically used if you have generated new code, say by jitting it or paging it in from disk.
```
对于 dmb dsb isb 总结的简单清晰，但是:

1. 但是 x86 都是直接自动简化掉的，不敢现象 x86 的硬件如何实现的
例如 x86 的是:
```txt
#define __dma_wmb()	barrier()
```
AArch64 是
```txt
#define __dma_wmb()	dmb(oshst)
```

2. isb 的问题
  - 内核中动态生成代码的不少啊，但是 isb() 的调用位置全部都在 arm 架构相关的代码中. 例如内核模块加载之后，是需要 isb 的吧，或者 bpf 的执行

```txt
    ; sequential consistency interlocked increment and
    ; acquire-release interlocked increment
@@: ldaxr   w8, [x0]                ; load acquire from x0
    add     w8, w8, 1               ; increment
    stlxr   w9, w8, [x0]            ; store it back with release
    cbnz    @B                      ; if failed, try again

    ; acquire-only interlocked increment
@@: ldaxr   w8, [x0]                ; load acquire from x0
    add     w8, w8, 1               ; increment
    stxr   w9, w8, [x0]             ; store it back (no release)
    cbnz    @B                      ; if failed, try again

    ; release-only interlocked increment
@@: ldxr    w8, [x0]                ; load (no acquire) from x0
    add     w8, w8, 1               ; increment
    stlxr   w9, w8, [x0]            ; store it back with release
    cbnz    @B                      ; if failed, try again

    ; relaxed interlocked increment
@@: ldxr    w8, [x0]                ; load from x0
    add     w8, w8, 1               ; increment
    stxr    w9, w8, [x0]            ; store it back
    cbnz    @B                      ; if failed, try again
```
问题:
1. 这就是实现 lock 的方法吗?
2. 如何理解 x : exclusive
3. 如果实现 lock ，后面的 acquire-only interlocked increment
release-only interlocked increment
relaxed interlocked increment
应该是没有什么意义吧

## 如何理解 arm 的 synchronization domain

## [Random ASCII – tech blog of Bruce Dawson](https://randomascii.wordpress.com/2020/11/29/arm-and-lock-free-programming/)

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
