tty 中的文件是如何使用的?
在 ssh 等

不知道为什么 tty0 现在不会输出日志，直接进入 console 了，
继续看看是为什么?

## 看看
搜索这里的 hv
https://www.qemu.org/docs/master/system/qemu-manpage.html

## 为什么在 ssh 中启动，他可以知道这个 gtk initialization failed
🧀  /home/martins3/hack/vm/2403-nix/cmd.sh
gtk initialization failed

## 看看 Ctrl Alt Delete 是如何支持的，在 hvc 也可以吗?

## /dev/vcs 是什么?

## 有办法把 shell 和 tty 分开吗？
在 vscode 中打开一个 shell ，那个会增加一个 pts 吗?

## 原来 hvc0 是 legacy 啊
printk: legacy console [hvc0] enabled

## 调查一下 alacritty 之类的 terminal emualtor 和 terminal 的关系支持

才两万行，而且还支持三个平台

## console 和 Suspend 也是有关系的
printk: Suspending console(s) (use no_console_suspend to debug)

## kvm 的行动，设置 guest os 的配置
kvm_arch_suspend_notifier -> kvm_set_guest_paused

## 发现了一个事情，uefi 是可以把 grub 显示到 stdio 的
但是 seabios 不可以，只有内核启动的之后， serial 才有显示。

## 原来 tty 中还有这个
- __do_SAK 中有 : do_each_pid_task

- tty_signal_session_leader 调用了这个 : do_each_pid_task

## 用了 UEFI 启动，似乎不需要配置 serial=ttyS0 ，stdio 就是有输出的

## top : 按 f ，展示 TTY

似乎前台的都是在  pts 下 ，但是很多都是问号


## 这个目录中内容看看 drivers/video/console/

drivers/video/console/vgacon.c

这个设备中有哪些 QEMU 可以模拟，哪些设备
在物理机中还可以找到

## DELL 服务器的 bios 设置中有 serial 相关设置


## bash
https://github.com/reubeno/brush

## 这个做什么的
https://github.com/tio/tio

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
