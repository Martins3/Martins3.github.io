# windows 驱动开发

## 这个是最好的入口
- https://github.com/microsoft/Windows-driver-samples
- https://learn.microsoft.com/zh-cn/windows-hardware/drivers/gettingstarted/writing-a-very-small-kmdf--driver
  - 从 VS 的安装到内核
  - 适合所有的程序员的概念，对于 Linux 内核工程师来说，也是不错的

1. VS 无法安装所有的东西，其中 WDK 就是需要手动安装的
2. 调试器也安装一下: winget install Microsoft.WinDbg


## 看看这些东西吧
- https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/
- https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-wdm
- https://learn.microsoft.com/zh-cn/windows/dev-drive/

## 这个正好是我们需要的
https://news.ycombinator.com/item?id=41490290

## 社区
https://community.osr.com/c/windbg/5

## windows Internals 的 pdf
https://empyreal96.github.io/nt-info-depot/Windows-Internals-PDFs/Windows%20System%20Internals%207e%20Part%201.pdf

## rust 的
https://github.com/microsoft/windows-drivers-rs



## 原来 windows 中甚至可以写一个自己的驱动的
- https://github.com/dokan-dev/dokany

- 用户态文件系统 (FUSE)：Windows 上可以用 Dokan（一个类似 FUSE 的框架），在用户态实现文件系统逻辑，避免内核编程。
- 基于现有文件系统：在 NTFS 上实现一个过滤驱动，添加自定义功能（如加密、日志记录）。
- RAM 盘：实现一个简单的内存文件系统，用于临时存储。

https://github.com/winfsp/winfsp
构建需要使用 : build\choco\ ，不知道是什么东西了
https://community.chocolatey.org/packages/visualstudio2022buildtools


## 这里开始可以顺便解决问题
https://virtio-win.github.io/Development/Building-the-drivers-using-Windows-11-24H2-EWDK


## 理解一下 windows driver 的问题
git log --grep "viostor" mm249..mm260

VirtIoStartIo
```txt
                    case StorStopDevice:
                        adaptExt->stopped = TRUE;
```

VirtIoAdapterControl 中的代码实现靠什么东西?
```txt
        case ScsiStopAdapter:
            {
                RhelDbgPrint(TRACE_LEVEL_VERBOSE, " ScsiStopAdapter\n");
                if (adaptExt->removed == TRUE || adaptExt->stopped == TRUE)
                {
                    RhelShutDown(DeviceExtension);
                }
                if (adaptExt->stopped)
                {
                    if (adaptExt->pmsg_affinity != NULL)
                    {
                        StorPortFreePool(DeviceExtension, (PVOID)adaptExt->pmsg_affinity);
                        adaptExt->pmsg_affinity = NULL;
                    }
                    adaptExt->perfFlags = 0;
                }
                status = ScsiAdapterControlSuccess;
                break;
```

## 为什么 virtio scsi 没有问题?

```txt
[vioscs] fix multi-queue support for cases when number of CPUs and number of virtual queues are not matching
```

## rust windows 内核驱动
https://news.ycombinator.com/item?id=42984457

https://github.com/basil00/WinDivert
https://github.com/desowin/usbpcap

## 看看这个

- Software Driver
  Microsoft/Windows-driver-samples/general/toaster
- File System Filter Driver
  Windows-driver-samples/filesys/miniFilter
  https://github.com/dokan-dev/dokany
- File System Driver

### Windows-driver-samples
.\Build-AllSamples.ps1 目前的环境就是构建所有的

cd C:\Users\97936\data\Windows-driver-samples\filesys\fastfat
参考 readme 就可以构建

这个方法不行
msbuild /t:clean /t:build /t:ClangTidy  .\\fastfat.vcxproj

也不是都不行:
msbuild /t:clean /t:build /t:ClangTidy  .\devcon.vcxproj

这个 devcon 还是不错的
C:\Users\97936\data\Windows-driver-samples\setup\devcon\README.md

### [ ] 下一步，尝试一下 ClangTidy 是不是其他的简单的项目就可以了



### [ ] UMFD 和 KMFD 都是什么东西?


https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-wdm

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
