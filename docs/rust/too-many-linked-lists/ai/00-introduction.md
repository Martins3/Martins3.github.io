# Introduction（引言）

## 全书定位

作者（Gankra，Rust 标准库集合模块的前维护者）经常被问"如何用 Rust 实现链表"，答案取决于具体需求、难以当场说清，于是写这本书一劳永逸地回答。全书通过实现 **6 种链表**来教授从基础到进阶的 Rust 编程。

本书基于 **Rust 2018 edition**（rustc 1.31，2018-12 发布）。使用更新的工具链即可；用更老的工具链会触发书中未提及的额外编译错误（作者戏称为 "hardmode"）。

## 将实现的 6 种链表

1. **A Bad Singly-Linked Stack** —— 糟糕的 `Box` 单链栈
2. **An Ok Singly-Linked Stack** —— 合格的单链栈（泛型、Option、借用、迭代器）
3. **A Persistent Singly-Linked Stack** —— 基于 `Rc` 的持久化（函数式）单链栈
4. **A Bad But Safe Doubly-Linked Deque** —— 基于 `Rc` + `RefCell` 的安全双端队列
5. **An Unsafe Singly-Linked Queue** —— 用 `unsafe` + raw pointer 实现的单链队列
6. **TODO: An Ok Unsafe Doubly-Linked Deque** —— 更完善的 unsafe 双端队列（成书时未完成）
7. Bonus: A Bunch of Silly Lists —— 附录：各种搞笑链表

## 学习目标（覆盖的 Rust 概念）

- 指针类型：`&`、`&mut`、`Box`、`Rc`、`Arc`、`*const`、`*mut`、`NonNull`(?)
- Ownership、borrowing、inherited mutability、interior mutability、`Copy`
- 全部关键字：struct、enum、fn、pub、impl、use 等
- Pattern matching、generics、destructors
- Testing、安装工具链（rustup）、使用 `miri`
- Unsafe Rust：raw pointers、aliasing、stacked borrows、`UnsafeCell`、variance

链表之所以是绝佳的教学载体，正是因为它"足够糟糕"，以至于在实现过程中会真实碰到上述所有概念。

## 开发环境

```text
> cargo new --lib lists
> cd lists
```

- 使用标准包管理器 Cargo（非必需，但远好于直接用 rustc）；简单试验可用 play.rust-lang.org。
- 每个链表放在单独的文件中，互不覆盖。
- 后续章节会用 `rustup` 安装额外工具（如 miri），作者强烈建议所有工具链都通过 rustup 管理。

## 学习前提与教学方式

- **真实的 Rust 学习体验 = 写代码 → 编译器报错 → 搞清楚错误含义**。作者刻意保留大量编译错误现场，因为学会阅读 Rust 优秀的编译错误和文档对成为高效的 Rust 程序员至关重要。
- 但这是一场"有向导的报错之旅"：书中只展示有教学价值的错误，省略了"复制粘贴手滑"之类的常见错误（后续章节尤其如此）。
- 教学节奏偏慢，语言不严肃，不适合追求信息密度最大、形式化内容的读者。

## PSA：作者痛恨链表（以及为什么不重要）

**核心立场**：linked list 是糟糕的数据结构。在 Rust 程序中，99% 的场景应该用 `Vec`（array stack），剩下 1% 中的 99% 应该用 `VecDeque`（array deque）。原因：分配次数更少、内存开销更低、真正的随机访问、cache locality。链表的适用场景（大量 split/merge、lock-free 并发、内核 intrusive list、纯函数式语言）都是**罕见例外**而非普遍情况。

### 常见反驳及作者的回应

- **"性能不重要"**：这不是选链表的理由，这是"随便选什么都行"的理由——那就更应该用数组这个默认选项。
- **"有指针时 split/append/insert/remove 是 O(1)"**：成立，但（Bjarne Stroustrup 的观点）如果*获得那个指针*的开销远大于直接拷贝整个数组（拷贝其实很快），O(1) 就毫无意义。缓存效应和代码复杂度会抵消理论收益，除非 workload 被 split/merge 主导。
- **"我承受不起 amortized 成本"**：数组并非必然 amortized——能预估或上界元素数量时，预分配全部空间后 `push`/`pop` 就是真 O(1)，且远快于链表（指针偏移 + 写数据 + 整数自增，不走分配器）。Rust 迭代器提供 `size_hint` 正是为此。仅在无法预估负载时链表才有 worst-case 延迟优势。
- **"链表省空间"**：复杂。数组的浪费是最坏情况（扩容至多浪费一半；Rust 集合不自动缩容），最好情况整个数组只有 3 个指针的开销。链表则**无条件按元素浪费**：单链 1 个指针/元素，双链 2 个。元素很小时相对开销极大——存 byte 时可达 16x（32 位下 8x），加上对齐 padding 实际约 23x（32 位下 11x）。这还假设分配器密集分配、无碎片。只有元素巨大、负载不可预测、分配器良好时才有内存收益。
- **"函数式语言里我一直在用链表"**：成立——函数式语言中链表代表"无需可变状态的迭代"（下一个子表即下一步），配合惰性求值还能处理无限列表。但 Rust 用 **iterators** 做同样的事（可无限、可 map/filter/reverse/concat，同样惰性）；用 **slices** 表达子数组，函数式的 head/tail 拆分就是 `slice.split_at_mut(1)`，且有 slice patterns。函数式语言的"不可变语义"是其特性（允许编译器做 exotic transformations / fusion），但存取数据仍应选合适的数据结构。
- **"链表适合并发数据结构"**：成立，但写并发数据结构是完全不同的领域；且写完之后用户选的是"MPSC queue"这类抽象，不是"链表"这种实现策略。链表确实是 lock-free 并发世界的事实英雄。
- **"内核/嵌入式 intrusive list"**：niche，完全不依赖语言 runtime，且 wildly unsafe。
- **"迭代器不被无关的插入/删除失效"**：是微妙的玩法，无 GC 时更危险；但可以用 cursor 做很酷的事。
- **"链表简单、适合教学"**：成立——这正是本书存在的前提。单链表确实简单，双链表则会相当棘手。

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
