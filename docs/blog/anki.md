## https://borretti.me/article/hashcards-plain-text-spaced-repetition

似乎还是没办法直接使用，而且代码量太大了

- https://github.com/eudoxia0/hashcards
	- 直接 make ，但是 ./target/release/hashcards drill example/ 需要本地才可以访问，必须 localhost 才可以
- https://borretti.me/article/implementing-fsrs-in-100-lines
	- https://github.com/open-spaced-repetition/fsrs-rs

## fsrs-rs 简介

- Retrievability(R) : is the probability of recalling the memory. This is a real number in the range [0, 1]
- Stability (S) is the time in days for R to go from 1 to 0.9 (i.e. 90% probability). This is a real number in the range [0,+∞].
- Difficulty (D) models how hard it is to recall the memory. This is a real number in [1,10]. Note that we start at 1 not 0.

examples/schedule.rs 的例子很好，讲述了两个场景的使用，如何创建一个新的卡和规划下一个卡

主要由你的历史评分（Again / Hard / Good / Easy）长期决定，所以一共存在四个评分

open-spaced-repetition/fsrs-rs 的基本使用方法:

cargo build --example migrate

## 细小的代办

1. 为什么 anki 的 import 这么慢?
	- 按道理一次 rg 就可以了
4. 如何让这些东西写入到发我的电纸书中去
	- 电纸书中安装 terminal ? (应该不难的的)
	- localsend
	- screensrcy 都试试吧
	- 看看这里的内容，也分析一下如何制作 epub
		- https://github.com/SwamiRama/10-ways-to-ruin-proxmox
6. 现在代码变的没有办法记忆了，这显然是很坑的了，最后都完全没记忆自己写过什么代码了
```txt
## docs/concurrent/code/cpp-std/memory-model/disassembly/mm-dis.cpp
TODO 其实里面所有的源码都是需要形成结论，然后整理处理的
```
9. 需要制作一个复习过的词条出来，然后传递到手机上

查询任何一个的结果: 我需要各种复习的图形化展示，从而来调试 fsrs 的算法

10. 现在必须可以兼容私有的仓库，必然每日的总结实在是有点太羞耻了。

11. 需要兼容代码，所以索引的格式需要稍微变化一下
	- 其实不用开头，然后 markdown 注释已经很强了

## 单词的上传办法
https://web.shanbay.com/wordsweb/#/collection

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
