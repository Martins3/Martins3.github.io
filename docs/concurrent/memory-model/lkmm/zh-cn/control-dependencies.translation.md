# control-dependencies.txt 中文译解

源文件：`tools/memory-model/Documentation/control-dependencies.txt`

## 文档定位

这篇文档在反复强调一件事：控制依赖是 LKMM 中最脆弱、最容易被误用的排序机制之一。硬件可能投机，编译器更可能直接把你以为存在的条件分支优化没掉。

## 控制依赖到底能保证什么

控制依赖最可靠的场景是“先 load，再根据条件做 store”。也就是：

```c
q = READ_ONCE(a);
if (q)
    WRITE_ONCE(b, 1);
```

这个模式通常能保证前面的读先于后面的写。

但它**不能**单独保证“先读后读”的顺序。像下面这样是不够的：

```c
q = READ_ONCE(a);
if (q)
    p = READ_ONCE(b);
```

如果你想保证第二个读不能跑到前面去，必须加 `smp_rmb()` 或改成更直接的 acquire 语义。

## 为什么 `READ_ONCE()` 和 `WRITE_ONCE()` 不能省

如果前面的读不是 `READ_ONCE()`，编译器可能把它和别的读融合。
如果后面的写不是 `WRITE_ONCE()`，编译器可能融合写、甚至生成“先读再判断再写”的序列，从而把你依赖的控制边破坏掉。

## “两个分支写同一个值”为什么不行

如果 `if` 和 `else` 两边都写 `b = 1`，编译器很可能把这条写提升到条件分支外面。这样一来，load 和 store 之间就不再有真实条件分支，控制依赖消失，CPU 也就可以乱序。

文档给出的结论很明确：

- 如果两边写同一个值，不要指望控制依赖。
- 需要排序时，应改用 `smp_store_release()` 或显式 barrier。

## 运行时条件必须真的存在

控制依赖依赖的是“最终汇编里还存在一个真实条件”。如果编译器能证明条件恒真或恒假，它就有权把分支删掉。比如 `q % MAX` 在 `MAX == 1` 时恒为 0，这样控制依赖就不复存在。

## 控制依赖只覆盖 `if` 分支内部

控制依赖只约束 then/else 分支中的 store，不自动延伸到 `if` 之后的代码。因此：

```c
q = READ_ONCE(a);
if (q)
    WRITE_ONCE(b, 1);
else
    WRITE_ONCE(b, 2);
WRITE_ONCE(c, 1);
```

这里 `a` 对 `c` 没有由控制依赖带来的排序保证。

## 文档最后的结论

- 控制依赖只能稳定地约束“先读后写”。
- 对“先读后读”“先写后读”“先写后写”等其他形式，不要用控制依赖硬凑。
- 编译器根本不理解控制依赖，所以你必须主动保护它不被优化掉。

## 实践上的更稳做法

除非你非常清楚自己在做什么，否则优先用：

- `smp_load_acquire()`
- `smp_store_release()`
- `smp_mb()`

控制依赖应该被当成特例技巧，而不是默认同步方案。

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
