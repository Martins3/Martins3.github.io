# 使用 codex 重写 collei

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
- bash 是适合写大型项目，我感觉超过 100 行就不行了，但是我在重写的时候，项目已经到 5000 行作用了，其中的库依赖问题简直就是灾难。

## how

## 好处

似乎有一些问题一直很难解决:

vm_dir 的生命周期:
  - vm_root 读取 ~/.config/collei/vm：collei/scripts/config.py
  - 默认链接读取 ~/.config/collei/last：collei/scripts/config.py
  - 指定 -n yyds 时使用 vm_root / "yyds"；未指定时解析默认链接：collei/scripts/runtime.py: context.vm()
  - -n 或 -s 会同时更新默认 VM 链接：collei/scripts/collei-action.py
        context = ColleiContext.load()
  - collei.py 启动 VM 时直接使用默认链接：collei/scripts/collei.py  (main ->         vm = context.vm() )

s / t : 虚拟机

长期无法完成的工作用 python 实现起来很容易

无法测试

## 问题

1. 莫名其妙的抽象:
```txt
class ActionContext:
    collei: ColleiContext
    vm: VmRuntime
    runner: CommandRunner
    auto_yes: bool = False

    def ssh_info(self) -> tuple[str, str, int | None]:
        user = self.vm.config.options.get("user") or "root"
        ip = self.vm.config.options.get("ip")
        if ip is not None:
            return user, ip, None
        return user, "localhost", self.vm.tcp_port("ssh")
```

## 其他内容
https://bun.com/blog/bun-in-rust

如果真的如此，那么说 gpt 5.5 和 claude fable 的差别就太大了。

https://andrewkelley.me/post/my-thoughts-bun-rust-rewrite.html

## TODO
1. 似乎存在太多 context.vm.directory / context.vm.which_qemu

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
