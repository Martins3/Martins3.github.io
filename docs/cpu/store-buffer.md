# CPU ARCH : 进入到 store buffer 意味着已经 commit 了吗?
<!-- f4489f67-80e8-4622-b835-ba80b4425e3f -->
基本可以这么理解，但要非常小心“进入 Store Buffer”这个说法，因为不同资料对 **Store Buffer / Store Queue** 的命名不完全一致。

对于现代 x86，最准确的理解是：

```text
              执行阶段                    Retirement
                 │                           │
Store 指令 ──────┼───────────────────────────┼─────────────>
                 │                           │
        地址/数据计算完成                    │
                 │                           │
                 ▼                           ▼
        Store Queue / Store Buffer     变成 non-speculative
                                             │
                                             ▼
                                      可以真正写入 L1
                                             │
                                             ▼
                                      对其他 CPU 可见
```

### 关键点

**Store 的地址和数据可以在 ROB retire 之前就进入某个 store queue/buffer 条目。**

例如：

```asm
mov [x], 1
```

它可能经历：

```text
1. decode / rename
2. dispatch
3. store-address uop 执行
4. store-data uop 执行
5. 地址 + 数据已经存在 Store Queue 中
6. 但是 ROB 还没有 retire 这条指令
7. ROB retire
8. 这个 store 变成 non-speculative
9. 之后从 store buffer drain 到 L1 cache
```

所以，如果严格说：

> **“已经进入 Store Buffer”不一定意味着 ROB 已经 commit/retire。**

因为 CPU 必须允许 speculative store **先计算地址和数据**，否则 OoO 性能会非常差。

但是 speculative store **绝对不能真正修改 architectural cache state，使其他 CPU 看见它**。

---

这里有一个特别重要的分界线。

假设：

```asm
mov [x], 1
; 后来发现前面 branch prediction 错了
```

如果这个 store 尚未 retire：

```text
ROB:
  ...
  mov [x],1       <- speculative

Store Queue:
  x = 1           <- 有这个 entry
```

此时如果发生 branch misprediction：

```text
ROB entry          丢弃
Store Queue entry  丢弃
```

因此：

```text
CPU 1 永远不能看到 x = 1
```

否则 speculation 就无法回滚了。

---

### 那 retire 之后发生什么？

一旦 store 到达 ROB head，并成功 retire：

```text
             retire
               │
               ▼
Store entry: speculative
               │
               ▼
Store entry: committed / non-speculative
```

此时从 architectural state 的角度：

> 这条 store 已经 commit 了，不能再因为 exception / branch misprediction 被撤销。

但注意：

**commit ≠ 已经写入 cache。**

这正是 Store Buffer 存在的核心意义。

例如：

```text
ROB                   Store Buffer                 L1
────                  ────────────                 ──

mov [x],1
    │
    └── retire ─────> x=1
                       │
CPU继续执行            │
下一条指令             │
                       └──────── later ─────────> x=1
```

因此可以出现：

```text
ROB: 这条 store 已经退休
CPU architectural state: 已经承诺这个 store 一定发生

但是

Store Buffer:
    x = 1

L1:
    x = 0
```

也就是说：

> **对于 ROB：已经 commit。**
>
> **对于 cache/coherence：还没有完成。**

这也是理解 memory ordering 时特别关键的一层。

---

可以把状态划成三层：

```text
                speculative              architectural        globally visible
                     │                         │                     │
                     ▼                         ▼                     ▼

Store instruction
      │
      │ execute
      ▼
Store Queue
[address,data]
      │
      │ ROB retire
      ▼
Committed Store
(Store Buffer)
      │
      │ drain
      ▼
L1 D-cache
      │
      │ coherence
      ▼
其他 CPU 能观察到
```

因此三个概念不要混：

| 状态                      | 含义                       |
| ----------------------- | ------------------------ |
| store executed          | 地址/数据算出来了                |
| store retired/committed | 不再 speculative，不能回滚      |
| store globally visible  | coherence 意义上别的 CPU 能观察到 |

尤其是：

```text
retired
   !=
globally visible
```

这正是你之前问 `mfence`、store buffer、memory ordering 时最核心的区别。

顺便再强调一个很容易产生误解的地方：很多教材直接画成

```text
CPU → Store Buffer → Cache
```

这通常把 **speculative Store Queue** 和 **post-retirement Store Buffer** 合并画了。所以如果一本资料明确把 “Store Buffer” 定义成“只存放 retired stores 的结构”，那么在**那本资料的术语下**，“进入 Store Buffer”确实就意味着已经 ROB retire；但真实微架构往往没有这么简单的概念边界。

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
