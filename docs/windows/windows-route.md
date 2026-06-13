# windows
- https://github.com/hzqst/unicorn_pe

- https://rentry.co/build-win2k3
- https://github.com/j00ru/windows-syscalls : The repository contains system call tables collected from all modern and most older releases of Windows, starting with Windows NT.
- https://github.com/sandboxie-plus/Sandboxie : windows 操作系统上的 sandbox

## 通过 virtio-balloon 来实现 windows 内核编程?

## windows 内核编程

## windows 文档
https://learn.microsoft.com/zh-cn/windows-hardware/drivers/kernel/
- https://github.com/virtio-win : 想不到吧，还存在这种优化


## 调试工具
- https://github.com/HyperDbg/HyperDbg
  - 也是理解虚拟化的项目


## balloon

### 尝试分解一下
- https://stackoverflow.com/questions/6801008/globalmemorystatusex-win32
  - 获取 memory 的基本信息

- windows 的 malloc 直接会导致 memory 的使用量增加的
- windows 中，如果 free memory 分配完了，nalloc 就会失败，不会去压缩 page cache 的


### windows 内存管理
- https://learn.microsoft.com/en-us/troubleshoot/windows-server/performance/ram-virtual-memory-pagefile-management

In Windows systems, these paged out pages are stored in one or more files (Pagefile.sys files) in the root of a partition.
There can be one such file in each disk partition. The location and size of the page file is configured in **System Properties** (click **Advanced**, click **Performance**, and then click the **Settings** button).

- 在 windows 中如何查看 cache 的大小:
  - https://superuser.com/questions/793304/how-to-increase-swap-memory-in-windows
    - 输入 SystemPropertiesAdvanced.exe 即可
  - [ ] 存在疑惑的?

## 小工具
- https://github.com/winsiderss/systeminformer
- https://github.com/files-community/Files

## 有点好奇，这个 rust 项目没有使用 vs 构建
https://github.com/microsoft/sudo?tab=readme-ov-file

## 这个工具也是需要的
https://learn.microsoft.com/en-us/windows-hardware/test/wpt/windows-performance-analyzer

## 有趣的套路
https://www.reddit.com/r/linux/comments/1ecbon7/what_does_windows_have_thats_better_than_linux/

## win gdb
https://superuser.com/questions/1198896/how-to-install-windbg-on-a-pc-without-internet-connection

## 可以这个，似乎很多
https://learn.microsoft.com/en-us/windows/dev-drive/

## 有趣的回答
https://www.zhihu.com/question/40359698/answer/1939722422694683007

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
