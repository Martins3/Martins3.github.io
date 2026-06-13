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

## AI 下如何加速学习
<!-- 83d310ad-0545-407c-9f70-0a43f85a03c9 -->

学习，掌握内容现在显然是一个瓶颈，现在人的判断力是瓶颈了。

这里有两个问题，我总是感觉很奇怪:
- 为什么学数学需要做题?
- 为什么去教别人会有比较好的效果?

很多时候，我们会自动的完成
那么，现在我会不会拥有更好的，更加直接的方法来检测，某一个东西，自己其实是没有掌握的。

现在加速的原因:
1. 可以快速搭建环境 (simpelfs 写出来之后，有了这个东西作为支撑)
	- 这里有一个很关键的问题，让你聚焦于你最关心的部分，而不是开始就被 Documentaion/fs 下的上百个文档压垮掉
2. 可以整理重点，让宝贵的认知资源放到最关键的部分，而不是
	- english -> 中文
	- 背景介绍
	- 小的 trick
	- 继续看已经会的东西，可以不断的追问他，直到理解了，而一旦理解了，就很难忘记，因为可以随时随地的回忆这些事情。

这个问题的经典的例子在于，线性代数 + cuda 的学习
	- 需要知道，我们之前是学过自动化控制原理的，然后马上就发现学不下去了，
	遇到了问题很难推进。

3. ai 的分析能力很强，不存在看不懂的代码反复卡住人，人仅仅来解决 ai 无法解决的问题。
	- 经典案例就是 kvm 嵌套虚拟化
	- 这里我需要说明下，如果加速，就是一个很强的能力，我的空闲时间就那么多，不然咋办?

一些有意思的观察:
- 经过多次使用 codex 来学习数学，每次遇到一点困难的时候，我都是会让 codex 来举一个例子，
然后问题一下子豁然开朗了。
这导致了很多事情做法的转向:
	1. 开始尝试更大的挑战，而不是被曾经的那些东西所限制了
	2. 一起是从具体开始做起，现在是从抽象做起。
- 为什么，别人写的文章我是懒得看的，但是我不断的和 codex ，最后也是写了几十页的文档，我就是愿意慢慢写出来的。
因为这完全是顺着我的思路来的。但是，最后其实东西都差不多。

当然，现在有了更多的时间来学习了。

2026-05-23 今天想到了一个更高的话题，其实叫不要去学，而是直接去做，才知道什么才是需要掌握的。

2026-05-26 删代码比写代码更快，所以，把书读厚的过程现在就很快了。

```txt
## 为什么要阅读源码
阅读了一些源码，学习到了很多东西:

我认为阅读源码是学习水平中不可获取的一环:
- 教科书只是总结了最核心的原理，没有实战，其中的奥妙总是难以体会
- 文档只会分析其中关键结果，只有骨架，没有血肉

代码的阅读可以尽量的广泛。从编译器，数据库，操作系统，CPU / GPU 都是可以涉猎一下，
一个完全不同的领域可以改变你对于计算机的认识。

我自己认为的阅读方法:
- 显然代码跑起来，要读活代码，不要读死代码
- 文档和代码交替进行
  - 文档枯燥无聊，代码有时候晦涩难懂，如果在代码看不下去的时候阅读文档，那么就会感觉文档是雪中送碳，在枯燥的文档阅读之后，看看代码，如沐春风。
  - 首先将问题列举出来，带着问题去阅读，而不是为了阅读而阅读。
```

当然，最终的目的是将别人的优秀技术运用在自己的项目中，最好是有所改进。

## AI 如何加速创新?
<!-- 315fc427-086c-4b49-8e40-f739ce20bb54 -->

当然，加速学习，就是加速创新，那么如何直接创新?

如何直接制作想法出来?

## 为什么叫这个名字
遗迹守卫（Ruin Guard）

《原神》中有被称为"耕地机"的机械敌人。

是玩家在提瓦特大陆上经常遇到的大型古代机械。因为它们圆滚滚的身体和挥舞双臂的动作有点像在耕地，所以部分旅行者会这样称呼它们。

此外，遗迹系列还有几种相关的机械：
- 遗迹猎者（Ruin Hunter）—— 会飞的型号
- 遗迹重机（Ruin Grader）—— 更大型的四足版本

在剧情和角色语音中，"独眼小宝"是更常见的昵称（尤其是达达利亚相关剧情里）。这些机械都是坎瑞亚古国遗留下来的战争机器。

## 可以覆盖的内容

### 1. 所有的会议
使用 ai 来讲 youtube 演讲的内容整理下来

https://uni.bluepuni.com/archives/the-linux-scheduler-a-decade-of-wasted-cores/

显然，这个要比看 slides 效果要好的

### 2. 基本经典的书籍

1. perfbook
2. eac
3. CPU 设计的一系列的小册子
5. 各种数学书之类的东西

### 3. 经典的论文合集

就是这个东西了:
- https://papers.cool/ 就是这个东西了
	- https://github.com/bojone/papers.cool

https://github.com/AmberLJC/LLMSys-PaperList

## 现在不用关心
https://github.com/Lum1104/Understand-Anything/blob/main/READMEs/README.zh-CN.md
受 [7days-golang](https://github.com/geektutu/7days-golang) 启发。


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
