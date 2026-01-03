# 使用 Anki 持续思考

为什么我需要 Anki

## 1. Behavior modification
https://news.ycombinator.com/item?id=46264492#46266799

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

## 2. 记住任何事情

很多事情，用 ChatGPT 查询就代价太高了，因为切换了上下文，等浏览器上的操作完成之后，
我都不知道我刚才想要做什么的:

我正在尝试使用 anki 记录下:
- 将 baidu.fanyi.com 中查询过的单词
- 常见的 trace 技术
- aarch64 的语法
- memory model 的几个例子
- x86 汇编
- 记录的所有 check sheet
- kernel lock api

```txt
🧀  find . -name "*.md" | wc -l
1947
```

## 3. 持续的思考复杂的问题

我之前老是认为，如果彻底理解了，就不会忘记，但是这有两个问题:
1. 很多问题都是一次性没办法完全搞清楚，如果不去持续思考，之前的努力都会浪费
2. 有的问题即便是想清楚了，随着时间对于细节的磨损，
对于问题的理解也会逐渐出现出入。

anki 让我持续思考这些事情，例如我长时间不会看 Linux kernel scheduler 相关的内容，但是可以通过 anki 来保证我的理解不会
出现退化，总是会有一些推进的。

## 如何使用 Anki

不过我去尝试 Anki ，我发现 Anki 和我现在的笔记体系有点冲突，基本想法和
[Hashcards: A Plain-Text Spaced Repetition System](https://borretti.me/article/hashcards-plain-text-spaced-repetition)
相似，不过希望我有一个比 Hashcards 更加简单的方法:

1. 如果我想记录下什么问题，那么就在一个 markdown 标题下添加一个 uuid 。提示词就是标题，需要回忆的内容就是标题下的内容。
2. 仅仅依赖 open-spaced-repetition/fsrs-rs 提供算法

一共就两个代码:
- ./anki.sh : 使用 gum bat rg neovim 来实现 deck 的制作，交互
- ./fsrs/src/main.rs : 封装 fsrs-rs 算法库

另外，提供了一个单独的工具来搜索 deck
- ./anki-fzf.sh :
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
