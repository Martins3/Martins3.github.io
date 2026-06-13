# 我用 ai 写了一个文件系统

准确来说，

首先，这个不是从头开发的，不过我感觉影响也不到

## 为什么?
1. 文件系统极其复杂，
内核大多数黑魔法其实都是和文件系统相关，尤其是同步机制
2. 我在早期学习 linux 内核的时候，记录了大量关于 fs 的疑问，这些东西始终没有得到解决。
3. 这个例子是非常经典的，其实外部约束是非常强的了

## 第一个阶段
人肉工作，只能完成兼容性的文件，而且还是各种挤压时间才搞的。

部分替换掉 iomap

## 第二阶段 : 用 qwen cli 工具

升级之后:

出现了 bug ，这个 bug 非常诡异，就是
然后完全无法调试。
我也不会，ai 也不会，而且 ai 在哪里瞎扯。

## 第三阶段 : kimi + claude code

第一部分 :
1. 我发现 kimi 写了  800 行的脚本来运行 xfstests 的测试，我看了之后，简化到 100 行
2. 不知道为什么，agents 总是容易忘记 nfs 的配置

claude 4.6 opus
已经感受到 kimi 其实有点不行了。

## 第四阶段 : slock -> claude + codex
他们真的很棒

## 总结反思

我学到了什么:
1. ai 基本上，人类会的东西，他都是会的，也就是我们现在没必要把时间浪费构建类似的东西上了。

但是，如果是我自己来写的话，当然，我对于 fs 会有更好的理解，但是问题是，
这个问题推进到现在，我一共就花费不到一个小时:

的确，手动写代码会有很多好处，自己悟也会有很多好处，但是随着我完各种游戏的经验
总结，其实抄其他的教程还是最快的。

很多东西似乎没有细看，然后 ai 就直接做完了，我中间都不知道 ai 做了什么:
1. fiemap : 这个是做什么的?
2. mmap + fallocte : 如果是写 fuse ，需要考虑这个问题么?
3. journal 到底如何实现的
4. fs 如何处理 scsi_debug
5. fs 应该如何创出 dm flakey 的


第一个阶段:
搭建好可以循环的环境是重要的，让 ai 就像人一样(曾经的人)来写代码，
遇到文件，利用现有的知识，解决问题。


我总共花费大约 500$ 的 token ，主要是 claude code


总结反思:
1. 类似的扩展: fuse 用户态存储? 对象存储？用户态网络栈? 另一个 qemu 的实现?
	- 可以尝试将 dune 移植到 x86 上
2. 不要把时间用到 ai 可以做的事情上?
	- 调查 bug (尤其是可以复现的 bug ，让活人来调查太慢，太弱)
3. 如何减少人的审查量，也就是需要 AI 写的东西是自证明的，自己写测试，或者利用现有的测试

(很奇怪，ai 不会自动的检查环境，为什么可以加速事情推进的工作，但是 ai 不会去做)

不得不说，文件系统的确复杂: 后面的高级功能全部没实现
1. 动态扩缩容
2. 加密

类似的东西:
https://github.com/wangrunji0408/iJiegeOS

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
