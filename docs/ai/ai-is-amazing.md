# 还没结束呢！和 AI 的故事，现在才开始！

全世界软件程序难度下降了 100 倍

## 我想法的转变

### 2017

当时所有的大学老师都开始搞 AI ，这是一个很危险的信号，因为这些搞 AI 的老师似乎都很菜，
有一种大炼钢铁感觉。

其实我之前也是搞过 AI 的，但是发现当时大学里面所有老师基本上都在搞
AI，然后我去了解一下它的原理。它真的跟炼丹是非常像的。然后就是大家，这个导致了一些问题啊，就是说第一个你不太清楚未来的改进方向。那如果我长期找不到改进方向，我整个行业很快就陷入到一个停滞的状态了。然后另外的一个是，我一直对于操作系统怎么运行，CPU
怎么运行，很感兴趣。所以我在研究生的时候选择做体结构相关的东西

### 2024.5

gpt 的能力还不是那么强的时候?

我写到:

```txt
为什么需要单测，linter , gcc 的各种检测，是为了避免低级错误。

但是 ai 代码辅助似乎容易引入低级错误。

还存在一个问题就是 gpt 生成 bash ，实际上阻碍了对于 bash 的理解。
如果代码可以生成，为什么一开始的时候不去做更好的抽象，相似的代码多难维护啊。
```

明显这个时候，我还是抵触的想法，我现在

### 2025.7

22 May 2024 的时候，我写下来了 "但是也许生成单元测试也不是不可以的"
，当时找到了一个项目 https://github.com/Codium-ai/cover-agent

这个时候，我还在犹豫中:

在 2025-07-08 的时候，我写下了 "不得不说，AI 趋势的确无人能挡，现在能做的就是吃好喝好，等待机会。"

因为当时我去 Gemini 一个长期让我觉得很疑惑的问题，就是 Linux 里面的中断和异常 NMI
它们互相谁能嵌套？

他基本上就是回答得非常好，像一个非常资深的 Linux 内核专家了，而且最恐怖的是，他拥有无限的耐心。

2025-12-26 我让豆包给我讲了一晚上的本科数学，因为我真的豆包聊了一个小时，我大受震动。

### 2026

我用了 kimi 和 claude 之后，然后尤其是用了 claude
去做了一些事情，发现它已经比人做的要好得多了。这个时候。我之前所有的想法都变化了，
不再自欺欺人，不再犹豫，彻底拥抱 AI 。

## 我的习惯已经改变了
1. 投机执行
	- 我做所有的事情都是投机执行的，也就是说我会尝试先让 AI 去做一下，如果不行的话。我自己再来处理，在处理的过程中又继续分解成让 AI 能做
	- openclaw
2. 语音输入
3. 网页翻译
	- deepseek
4. AI 写 code 没有**任何**低级错误，而这，是我之前编码的时候非常厌烦的点。

## 终于从 kernel 和 qemu 的泥潭中抽身了
现在有了 ai ，内核的难度迅速从 Dota2 退化为俄罗斯方块

利用 codex ，终于解决了两个长期困扰我的问题
- perfbool
- kvm 嵌套虚拟化


此外，我顺手做一些
1. vim 好几插件
2. 写一个内核态的 fs ，虽然 (可以继续尝试下，利用 /goal 不断的优化)
3. dune 的 x86
4. 实现自己的记忆系统 : anki
5. 理解 ext4 文件系统
- cuda tutorial
- eac
7. 学习数学
8. 我不会再对 fedora 中，我不会再默默忍受 neovim 或者 tmux crash 的 crash ，而是打开 codex 来分析:
```txt
• 定位到了：不是 OOM，也不是插件脚本崩溃。dmesg 和 systemd-coredump 显示 tmux 3.2 在 17:45:13 SIGSEGV，栈是：

  cmd_resize_pane_mouse_update -> layout_resize_layout -> window_pane_resize -> grid_reflow_join

  也就是鼠标拖拽调整 pane 大小时，旧版 tmux server 崩了。

  我已处理：

  - 修改 /home/martins3/.dotfiles/nixpkgs/home/tools.nix:80，去掉固定到旧 nixpkgs 的 old_tmux，改回当前 nixpkgs 的 tmux。
  - 执行了 home-manager switch。
  - 验证当前默认 tmux 已是 tmux 3.6a，~/.nix-profile/bin/tmux 指向新版。
  - 用现有 tmux.conf 启动独立 server 测试通过。
  - 当前还在运行的 server 仍是旧 3.2，我临时执行了 tmux set -g mouse off，避免你重启 tmux 前再次拖拽 pane 触发同一路径。
```

## 结语
1. 不要让你的工作去把你所有的细节的时间全部都填满，不然的话，你没有时间去做真正深入的思考。
2. AI 的重要性再怎么强调也不为过，因为这是一个降维打击。这个和人类可以从空气中捕获氮肥，可以使用煤来代替肌肉力量，现在我们用电力就可以转化成智力。这是前所未有的事情
3. 计算机行业真的是能人辈出啊，每几年就搞出新的花样来。
	- 我劝大家，也是提醒我自己。不要工作得太努力啊，就是因为你今年可能拿到这个钱觉得还不错，明年可能很多东西都变化了，所以把搞好身体，苟到最后，后面还有很多可以搞的事情的
	- 接纳快速转向的事实，即便是 Linux kernel ，编译器，CPU 设计这种已经稳定发展很久的东西，也不是高枕无忧的。


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
