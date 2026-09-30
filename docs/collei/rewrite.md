# 使用 codex 重写 collei

<!-- 906dbf3d-cf38-459d-97ee-dc4cf299a593 -->

## why 要重写

### bash vs python

和 python 做对比:

为什么最开始我会不喜欢 python ?

- 源自于曾经的痛苦记忆
- 我 python 无法构建起来环境
- bash 可以更好的复用

python 的好处:

- 自动的 backtrace 机制
- 更加容易抽象，让我发现了 collei.py 中，不同的安装启动模式就是一个 class 了
- 可以不用依赖 gum

bash 的问题

- bash 就不是为大型项目设计de，我感觉超过 100
  行就不行了，但是我在重写的时候，项目已经到 5000
  行作用了，其中的库依赖问题简直就是灾难。

显然，作为一个调试项目，我是不愿意仔细手写这个项目的，用 AI 写，python 比 bash
好很多， 虽然我已经算是勉强驾驭 bash ，但是 AI 总是可以写出来非常难懂的语法:
- https://danluu.com/pl-tokens/

## 好处

似乎有一些问题一直很难解决:

vm_dir 的生命周期:
- vm_root 读取 `~/.config/collei/config.ini` 的 `vm`：collei/scripts/config.py
- 默认 VM 读取同一文件的 `default_vm`：collei/scripts/config.py
- 指定 `-n yyds` 时使用 `vm_root / "yyds"`；未指定时使用默认 VM： collei/scripts/runtime.py 中 `ColleiContext.vm()`
- `-n` 或 `-s` 会更新 `default_vm`：collei/scripts/collei-action.py context =
  ColleiContext.load()
- collei.py 启动 VM 时读取默认 VM：collei/scripts/collei.py 中 `main()`

s / t : 虚拟机虚拟机的状态

长期无法完成的工作用 python 实现起来很容易

## 关于重写

在 AI 出现之前就存在用 Rust 重写项目的趋势，现在这个趋势更强了:
https://news.ycombinator.com/item?id=34588340

https://bun.com/blog/bun-in-rust

- https://andrewkelley.me/post/my-thoughts-bun-rust-rewrite.html

如果真的如此，那么说 GPT 5.5 和 claude fable 的差别就太大了，后面我还发现了 ai
实现的很多问题，不过，这只是 GPT 5.5

我当时在重写这个项目的时候，其实我还没意识自己是这个洪流中一滴水:

> https://www.ruanyifeng.com/blog/2026/09/weekly-issue-411.html
>
> 根本没人会为当前使用的堆栈辩护，大家争前恐后拥抱最佳实践。
>
> 他的结论就是："这样下去，除了少数例外，每一层最终都会变为当下最流行的解决方案。前端趋向
> React，系统趋向 Rust，脚本趋向 Python，甚至连页面本身都趋向
> Next.js，完全无视适配性。"

我花费了很长的时间来掌握 bash ，之所以 collei 用 bash 实现，因为很多时候，我都是先在命令行中执行了命令，
然后我想要直接集成到脚本中，如果用 python 写，这个从 bash 到 python 是一些思考上的开销，但是现在存在了 python ，
这都不是问题。

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
