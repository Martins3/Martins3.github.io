# 日志压缩式的信息获取

## 前 AI 时代
### 存储领域的 Log Append

存储系统里，log append 是一种很常见的写入模式。

- 写 WAL（Write-Ahead Log）时，把变更顺序追加到文件末尾，顺序写比随机写快得多。
- LSM-tree 里，写入先落到 memtable，再刷成不可变的 SSTable；后台再跑 compaction，把重叠的 key 合并、清理过期版本。
- Kafka 的日志也是追加写，retention 和 compaction 负责回收空间。

核心思路都是：先尽情地 append，再异步地压缩。append 快，压缩慢，但只要压缩能跟上，系统就不会爆炸。

### 工作
我日常的信息输入基本也是这个模式：


我平时主要看 [Hacker News](https://news.ycombinator.com/) 和 [Reddit](https://www.reddit.com/)

内核主要是相关会议（[LPC](docs/kernel/lpc/)、[LSF/MM/BPF](docs/kernel/lsfmmbpf/)、[OSPM](docs/kernel/ospm/)），[LWN](https://lwn.net/)，
极少看邮件列表，跟踪不过来

AI 相关主要看知乎，小红书质量一般，营销偏多。

于是笔记越来越臃肿，真正内化的东西却没多少。
append 速度远大于压缩速度，信息债越积越多，我发现在笔记仓库中居然有 2000 多个 markdown 文档。
但是我没办法，人的分析能力是有限的，我还要上班。

## 日志压缩加速

现在有了 kimi、codex、claude 这些工具，压缩效率突然上来了：

直接提供一个文档给 codex ，让他回答我关于某一个领域积累的所有问题

压缩速度第一次超过了 log append 速度。信息不再只是堆在那里，而是能被及时处理、吸收、输出。

也许下一步要关注的，不是收集更多，而是让压缩质量更高。

## AI 如何加速创新?
<!-- 315fc427-086c-4b49-8e40-f739ce20bb54 -->

当然，加速学习，就是加速创新，那么如何直接创新?

如何直接制作想法出来?

## 现在不用关心
https://github.com/Lum1104/Understand-Anything/blob/main/READMEs/README.zh-CN.md
受 [7days-golang](https://github.com/geektutu/7days-golang) 启发。

## ai 时代如何 review 代码
<!-- c2f516b1-946c-4da8-85d4-e05e19dfc331 -->

1. 不要看代码，而是首先自己想这个问题该如何表述

## [ ] 总结一下阅读方法
- uftrace 之类的
- 内核和用户态的分别都需要一个
- 总结两边都可以使用的

- [qbe](https://github.com/Martins3/Martins3.github.io/blob/master/compiler/qbe.md) : LLVM 对于我来说已经过于庞大了
- [chibicc](https://github.com/rui314/chibicc) : 支持 C11 编译器
- [lua](https://www.lua.org/source/) : 大名鼎鼎的 lua 语言，被广泛的使用，其代码量只有 10000 多行。
- [skift](https://github.com/skiftOS/skift) : 两万行 C++ 构建的操作，支持 image viewer 之类的
- [leveldb](https://github.com/google/leveldb)
- sqlite
- [toydb](https://github.com/erikgrinaker/toydb)
- [musl](./linux/musl.md) : 大名鼎鼎的 musl 库，写的非常清晰
- https://limpet.net/mbrubeck/2014/08/08/toy-layout-engine-1.html
- [mold](https://github.com/rui314/mold) : 配合[教程学习](https://eli.thegreenplace.net/tag/linkers-and-loaders) 应该是不错的
- https://github.com/doocs/jvm

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
