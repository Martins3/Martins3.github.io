## 收集一些和 rust 有关的项目

- [LWN：新的容器文件系统 PuzzleFS](https://mp.weixin.qq.com/s/3ErrdCi78G7QrEdnUe9Mpw)
  - 这里的关于很多问题的讨论其实很有意思，例如压缩，索引，tar 之类的
  - 这个项目现在是 fuse 的形式，但是想要合并到主线中，还需要等待 rust 的接口的实现
  - https://github.com/project-machine/puzzlefs

## 搭建 rust 的环境
- https://wusyong.github.io/posts/rust-kernel-module-01/

## 这里提到了很多资源
- https://lwn.net/Articles/990619/
- https://news.ycombinator.com/item?id=41875792

## qemu 也开始跟进了
- https://wiki.qemu.org/RustInQemu
- https://news.ycombinator.com/item?id=24133128 : 很早的讨论

## 真复杂啊
https://mp.weixin.qq.com/s/WozICJf39UMvx9cYzAG0Ow

## 用户态的支持
https://github.com/libbpf/libbpf-sys

## 基本的测试
居然需要把这两个去掉:
```txt
CONFIG_MODVERSIONS=y
CONFIG_ASM_MODVERSIONS=y
```

之前真的忘记了，到底为什么设置发上这两个

而且，作用是什么？

## 看看这个
https://mp.weixin.qq.com/s/XngV6_kG6IZS0Vzm49vfZg

## 思考一下，rust 带来了什么问题?

编译上的所有的问题:

ebpf / kprobe / ftrace / link (percpu)

### 将其中的大讨论整理一下

## 很好的总结
https://news.ycombinator.com/item?id=42253814

## 总结
https://www.infoq.cn/article/jeiFQpEtCg47ZWBK8IJ3

## 环境搭建
其实没那么难，如果各种环境都升级了之后:
https://github.com/Rust-for-Linux/rust-out-of-tree-module
https://github.com/mboyar/docker-dev-env-linux-kernel-rust/blob/main/x86_64/Dockerfile


参考这里的命令 ? : https://github.com/mboyar/docker-dev-env-linux-kernel-rust/blob/main/x86_64/Dockerfile

## 天天讨论
https://news.ycombinator.com/item?id=43101204
https://lore.kernel.org/rust-for-linux/CAHk-=wgLbz1Bm8QhmJ4dJGSmTuV5w_R0Gwvg5kHrYr4Ko9dUHQ@mail.gmail.com/

## 按照这个搭建环境还是不错的
https://mp.weixin.qq.com/s/vfA2PfBl6uVksWQ39SPAkw


## 这个看看
入门rust是读rust语言圣经好还是读微软的教程好？ - Blackbird的回答 - 知乎
https://www.zhihu.com/question/585031734/answer/3523366806


## 6.18 版本中 binder 修改为 rust 了

commit eafedbc7c050 ("rust_binder: add Rust Binder driver")k
https://lore.kernel.org/all/20250919-rust-binder-v2-1-a384b09f28dd@google.com/

## 有趣的
https://mp.weixin.qq.com/s/flQRGWUQDhDea56cTDSNOg

- https://security.googleblog.com/2021/04/rust-in-linux-kernel.html

## 也许可以测试下这个 binder 工具
commit eafedbc7c050 ("rust_binder: add Rust Binder driver")k
https://lore.kernel.org/all/20250919-rust-binder-v2-1-a384b09f28dd@google.com/

## debugfs 的 rust 接口

2026-01-02 已经合并了:
https://mp.weixin.qq.com/s/QFi-_Xu4fLseqEL7Rs76Bg

## tyr
https://lwn.net/Articles/1055590/

## lwn
https://mp.weixin.qq.com/s/DH_bSrQ-Upfpckl9w1Muwg

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
