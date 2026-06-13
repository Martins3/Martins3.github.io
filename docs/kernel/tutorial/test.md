# Linux 测试

## 收集下 Linux 的各种测试
- https://github.com/osandov/blktests
- 自己搭建一下 google fuzzing bot 出来
- intel 的 github 上部署了一个测试
- phronix 似乎也是存在一个自动的性能测试的
- ltp 测试

我记得有一篇 lwn 分析过为什么测试 Linux kernel 很难

### 有时间应该看看这个

- https://stackoverflow.com/questions/3177338/how-is-the-linux-kernel-tested
- https://www.kernel.org/doc/html/latest/dev-tools/
  - https://docs.kernel.org/dev-tools/testing-overview.html#
  - 很遗憾，大多数工具都很一般
- https://github.com/intel/lkp-tests
- https://news.ycombinator.com/item?id=33742130

## Coccinelle : 没太看懂
- https://www.usenix.org/system/files/conference/atc18/atc18-lawall.pdf
- https://bottest.wiki.kernel.org/coccicheck
- https://mp.weixin.qq.com/s/6wB62p4F3KBkldD-4ZL8lw : 将其用于 Rust ，简单的先看懂吧

## 收集一个开发环境
https://github.com/kernel-cyrus/lightbox

## 居然有人专门跟踪 Linux 的 regression ，实在是有趣
- https://linux-regtracking.leemhuis.info/post/frequent-reasons-why-linux-kernel-bug-reports-are-ignored/
  - https://news.ycombinator.com/item?id=40514033

## lkvs
https://www.openeuler.org/zh/blog/20240306-lkvs/20240306-lkvs.html

## lvm2 的测试
https://github.com/lvmteam/lvm2/blob/main/test/shell/integrity-caching.sh


## 网络的测试
https://lore.kernel.org/all/20230719140858.13224-1-daniel@iogearbox.net/

> We tested this series with tc-testing selftest suite as well as BPF CI/selftests. Thanks!

## 思考

到底什么样子的测试才是好的测试?

单元测试比 gdb 好，但是好不了多少，因为单元测试也是将思维放到细节上了。

## unitest 参考
- core/linux/fs/ext4/inode-test.c

## selftest 实际上更加有趣啊
/home/martins3/core/linux/tools/testing/selftests/kvm/x86_64/hwcr_msr_test.c

## 总体来说
测试只能用来避免低级错误，抽象代码架构可以避免高级错误:

1. 对于 syscall 测试，内存和调度器，依赖用户态工具
1. 对于被依赖的内核模块，依靠社区大家都参与进来，因为其输入是其他的内存模块，例如 block layer
2. 对于内核模块的测试，更多是依赖 selftests ，从用户态来保证其覆盖率，因为其交互来源都是用户态

## io uring 的测试在 liburing 中
但是 aio 就没有测试了，我猜测，是因为 aio 比较简单，而且 aio 太简单了

## 参考下行业经验
https://mp.weixin.qq.com/s/H4NbhK_yEZYRR78waudBGA

## 特定模块的测试
- https://github.com/kdave/xfstests

## bcachefs 的测试文件系统的 workflow
- https://github.com/koverstreet/ktest

## 测试
https://github.com/kernelslacker/trinity

https://github.com/rapido-linux/rapido


原来图形领域本来就存在一个 ci
- https://lwn.net/Articles/972713/
- https://www.collabora.com/news-and-blog/blog/2024/02/08/drm-ci-a-gitlab-ci-pipeline-for-linux-kernel-testing/#:~:text=A%20GitLab%2DCI%20workflow%20presents,shared%20resources%20and%20community%20collaboration.

## 如果我自己作为一个 kernel model 的开发者，需要打开那些 CONFIG 来辅助测试

# 收集一些和测试相关的内容
- https://blog.cloudflare.com/a-gentle-introduction-to-linux-kernel-fuzzing/

- https://www.youtube.com/watch?v=yJw9Avhy_vc

## 原来 redhat 自己一直都有专业的测试工程师

https://patchwork.kernel.org/project/linux-nfs/patch/1485067624-3659-1-git-send-email-yin-jianhong@163.com/
https://cn.linkedin.com/in/yinjianhong

## nfs 测试
https://git.linux-nfs.org/?p=cdmackay/pynfs.git;

## 这里查漏补缺一下
```txt
< > KUnit - Enable support for unit tests  ----
< > Notifier error injection
[ ] Fault-injections of functions
[*] Fault-injection framework
[ ]   Fault-injection capability for kmalloc (NEW)
[ ]   Fault-injection capability for alloc_pages() (NEW)
[ ]   Fault injection capability for usercopy functions (NEW)
[ ]   Fault-injection capability for disk IO (NEW)
[ ]   Fault-injection capability for faking disk interrupts (NEW)
[ ]   Fault-injection capability for futexes (NEW)
[ ]   Debugfs entries for fault-injection capabilities (NEW)
[ ]   Configfs interface for fault-injection capabilities (NEW)
[ ] Code coverage for fuzzing
[*] Runtime Testing  --->
[ ] Memtest
[ ] Microsoft Hyper-V driver testing
```

## 两种测试思路

- simulation
- emulate

到底是精确到具体的函数，还是只是需要整个结果相同。

内核是倾向于 emulator 的测试

## 记录一下 c 语言的基本框架
https://github.com/ThrowTheSwitch/Unity

## 有趣的统计很分析
https://pebblebed.com/blog/kernel-bugs : 这个东西有趣的

https://people.kernel.org/metan/towards-parallel-kernel-test-runs

## 内核到底是如何做测试的
https://lwn.net/Kernel/Index/#Development_tools-Testing

## 使用 um 配合 kunit test ，真的 nb 啊
commit 150ec68fa799 ("hfs: introduce KUnit tests for HFS string operations")


## 经典

> 世界最流行的数据库 SQLite，本身代码15.6万行，但是测试用例9205万行，足足大了590倍！
https://sqlite.org/testing.html

## 原来 kunit 是这样的测试的
```txt
python3 tools/testing/kunit/kunit.py run \
    --arch=x86_64 \
    --build_dir=.kunit-drm-sched \
    --kunitconfig=drivers/gpu/drm/scheduler \
    --summary

[10:08:30] Configuring KUnit Kernel ...
Regenerating .config ...
Populating config with:
$ make ARCH=x86_64 O=.kunit-drm-sched olddefconfig
[10:08:40] Building KUnit Kernel ...
Populating config with:
$ make ARCH=x86_64 O=.kunit-drm-sched olddefconfig
```

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
