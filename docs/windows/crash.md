# Windows 蓝屏 dump 分析

> [!NOTE]
> 参考神奇海螺的意见，有待验证

用 `cdb.exe`（WinDbg 的命令行版本）无人值守地分析一个内核 dump 的模板命令，
以及用它定位到的一个 0x9F 实例。

## 分析命令

```txt
"C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe" -z C:\Users\97936\090926-5687-01.dmp -y srv*C:\symbols*https://msdl.microsoft.com/download/symbols -c "!analyze -v; !devstack ffffbc8f9611e750; !devobj ffffbc8f9611e750; !irp ffffbc8f96a42a60; q"
```

## 命令行参数

| 部分           | 含义                                                                                                                                                                                                                                          |
| -------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `cdb.exe`      | Console Debugger，WinDbg 的命令行版本（同一套引擎，无 GUI），适合脚本/CI 批量分析                                                                                                                                                             |
| `-z <dump>`    | 打开一个已存在的 dump 文件作为调试目标（不是启动进程）。示例中的文件名是 Windows 蓝屏 dump 的典型命名 `YYMMDD-序号-序号.dmp`，即 09 年 09 月 26 日生成                                                                                        |
| `-y <sympath>` | 符号路径。三段用 `*` 分隔，是 WinDbg 的「符号服务器」语法：第 1 段空 = 默认本地缓存目录；第 2 段 `C:\symbols` = 本地缓存位置（下载过的 PDB 存这里，下次离线可用）；第 3 段 `https://msdl.microsoft.com/download/symbols` = 微软公共符号服务器 |
| `-c "<cmds>"`  | 启动时立即执行引号里的命令串，分号分隔；执行完继续会话（所以末尾要有 `q`）                                                                                                                                                                    |

## `-c` 中的调试命令

- `!analyze -v`：自动分析的核心，`-v` = verbose。输出包括 bugcheck
  码及参数、崩溃时的
  调用栈、栈上被怀疑的驱动、`FAULTING_IP`、`IMAGE_NAME`、`PROCESS_NAME`、`STACK_TEXT`、
  `FAILURE_BUCKET_ID` 等。它自己也会打印出相关设备对象/IRP
  的地址——后面几条命令的地址 就是这么从它的输出里抄出来的。
- `!devstack <DEVICE_OBJECT>`：从某个设备对象出发，把附着（`AttachedDevice`
  链）的整个 设备栈打出来：每层的驱动名、设备对象地址、`!devobj`
  摘要、有没有挂起 IRP。用来判断 这个设备由哪些驱动分层堆叠（比如 磁盘 →
  分区/卷管理器 → 文件系统 过滤器）。
- `!devobj <DEVICE_OBJECT>`：打印单个 `DEVICE_OBJECT`
  的详细字段：`DriverObject`、
  `DeviceExtension`、`DeviceType`、`Flags`、`AttachedDevice`、`CurrentIrp`、引用计数等。
  示例里和上一条传的是同一个地址，说明该地址本身就是设备对象指针：先看它自身，
  再看它所在的整个栈。
- `!irp <IRP>`：打印一个 IRP 的所有 `IO_STACK_LOCATION`：每个栈位置对应的驱动、
  `MajorFunction`、参数，以及 IRP 的 `CurrentLocation`、pending/return
  状态。崩溃常见 原因就是某个驱动在 IRP 处理中出错，所以要把「当时正在处理的
  IRP」抓出来看是谁卡在 哪一层。
- `q`：退出调试器，让 `-c` 命令跑完进程结束（否则 cdb
  会停在提示符等待输入，脚本挂住）。

### 整体流程

`ffff...` 开头是 x64 内核态虚拟地址，说明这是内核 dump。整条命令的顺序是：

1. 加载 dump 并拉取符号；
2. `!analyze -v` 给出崩溃概览，并从输出里定位可疑的设备对象/IRP 地址；
3. 用 `!devobj` / `!devstack` 看该设备对象的层次结构、哪个驱动拥有它；
4. 用 `!irp` 看当时正在下发/挂起的 IO 请求走到了哪一层；
5. 退出。

典型场景：某驱动（存储/过滤/杀软/加密驱动）在 IO 路径上崩了，通过「设备栈 + 当前
IRP 栈位置」确定是自研驱动还是第三方过滤驱动的问题。

### 注意事项
- `-c` 命令串中若含 `!` 和空格，在 cmd 里必须整段用双引号包住（示例已包）；在
  PowerShell 里还要注意引号转义。
- 符号路径没配好会看到 `WARNING: Unable to verify checksum` /
  `*** ERROR: Module load completed but symbols could not be loaded`，此时栈只有地址没有
  函数名，分析价值大打折扣。
- 如果同一份 dump 要反复跑，把符号缓存 `C:\symbols`
  保留下来，第二次可以离线解析。
- 想看更完整的细节可以追加
  `!analyze -show`、`lm`、`kb`、`!thread`、`!process`、`.ecxr` 等；`!devstack` /
  `!irp` 属于「定位 IO 路径」的补充手段，不是每次都需要。

## 案例：0x9F，UsbHub3 电源 IRP 卡死（090926-5687-01.dmp）

### `!analyze -v` 结论

- `BUGCHECK_CODE: 9f`，`DRVPOWERSTATE_SUBCODE: 3`（设备对象阻塞 IRP 过久）
- `IMAGE_NAME: UsbHub3.sys`，`FAILURE_BUCKET_ID: 0x9F_3_IMAGE_UsbHub3.sys`
- `PROCESS_NAME: System`，调试时间 Wed Sep 9 09:55:11 2026 (+08:00)，System
  Uptime 8 天 23:48
- bugcheck 参数：
  - Arg2（阻塞设备栈的 PDO）= `ffffbc8f9611e750`
  - Arg3（`TRIAGE_9F_POWER`）= `fffff8030e259640`
  - Arg4（被阻塞的 IRP）= `ffffbc8f96a42a60`

### 设备栈：`!devstack ffffbc8f9611e750`

```txt
  !DevObj           !DrvObj            !DevExt           ObjectName
  ffffbc8f96b05410  \Driver\HidUsb     ...
> ffffbc8f9611e750  \Driver\USBHUB3    ffffbc8f96a45da0  ...
!DevNode ffffbc8f96b05010 :
  DeviceInst is "USB\VID_0627&PID_0001\68284-0000:00:0d.0-2"
  ServiceName is "HidUsb"
Device object (ffffbc8f9611e750) is for:
 \Driver\USBHUB3 DriverObject ffffbc8f9603b6c0
AttachedDevice (Upper) ffffbc8f96b05410 \Driver\HidUsb
```

### IRP 栈：`!irp ffffbc8f96a42a60`

节选，`>` 为当前栈位置：

```txt
Irp is active with 16 stacks 12 is current (= 0xffffbc8f96a42e48)
>[IRP_MJ_POWER(16), IRP_MN_SET_POWER(2)]
    0 e1 ffffbc8f9611e750 00000000 fffff80311e1c910-ffffbc8f961ba0d0 Success Error Cancel pending
       \Driver\USBHUB3  UsbHub3!HUBPDO_WdmPnpPowerIrpCompletionRoutineForAsynchronousCompletion
        Args: 00000000 00000001 00000001 00000000
 [IRP_MJ_POWER(16), IRP_MN_SET_POWER(2)]
    0 e0 ffffbc8f9611e750 00000000 fffff80311ee1d50-00000000 Success Error Cancel
       \Driver\USBHUB3  hidusb!HumPowerCompletion
 [IRP_MJ_POWER(16), IRP_MN_SET_POWER(2)]
    0 e0 ffffbc8f96b05410 00000000 fffff8037ba124f0-ffffbc8f96a422c8 Success Error Cancel
       \Driver\HidUsb  HIDCLASS!HidpFdoPowerCompletion
 [IRP_MJ_POWER(16), IRP_MN_SET_POWER(2)]
    0 e1 ffffbc8f96b05410 00000000 fffff8037ba124f0-ffffbc8f96a422c8 Success Error Cancel pending
       \Driver\HidUsb  nt!PopRequestCompletion
```

### 解读

- `Args`
  逐项（`Parameters.Power`）：SystemContext=0，Type=1（DevicePowerState），
  State=1（**PowerDeviceD0**），ShutdownType=0。
- 这是一个给 USB 键盘恢复 D0 的电源 IRP，停在 UsbHub3 的 PDO 层
  pending（标志位含 `PendingReturned`），上层 HIDCLASS、hidusb
  的完成例程与电源管理器的 `nt!PopRequestCompletion` 全部在等，永远等不到。

### 被卡设备是 QEMU usb-kbd

- 实例路径 `USB\VID_0627&PID_0001\68284-0000:00:0d.0-2`：
  - `VID_0627&PID_0001` = QEMU 虚拟 HID 设备（usb-kbd 和 usb-tablet 同用此 ID）
  - `0000:00:0d.0` = 该设备所在 xHCI 控制器的 PCI 地址，即 `-device qemu-xhci`
  - `-2` = 控制器 port 2
- 该 VM 当前 QEMU
  命令行（`~/data/hack/vm/Win11_24H2_Chinese_Simplified_x64/s/cmd.sh`）：

### 解决办法

替换为 virtio-keyboard

## 参考

- <https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/-analyze>
- <https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/-devstack>
- <https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/-devobj>
- <https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/-irp>
- <https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/symbol-path>
- <https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/bug-check-0x9f--driver-power-state-failure>

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
