# 使用 Anki 持续思考

## 1. Behavior modification
前几天看 [HN](https://news.ycombinator.com/item?id=46264492#46266799)，读到了一段这样的话:

> I think that the real power of spaced repetition is not in flashcard applications like this. It is in behavior modification.
> Let's take a real example to show how this works.
>
> August 19, 2025. My wife called me in to help her decide what to do about a dentist that she thought was ripping her off. A couple of quick suggestions later, and she went to being mad at me about not having heard the problem through before trying to fix it badly. As soon as she was mad, I immediately connected with how stupid what I did was, and that this never goes well. But, of course, it was now too late.
>
> Not a mistake I was going to make for a while. But, given my history, a mistake I was bound to make again.
>
> I changed that. This time I stuck this into my spaced repetition system. Each time the prompt comes up, I remember that scene, holding in mind how it important it is to emotionally engage, not offer quick suggestions, and be sure to listen to the full problem in detail. It takes me less than 30 seconds. Reviewing this prompt, for my whole lifetime, will take less than 15 minutes of work. Just typing this up this time takes more work than I'll spend on it in the next several years.
>
> This mistake hasn't happened since. Not once. And I believe it won't again in my life.
>
> I have literally changed dozens of such behaviors. My wife says that it is like there is a whole new me. She can't believe the transformation.
>
> All it took is looking at spaced repetition as general purpose structured reinforcement, and not as just a way to study flashcards.

这个想法真的震撼到我了，使用 Anki 来重塑习惯。

很多时候，我意识到问题，但是我来不及想如何解决，或者无法立刻做出改变，然后其他的事情出现了，来不及细想对策。
，然后问题继续出现，会想过往的事情，发现很多事情都是眼睁睁的看着问题恶化。
没有办法努力的办法。

我之前有一大段时间都有白天紧张刺激的上班，晚上紧张刺激的玩游戏，我知道这不对，
我晚上的时候应该抽时间打扫卫生，早点休息，去锻炼。但是，等到我回到我的出租屋的时候，这些想法早就忘记了，
只是顺手打开电脑，然后看几个 B 站的订阅更新，订阅看完之后，打开英雄联盟。最后，发誓明天要早睡。

其实想要纠正这个错误并不难，只是需要在白天的时候想起这个时期，做出决定，然后做一些可以纠正这些习惯的事情:
1. 不要在出租屋放电脑手机
2. 回去的时候，先用做点其他的事情，让自己不要进入到可以网上冲浪的气氛中。

## 2. 记忆修改解决问题的成本

我在调试内核的时候，就发现，如果一个问题涉及到物理机重启，最后调试起来都是旷日持久，
如果不涉及重启，一般会很快。如果统计下来，发现中间重启的次数一共也就十几次，总共两三个小时，
只是占总调试时间的一小部分。

实际上并不是，如果是重启虚拟机，或者 process ，可能一天会重启几百次，但是是物理机，每一次重启
都得想一想，导致最后重启次数很少。

同样的，很多事情，用 ChatGPT 查询就代价太高了，因为切换了上下文，等浏览器上的操作完成之后， 我都不知道我刚才想要做什么事情了。
而且本来很容易的事情，现在还需要上网查一下，成本不知不觉的改变了。

我正在尝试使用 anki 记录下:
- 查询过的单词
- 常见的 trace 技术
- aarch64 的语法
- memory model 的几个例子
- x86 汇编
- 记录的所有 check sheet
- kernel lock api
- crash utility 工具
- GCC inline assembly
- C++ 语法
- Rust 语法

虽然制作了 checksheet ，但是我发现相同的问题总是在查询:
- [navi](https://github.com/denisidoro/navi)
- [xieby1's checksheet](https://xieby1.github.io/cheatsheet.html)

## 3. 持续的思考复杂的问题

我之前老是认为，如果彻底理解了，就不会忘记，但是这有两个问题:
1. 很多问题都是一次性没办法完全搞清楚，如果不去持续思考，之前的努力都会浪费
2. 有的问题即便是想清楚了，随着时间对于细节的磨损，对于问题的理解也会逐渐出现出入。

anki 让我持续思考这些事情，例如我长时间不会看 Linux kernel scheduler 相关的内容，但是可以通过 anki 来保证我的理解不会
出现退化，总是会有一些推进的。

## 4. 使用零散的时间来掌握一些新的技能
1. 很多东西需要掌握了才可以记住，但是有的东西一次根本没法掌握(例如一个周末)
例如 Linux kernel 中的 io_uring
2. Rust / C++ 如果没有使用的项目，日常状态很难学会


利用 Anki ，我可以记住那些已经学会的，不然每一次回来，都是重新开始。

## 具体如何操作

我去尝试使用 Anki 的时候发现 ，我发现 Anki 和我现在的笔记体系有点冲突，

我调研了一些项目，发现 Anki 不太满足我需求:

1. 我想使用我熟悉的 UNIX 工具链，Anki 携带的 GUI 是不可接受的
2. 遇到感兴趣的东西我就会记录到笔记仓库中，等到有时间就将相关东西全部整理一下，现在笔记仓库积累了很多文档
```txt
🧀  find . -name "*.md" | wc -l
1947
```
我最终目的是持续思考这些记录下东西，所以这个笔记仓库就是核心，使用 neovim 编辑，git 同步
如果我制作 Anki Deck ，我需要搭建一个新体系，而且如何同步笔记到 Anki Deck 中也是一个问题。
3. Anki 体系下的大部分代码都是 Python 写的，我每次和 Python 打交道都不愉快，我不想使用 Python 。

基本想法和 [Hashcards: A Plain-Text Spaced Repetition System](https://borretti.me/article/hashcards-plain-text-spaced-repetition)
相似，不过希望我有一个比 Hashcards 更加简单的方法:
1. 如果我想记录下什么问题，那么就在一个 markdown 标题下添加一个 uuid 。提示词就是标题，需要回忆的内容就是标题下的内容。
2. 仅仅依赖 open-spaced-repetition/fsrs-rs 提供算法
3. 我不想使用 sqlite 来存储数据，数据就是用 json 格式保存，我现在还没有那么大的数据量了，用 git 来保存同步数据库就可以了。

主要就使用这几个代码:
- [nvim 快捷键](https://github.com/Martins3/My-Linux-Config/blob/6835bc4e74e363798c6fd4abfe730d951a4aec8f/nvim/lua/usr/util.lua#L22) : 快速在文本中插入一个 uuid
这样，我就可以把之前的笔记变成 Anki deck 了。
- [anki.sh](https://github.com/Martins3/Martins3.github.io/blob/master/docs/blog/anki.sh) : 使用 gum rg neovim 来实现和 deck 交互
- [fsrs/src/main.rs](https://github.com/Martins3/Martins3.github.io/blob/master/docs/blog/fsrs/src/main.rs) : 封装 fsrs-rs 算法库，将数据存储在 json 中，提供对于这个 json 数据库的 crud 。

另外，提供了一个单独的工具来搜索 deck [anki-fzf](https://github.com/Martins3/Martins3.github.io/blob/master/docs/blog/anki-fzf.sh)

## 附录

调研过的项目:
- https://github.com/badlydrawnrob/anki
- https://git.foosoft.net/alex/anki-connect
- https://github.com/Mochitto/Markdown2Anki
- https://github.com/ankidroid/Anki-Android
- https://github.com/kerrickstaley/genanki
- https://addon-docs.ankiweb.net/command-line-use.html
- https://www.reddit.com/r/Anki/comments/1o4pr8e/new_addon_released_onigiri_a_more_modern_anki_for/
- https://github.com/HayesBarber/spaced-repetition-learning
- https://www.zhihu.com/question/57569577/answer/1914530358248055064
- https://github.com/ZetloStudio/ZeQLplus : sqlite 的 browser
- https://zhuanlan.zhihu.com/p/1907786810685395276 : FSRS for Anki 发展史 - Jarrett Ye的文章 - 知乎
- https://github.com/shaankhosla/repeater : 类似 Hashcards ，4000 行，TUI
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
