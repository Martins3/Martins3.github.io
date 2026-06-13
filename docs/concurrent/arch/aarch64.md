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
