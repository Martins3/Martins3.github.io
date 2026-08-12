# windows 驱动开发

## 2026-08-10：Windows 11 build 26200 开发环境

已经在 `Win11_24H2_Chinese_Simplified_x64` 上完成：

- Visual Studio Community 2026 `18.8.2`；
- Windows SDK `10.0.28000.2114`；
- Windows WDK `10.1.28000.2526`；
- WinDbg `1.2606.22001.0`；
- 官方 `Windows-driver-samples` 仓库；
- KMDF Echo 示例的 x64 Debug 完整构建、INF/API 验证、catalog 生成和测试签名。

可重复部署和构建入口位于 dotfiles：

```powershell
pwsh -File "$HOME\.dotfiles\scripts\windows-driver-setup.ps1"
pwsh -File "$HOME\.dotfiles\scripts\windows-driver-build.ps1"
```

关键问题：必须使用 Visual Studio 2026 的 `MSBuild\Current\Bin\amd64\MSBuild.exe`。
32 位 MSBuild 会让 WDK 28000 查找不存在的 x86 `InfVerif.dll`，并使 `ApiValidator` 中的
`aitstatic` 返回 193。64 位 MSBuild 构建成功。

自有最小 KMDF 驱动位于 `docs/windows/driver/code/kmdf-hello`，实现 `DriverEntry()` 和
`KmdfHelloEvtDeviceAdd()`，用于脱离官方示例验证源码、INF、catalog 和测试签名链路。

第一次构建被 INF verification 1199 拒绝：使用 Driver Store `DIRID 13` 时，manufacturer
decoration 不能只写 `NTamd64.10.0`，必须限制为 `NTamd64.10.0...16299`。修正后构建为
0 error、0 warning。

生成的 `KmdfHello.sys` 是 x64 Native subsystem PE，SYS 和 CAT 均带 `WDKTestCert` 测试签名。
开发机没有信任自签名根，因此独立 `SignTool verify /kp` 返回 untrusted root；证书信任与
`TESTSIGNING` 留到带快照的 target VM 中配置。

该开发机本身已经是 QEMU Q35 guest，`HypervisorPresent=True`，所以 target VM 应优先由
外层 QEMU/libvirt 环境创建，而不是假定可以使用嵌套 Hyper-V。

### 尚未执行：在测试 VM 中安装和调试

当前状态停在“构建成功，SYS/CAT 已由 `WDKTestCert` 测试签名”。驱动尚未安装或加载。后续
需要执行会改变目标机信任、启动和内核状态的操作，因此留待有时间并且已经准备好 VM 快照与
控制台时再做：

1. 从 catalog 导出本次构建的 `WDKTestCert`，在 target 的 Local Machine `Trusted Root`
   和 `Trusted Publishers` 中信任它；
2. 执行 `bcdedit /set testsigning on` 并重启 target；
3. 先连接 kernel WinDbg，以便捕获 `DriverEntry()`；
4. 构建官方 samples 中的 x64 DevCon，执行 `devcon install kmdf-hello.inf
   ROOT\MARTINS3_KMDF_HELLO`，创建 root-enumerated device 并安装驱动；
5. 用 `Get-PnpDevice`、PnPUtil、`sc.exe`、`driverquery`、SignTool 和 WinDbg 验证设备、服务、
   签名、内核模块及 `KdPrintEx()` 输出；
6. 测试后恢复快照，或精确删除设备、Driver Store package 和测试证书，关闭 `TESTSIGNING`
   后重启。

依赖下载、证书导出、完整命令、成功标准与手工回滚步骤统一维护在
`~/.dotfiles/docs/windows-driver.md` 的“将 KmdfHello 安装到测试 VM”章节。当前 WDK 28000
没有预装 `devcon.exe`；详细文档记录了如何从
`~/data/Windows-driver-samples/setup/devcon/devcon.vcxproj` 构建，不能只写一句“运行
DevCon”而遗漏这个依赖。

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

## 可以看看 wireguard 如何在 windows 中写的，也是一个 windows 驱动吧

https://github.com/mrexodia/TitanHide

## windows driver 的最佳入口
<!-- f7c573e7-a0ec-43a8-8e31-26ffbf1c0b42 -->

https://rayanfam.com/topics/hypervisor-from-scratch-part-1/

- [ ] 似乎只能远程调试。

而且这个看上去相当不错:
https://github.com/HyperDbg/HyperDbg

有类似的 linux 替代品吗?

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
