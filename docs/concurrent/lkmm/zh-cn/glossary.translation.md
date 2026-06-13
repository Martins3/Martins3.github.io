# glossary.txt 中文译解

源文件：`tools/memory-model/Documentation/glossary.txt`

## 文档定位

这是一份 LKMM 术语表。它不是让你顺序读完的，而是查词典用的。下面保留原始英文术语，并给出中文理解。

## 术语译解

### Address Dependency

地址依赖。后一个内存访问的地址取决于前一个 load 读出的值。RCU 读侧通过指针访问字段时经常出现这种依赖。

### Acquire

获取语义。对锁来说是加锁；对共享变量来说是带 acquire 的 load 或 RMW。它把这次 load 排在本 CPU 后续访问之前。

### Coherence (`co`)

一致性顺序。描述同一变量上的多个 store 之间，哪个覆盖了哪个。可分为同 CPU 内的 `coi` 和跨 CPU 的 `coe`。

### Control Dependency

控制依赖。后一个 store 是否执行，取决于前一个 load 参与的条件判断。

### Cycle

环。多个 CPU 的排序关系首尾相连形成闭环时，某些结果会被 LKMM 禁止。

### Data Dependency

数据依赖。后一个 store 写入的值依赖于前一个 load 读出的值。

### From-Reads (`fr`)

从读出发的关系。某个 store 来得太晚，没能影响另一个 CPU 的某次 load，于是从该 load 指向这个晚到的 store。

### Fully Ordered

全序。像 `smp_mb()` 或某些全序 RMW 那样，能把本 CPU 前后的访问都隔开。

### Happens-Before (`hb`)

先行发生关系。LKMM 保证前一个访问必须先于后一个访问执行。

### Marked Access

带标记访问。使用 `READ_ONCE()`、`WRITE_ONCE()`、`smp_store_release()` 等特殊原语的访问。

### Pairing

配对。两个 CPU 上的 barrier、release/acquire 或其他同步操作相互呼应，建立同步边。

### Reads-From (`rf`)

读取自。某个 load 读到了某个 store 写入的值。可分为同 CPU 的 `rfi` 和跨 CPU 的 `rfe`。

### Relaxed

宽松语义。访问本身受控，但不提供排序，例如 `READ_ONCE()`、`WRITE_ONCE()`、`*_relaxed`。

### Release

释放语义。对锁来说是解锁；对共享变量来说是带 release 的 store 或 RMW。它把本 CPU 之前的访问排在这次 store 之前。

### Unmarked Access

未标记访问。普通 C 语法读写，如 `a = b[2]`。

## 使用建议

读 `explanation.txt`、`ordering.txt`、`litmus-tests.txt` 时，一旦遇到缩写或关系名卡住，就回来看这份术语表。

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
