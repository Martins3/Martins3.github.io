## 为什么我会想到这个
<!-- 93f00b6b-1097-489c-9864-a93b610f228a -->

https://news.ycombinator.com/item?id=46938814

You're not going crazy. That is what I see as well. But, I do think there is value in:
- driving the LLM instead of doing it yourself. - sometimes I just can't get the activation energy and the LLM is always ready to go so it gives me a kickstart

- doing things you normally don't know. I learned a lot of command like tools and trucks by seeing what Claude does. Doing short scripts for stuff is super useful. Of course, the catch here is if you don't know stuff you can't drive it very well. So you need to use the things in isolation.

- exploring alternative solutions. Stuff that by definition you don't know. Of course, some will not work, but it widens your horizon

- exploring unfamiliar codebases. It can ingest huge amounts of data so exploration will be faster. (But less comprehensive than if you do it yourself fully)

- maintaining change consistency. This I think it's just better than humans. If you have stuff you need to change at 2 or 3 places, you will probably forget. LLM's are better at keeping consistency at details (but not at big picture stuff, interestingly.)

我总结想到的就是:
1. 有大致的想法，可以让 ai 快速来做
2. 内容总结
	- 看书
	- 看一个不熟悉的仓库
3. 一些细节的操作的学习

全世界软件程序难度下降了 100 倍

# 异构计算

gpu direct memory

hmm 的管理:
Documentation/translations/zh_CN/mm/hmm.rst

zero copy
用户态驱动?

dma buf 到底有什么关系?

写一个 GPU 和 CPU 的对比?
1. GPU 体系结构的对比是什么
2. GPU 存在类似 perf book 类似的这种分析吗?
4. 继续看看 GPU 驱动都做什么的? (为什么 nova 驱动可以那么简单啊)
	- 不太容易啊

参考一下这个东西:
https://zhuanlan.zhihu.com/p/2024418152105214167 : ai 时代的 os

## GPU scheduler
https://zhuanlan.zhihu.com/p/641331417

到时候看下这个
```txt
struct drm_gpu_scheduler
struct drm_sched_rq
```

CONFIG_DRM_SCHED 原来是关联到这个文件哦

如果你去看看 intel 手册，会发现 system programing 有很大的
一部分都是关于虚拟化的。

但是到了 GPU 时代，尤其是大模型时代，这些东西不在重要，
甚至都是累赘，例如 iommu 会干扰 gpu direct memory 。

相比之下， vdi 的市场也是非常小的，根本需要 vGPU 的

## 想法

你见过对俄罗斯方块很感兴趣的人吗？

或者 1 + 1 感兴趣的?

现在，有了 ai ，内核迅速从 LOL 退化为俄罗斯方块

## 首先，使用的感受
1. vim 插件
2. 写一个内核态的 fs (可以继续尝试下，利用 /goal 不断的优化)
3. dune 的 x86
4. anki
5. 理解 ext4 文件系统
6. 阅读几本我长期想看的书和项目
	- cuda tutorial
	- perfbool
	- eac
7. 重新学习数学和自动化控制原理


现在我的 neovim 或者 tmux crash 的时候，我不会默默忍受，打开 codex 来分析:
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

## 为什么我感觉两边都是对比的
内核的视角，就是云计算的视角了。

1. 编译器 (CPU 编译器和 GPU 编译器)
2. CPU 架构 : SIMD 和 SIMT 的

以前的 infra

- 存储
	- 数据库
	- HBM
	- 内存
	- 内存架构 ( register / L1 / L2 的 share memory 的)
	- 那么为什么，计算需要 SSD 呢？
- 计算
	- GPU (那么 CPU 内部也是喝多相似性)
	- CPU
- 网络 :
	- nfs (pnfs ?)
	- ethernet (只有)
	- RDMA (两个都有吧)
	- 核心中的总线? (和多核相同的吗?)

他们都是超大的，在小的优化都需要人来做

矩阵革命

## 不要做 yes man

文件系统只是无数内容中的一个，不是没有感情的，

自己来思考?

电影解说

https://movie.douban.com/subject/36176155/

## 显然，ai 没有内核难，我错误了，还是很难的，做深入了，还是很难的

1. 用 ai 学 ai
2. 资源更多，讨论的人更多

## emotion

现在，我每天都在思考如何解决 GPU , transformer ，发现之前每天都在研究 qemu 内核的我
如此陌生，切换如此之快，以至于我在，一个月前的我和现在的我会有共同语言吗？

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
