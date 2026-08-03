# 代办
clk 和 kvm forum 的东西的确是需要看的
以及 2025 的 plumber 的

vhost 为什么会和 userfault 冲突?

docs/rust/rust/question.md 可以用 anki 走一遍，其实就比较容易了

真的好想看看 virtio-gpu 相关的东西，的确可以从这里入手，然后到物理的 GPU ，最后发可以把网上的所有的 GPU 都买一下

学术追星是什么感受？有没有学术/行业偶像让你深受启发的？ - DBinary的回答 - 知乎
https://www.zhihu.com/question/1978849576766104290/answer/1978875046257784260


长期代办:
- 显然，rcu + fs 才是重点
    - 在 fs 之前，kvm 相关的东西都整理清楚吧
    - 从简单的开始，继续写 demo
    - 再研究一下 bcachefs 的文档，xfs 的文档和 ext4 的文档
- 这些事情感觉都是互相没什么关联，其实收益很小，而且把重要的事情都耽误了。

- showcase-linux-commits-search 让 ai 来调试一下吧

docs/concurrent/perfbook/words.md : 需要一个工具来获取这些东西的读音

https://docs.ros.org/en/jazzy/Installation.html : 这个是可以看看的

## 尝试复现一下这个工作
https://blog.vmsplice.net/2026/01/

## alpine.md 中的 setup_machine 可以修改一下?

## 现在，也许我们可以换一个思路来学习 windows 了
开始文档优先 (把那几个 pdf 都搞到，但是我现在没法看代码，也是痛苦d)


## 现在构建的内核中，13900k 上是不支持 pcm 的

此外发现了一些日志:
```txt
[Thu Jan 22 13:24:49 2026] msr: Write to unrecognized MSR 0x38f by i7z (pid: 82877). // i7z 导致的
[Thu Jan 22 13:24:49 2026] msr: See https://git.kernel.org/pub/scm/linux/kernel/git/tip/tip.git/about for details.

[Thu Jan 22 13:25:20 2026] NMI watchdog: Enabled. Permanently consumes one hw-PMU counter. // pcm 导致的
[Thu Jan 22 14:30:24 2026] NMI watchdog: Enabled. Permanently consumes one hw-PMU counter.
```

千真万确的啊，切换为 6.18.5-100.fc42.x86_64 之后就可以了:
```txt
 Core C-state residencies: C0 (active,non-halted): 0.25 %; C1: 4.37 %; C3: 0.00 %; C6: 49.56 %; C7: 45.82 %;
 Package C-state residencies:  C0: 43.72 %; C2: 56.28 %; C4: 0.00 %; C6: 0.00 %;
                             ┌────────────────────────────────────────────────────────────────────────────────┐
 Core    C-state distribution│11166666666666666666666666666666666666666667777777777777777777777777777777777777│
                             └────────────────────────────────────────────────────────────────────────────────┘
                             ┌────────────────────────────────────────────────────────────────────────────────┐
 Package C-state distribution│00000000000000000000000000000000000222222222222222222222222222222222222222222222│
                             └────────────────────────────────────────────────────────────────────────────────┘
```

## 不用 virtio 还不可以使用 xen 吗?

## 测试一下这个东西
```c
static inline void vm_flags_set(struct vm_area_struct *vma,
				vm_flags_t flags)
{
	vma_start_write(vma);
	ACCESS_PRIVATE(vma, __vm_flags) |= flags;
}
```
C 语音也是可以实现 private method 的

## 有时间看看这个吧
- [ ] https://linux-kernel-labs.github.io/refs/heads/master/lectures/debugging.html
- [ ] https://unix.stackexchange.com/questions/91854/whats-the-difference-between-a-kernel-oops-and-a-kernel-panic

https://github.com/fanyang89/bpftrace-formatter

## mini qemu
https://github.com/mistivia/mvvmm

## 尝试搞搞各种 blog 的订阅
https://gaocegege.com/Blog/

## bitwarden 也可以passkey

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
