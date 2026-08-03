# 存储一些原始材料

- include/linux/smpboot.h
  - smpboot_register_percpu_thread : 来启动，测试多线程问题的最佳人选啊
- epoll.c 内核中的设备如何实现 epoll

## nic.c

基本使用:
```sh
./mod.sh nic 1
ifconfig sn0 10.0.0.100/24 up

ping -I sn0 10.0.0.2 # 可以走到 snull_tx
```
不知道为什么，如果在 host 来 ping 10.0.0.100 是可以接受到响应的，当然，如果将其他的网卡 disable 掉，最后肯定是 Ping 不通的，但是
会卡到那个地方:
1. sn0 完全无法接受任何信息?
2. sn0 可以接受到，但是无法发送出去?

此外，显然是可以搭配出来，使用两个 ni0 和 sn1 互相通信的，看看 ldd3 的原文吧

# 文件说明
|-----------|-------------------------|
| sparese.c | must_hold               |
| mm-*.c    | 测试每一个 memory model |
| thread.c | 一个简单的 kthread 的封装|

这几个文件的中的内容有点类似，需要合并下
- watchdog.c
- preempt.c
- process_state.c


将 sg iov 和 bio 这种数据结构组织整理一下

## 如何打开 kernel

1. 构建 kernel module
```txt
DEBUG=1 make
```
2. 将 kernel 替换 /home/martins3/hack/vm/Fedora/opt/replace_kernel 中的内容修改为 debug 而不是 build


## qemu 有办法测试 linux-firmware 吗?
https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/refs/tags

## 常看常新的东西
https://github.com/k88hudson/git-flight-rules/blob/master/README_zh-CN.md

https://github.com/trimstray/test-your-sysadmin-skills

## 可以给 snull 配合 qemu 测试一下

## 测试一下 PCIe 设备的热插拔 : drivers/pci/hotplug/pciehp_hpc.c

虚拟机中的热插拔会有会走这个文件吗?

似乎 PCIe 热插拔有不同的路径

## docs/kernel/code/ 下的代码都移动到这里吧

## 快捷参考

1. https://github.com/torvalds/linux/blob/master/samples/kobject/kobject-example.c
2. https://github.com/sysprog21/lkmpg
3. https://github.com/martinezjavier/ldd3
4. https://github.com/sysprog21/simplefs


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
