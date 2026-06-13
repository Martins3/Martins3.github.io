## windows 的内存理解下如何使用

1. 看看 windows 的 kdump 来调试一下内存的问题
2. windows 的 dmesg 日志在哪里
3. windows 内核模块的调试方法是什么?

## memory
![](./img/windows-memory.png)

1. 如何理解 "已提交" ，超过了物理内存的大小?
  - 而且是两个数值
2. 分页和不分页是什么意思
3. 已使用中，已经压缩的部分是什么意思
  - 是压缩前占据了这么多，还是压缩之后占据了这么多的内存
  - 有办法测试这个压缩的效率吗?

## hwmonitor 是如何做到的，linux 下应该也有一个才对啊

不至于没有驱动吧。

hwmonitor 为什么需要管理员权限?

## k8s 可以部署到 windows 上吗?
可以，但是据说有只能运行 windows 容器

他在 windows 上有要求关闭 swap 吗?


## podman

非常有意思，居然是在 wls2 上的:
file:///C:/Program%20Files/RedHat/Podman/podman-for-windows.html

用用 podman 的界面吧，可以作为一个入门。

而且命令在 powershell 中直接执行，但是可以操作 wsl2 中东西，有趣

大坑: https://github.com/containers/podman/issues/22927

## 可以看看 wireguard 如何在 windows 中写的，也是一个 windows 驱动吧

## 有趣
https://docs.google.com/document/d/1IQ_IgvR5jR1ppfgbiAqLgWiJvVsj52yRIrT3HidyIX0/edit?tab=t.0


## 通过 github 似乎可以访问到 linux 虚拟机在 windows server 上运行
```txt
[    1.141822] hv_vmbus: registering driver hv_pci
[    1.227176] hv_vmbus: registering driver hv_storvsc
[    1.447088] hv_utils: Registering HyperV Utility Driver
[    1.450386] hv_vmbus: registering driver hv_utils
[    1.453708] hv_vmbus: registering driver hv_balloon
[    1.464786] hv_utils: TimeSync IC version 4.0
[    1.467650] hv_utils: Heartbeat IC version 3.0
[    1.470806] hv_utils: Shutdown IC version 3.2
[    1.473962] hv_balloon: Using Dynamic Memory protocol version 2.0
[    2.380983] systemd[1]: Unnecessary job was removed for /sys/devices/virtual/misc/vmbus!hv_vss.
[    2.383836] systemd[1]: Unnecessary job was removed for /sys/devices/virtual/misc/vmbus!hv_fcopy.
[    3.085282] hv_vmbus: registering driver hyperv_keyboard
[    3.085499] hv_vmbus: registering driver hyperv_fb
[    3.085907] hyperv_fb: Synthvid Version major 3, minor 5
[    3.097914] hv_vmbus: registering driver hid_hyperv
[    3.101915] hv_vmbus: registering driver hyperv_drm
[    3.110407] hv_vmbus: registering driver hv_netvsc
[    3.652230] hv_utils: KVP IC version 4.0
[   13.349564] workqueue: hvfb_update_work [hyperv_fb] hogged CPU for >10000us 4 times, consider switching to WQ_UNBOUND
[   49.593526] hv_balloon: Max. dynamic memory size: 8192 MB
[  130.245561] workqueue: hvfb_update_work [hyperv_fb] hogged CPU for >10000us 8 times, consider switching to WQ_UNBOUND
[  134.671564] workqueue: hvfb_update_work [hyperv_fb] hogged CPU for >10000us 16 times, consider switching to WQ_UNBOUND
[  141.814566] workqueue: hvfb_update_work [hyperv_fb] hogged CPU for >10000us 32 times, consider switching to WQ_UNBOUND
[  161.082565] workqueue: hvfb_update_work [hyperv_fb] hogged CPU for >10000us 64 times, consider switching to WQ_UNBOUND
[  198.779559] workqueue: hvfb_update_work [hyperv_fb] hogged CPU for >10000us 128 times, consider switching to WQ_UNBOUND
```
https://github.com/mrexodia/TitanHide

## windows 的任务管理器
https://www.xda-developers.com/powerful-tools-should-use-instead-task-manager/

## windows 源码
https://github.com/tongzx/nt5src

## 包管理器的区别是什么

winget
scoop
nuget
vcpkg

## 似乎 Windows Internals 7th edition 就是官方推荐的
https://learn.microsoft.com/en-us/sysinternals/resources/windows-internals

## windows3 的 png ，这个内存看不懂


## 安装一下 ureal engine


## 从 windows 切入图形，看看 blender 和 direct-X 吧

## windows 中 linux 用的盘是什么？
为什么 windows 中导入的 guest os 无法识别 virtio-blk

是需要使用这个吗?
drivers/hv/vmbus_drv.c

## Navida 的显卡驱动由于不开源结果和 linux 社区闹很不开心，
那这一个问题 windows 是如何解决的啊 ?

## 这个正好看看 WDDM 模式
https://learn.microsoft.com/en-us/windows-hardware/drivers/display/

## 看上去 api 只是提供了 c++ 接口，那么 rust 改如何操作?

尝试在 windows 上使用 rust 看看

## 关于内核的思路
1. vfio ?
3. 如果编译 llvm 也会会产生 sln 吗?

## 核心 api
4. 关于内存
5. process
6. 同步原语
  5. 我还是希望有类似 src/c 这个 API 测试才可以，巨大的 sln 项目对于我来说，很难受
7. 异步 io 模型
  - 存储
  - 网络
8. 异步 io 模型，也是有 epoll 的机制么


有无类似 ioctl 的机制，不然那么多设备该如何解决
例如nvmecli

有无 ebpf ，如果没有，是如何观测内核的
内核日志都是在哪里的?


### 看看 windows 是不是真的不支持 64 个以上的 socket

如果热插的话，真的会导致内核中 cpu 冲高吗？
看看 ntop 这个项目如何构建的

### windows 中为什么总是讲 c 运行时，这个到底指的是什么，对应 linux 中的什么东西

## windows 的并发编程了解一下

相关的 api ，类似 gcc 的 memory model ?

## 给 windows 提供 dummy driver

## 看 windows2.png ，似乎 amd 上没有到这里频率的吧

## write a minimal driver

## cup-z 如何获取 nvidia 的温度传感器的？

## 使用 fio 测试一下文件系统的性能

所以文件系统性能该如何测试来着?

## 看看 windows 是不是有 thread 和 process 的这种区别?

## 看看 ctrl c 可以 kill 程序的过程

## nvim 用的原生的库么?
通过什么形式的?

## 找到 Nvim 启动慢的原因
那个 blame 插件就可以了，似乎和 session 有关，在一个目录第一次打开很慢，但是之后就很快。

## windows io uring
https://github.com/CarterLi/libwinring
https://github.com/CarterLi/liburing4cpp

应该会有更好的项目，不过可以先仔细看看了

## windows 中有 rcu 么?
如果写内核驱动的话，就可以确认了。

不用了，直接可以找到，而且找到了更多的 windows 相关的 api :
https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kercureadlock

## windows 有 io scheduler 么，有什么办法来检查

有 multiqueue 之类的东西么?

## windows 有绑定核心的操作么?

## windows 有内核启动参数么?

## 看看 windows 的设备直通操作
应该是有的

## 该如何测试 windows 中的 hyperv
有那种 minimal 的 windows 的 hypervisor ，我记得

## iouring 相关的

- https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports

- https://learn.microsoft.com/en-us/windows/win32/fileio/network-i-o-concepts

- https://learn.microsoft.com/en-us/windows/win32/fileio/sparse-files

- https://learn.microsoft.com/en-us/windows/win32/fileio/symbolic-links


https://news.ycombinator.com/item?id=38949890

https://news.ycombinator.com/item?id=25223411

https://news.ycombinator.com/item?id=35548537

https://cor3ntin.github.io/posts/iouring/

## marksman 在 windows 可以用么?
但是感觉，总是感觉哪里卡卡的

## 看看这个库如何兼容的 windows 的
https://github.com/libuv/libuv

https://en.wikipedia.org/wiki/Windows_Display_Driver_Model

## 程序员的自由修养中关于 windows 的部分


## 可以从尝试写一个文件系统开始
- https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/
- https://learn.microsoft.com/en-us/windows-hardware/drivers/

可以先看看 windows 的 ntfs 的基本功能

## 想都不用想，windows 有各种驱动工程师，他们其实也需要调试 windows crash 的问题

哦，还是从这里入口
https://github.com/microsoft/Windows-driver-samples


## 学习他的调试技术
为什么飞书开屏动画能占用CPU20% ？ - Losgy浩的文章 - 知乎
https://zhuanlan.zhihu.com/p/1952095442474108549

## windows 对应 perf 的工具叫什么发?

## windows driver 的最佳入口
<!-- f7c573e7-a0ec-43a8-8e31-26ffbf1c0b42 -->

https://rayanfam.com/topics/hypervisor-from-scratch-part-1/

- [ ] 似乎只能远程调试。

而且这个看上去相当不错:
https://github.com/HyperDbg/HyperDbg

有类似的 linux 替代品吗?

## windows 有 ioctl 吗？

## windows 上如何执行 rdma?


## 秒到家了
https://github.com/winsiderss/systeminformer/tree/master

## windows 有类似 tracepoint 之类的 trace 机制么?

有 bpf 类似的技术么?

## dotnet 是类似 jvm 的机制么?

## everything is a file 真的好么?

是不是提供了太多东西

回忆一下 tracefs 之前遇到的过的 bug

## windows 这种跨进程的 sem 如何实现?
- data/vn/code/src/c/shm-posix/sem.c

## 通过 qemu 来 dump windows 11 ，以及 minidump 都是改如何操作


## https://github.com/TibixDev/winboat

## 作为一个基本对比吧
https://news.ycombinator.com/item?id=45589156

## 想不到 powershell 也是开源的
https://github.com/PowerShell/PowerShell

## docker 技术如何在 windows 中实现的?
  - https://learn.microsoft.com/en-us/virtualization/windowscontainers/about/

## https://news.ycombinator.com/item?id=46374413

## 看看这个好用吗?
https://github.com/FancyWM/fancywm


https://news.ycombinator.com/item?id=47022891


## Supplemental open-source components for use in developing device drivers for Windows.
https://github.com/microsoft/Windows-Driver-Developer-Supplemental-Tools

那么 codeql 到底是什么呢?

看看这个:
https://github.com/microsoft/sudo
了解一下机制的关系是什么?

## kimi 回答的解决方案
• 有，但主要是间接使用 Windows 程序的方案，没有提到 Wine/Proton 这类兼容层。该目录下记录的方法包括：
  1. 远程桌面 (rdp.md)
  • winapps-org/winapps — 基于 FreeRDP，在 Linux 上集成运行远程 Windows 应用
  • IronRDP — Rust 实现的 RDP 客户端
  • rustdesk / crossdesk — 远程桌面工具
  2. 虚拟机 (virt.md)
  • QEMU/KVM + GPU/USB 直通 — 在 Linux 上运行完整的 Windows 虚拟机
  • virt-manager 管理工具
  3. Docker 虚拟机 (later.md)
  • dockur/windows — 通过 Docker 容器运行 Windows 虚拟机
  4. 网络共享 (why.md)
  • 两台电脑方案，配合 SMB 共享 互通文件
  缺少的内容
  该目录没有记录以下常见方案：
  • Wine / Proton / CrossOver（在 Linux 本地直接运行 Windows 程序）
  • Bottles 等 Wine 前端工具

## 这个看看
https://github.com/zodiacon/WindowsInternals/tree/master/IoPriority

## 有趣的东西
https://github.com/microsoft/coreutils

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
