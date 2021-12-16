## 继续分析代码
- [ ] 如果执行了 illegal instruction，其现象是什么?
- [ ] 那么还可以检查 TLB refill 的入口吗?
  - [ ] 类似 la 的这种总是在虚拟地址上的怎么处理的呀
- [ ] acpi 在 UEFI 中已经支持了，为什么需要在内核中再次重新构建一次
  - [ ] 无论如何，kernel 是需要 acpi 实现电源管理的

- [x] UEFI supports polled drivers, not interrupts.
  - 既然如此，检查一下 UEFI 是如何使用 serial 的
  - 既然我们保证 UEFI 总是被动的使用 driver 的，岂不是，只要保证 UEFI 不去主动 poll，那么设备的状态就不会被修改
- [x] 5.1.1.2 Do not directly allocate a memory buffer for DMA access
  - 在分配这些内存会存在什么特殊的要求吗? 或者或 UEFI 增加什么特殊操作吗?
  - 因为一个 driver 不知道 CPU 的状态
- [x] omvf
  - [x] OVMF 的代码和 DXE 的代码什么关系，是不是主要负责 PEI 的部分啊
    - 感觉上是，OVMF 处理了和架构相关的部分，而 MdePkg 和 MdeModulePkg 都是在处理架构无关的
    - 比如 OVMF 需要处理 reset vector 和 timer 的事情
    - 当然最直接的即使，OVMF 实际上存在大量的和 QEMU virtio xen 相关的代码
  - [x] 那么 Loongson 上有没有这个东西啊
  - [x] 在物理机上的是什么样子的呀
- [x] 各种符号的地址是不是最后由于 physical address 确定的?
  - 是的，而且基本都是靠近物理内存的末端的
```txt
in GenericProtocolNotify 7FEAB91A
in core notify event 7FEAB91A
```

- [x] os loader 是可以加载 os 的，那么 os 那么是需要一个 nvme 驱动的
  - [x] 让我疑惑的内容是，内核实际上在 /boot/bzImage 上，所以，也存在一个 ext4 的 dirver 吗?
  - 似乎 ext4 不是 edk2 支持的，在 2012 7 月还在讨论 https://www.mail-archive.com/devel@edk2.groups.io/msg33956.html
  - 这部分是放到 grub 中间的

## Protocol Event
Implementation of protocols that produce an EFI_EVENT to inform protocol consumers when input is available.[^5]
- [ ] 找到一个这样的例子

https://uefi.org/sites/default/files/resources/Understanding%20UEFI%20and%20PI%20Events_Final.pdf
这里算是一些高级话题吧!

算是一个更加清楚的例子吧，也就是，让 protocol 关联上 event，当 install 的 event 可以被调用的

步骤一:
CoreRegisterProtocolNotify : 添加一个用于提醒的 Event
```c
/*
#0  CoreRegisterProtocolNotify (Protocol=Protocol@entry=0x7fec25f0, Event=0x7f8eeb18, Registration=Registration@entry=0x7fec4790) at /home/maritns3/core/ld/edk2-workst
ation/edk2/MdeModulePkg/Core/Dxe/Hand/Notify.c:108
#1  0x000000007feb9a9a in CoreInitializeImageServices (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:26
1
#2  0x000000007fea98d2 in DxeMain (HobStart=0x7bf56000) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:285
#3  0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#4  _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#5  0x000000007fee10cf in InternalSwitchStack ()
#6  0x0000000000000000 in ?? ()
```
在 CoreInitializeImageServices 中创建 Hook 为 PeCoffEmuProtocolNotify

步骤二:
CoreInstallProtocolInterfaceNotify : 注册的时候，可能会同时 notify 这个 protocol 关联的所有的 Event
```c
/*
#1  0x000000007feac4d8 in CoreNotifyEvent (Event=Event@entry=0x7f8eeb18) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:242
#2  0x000000007fead53e in CoreSignalEvent (UserEvent=0x7f8eeb18) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:591
#3  CoreSignalEvent (UserEvent=0x7f8eeb18) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:553
#4  0x000000007feb4489 in CoreNotifyProtocolEntry (ProtEntry=ProtEntry@entry=0x7f8eee98) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Not
ify.c:32
#5  0x000000007feb5eff in CoreInstallProtocolInterfaceNotify (InterfaceType=<optimized out>, Notify=1 '\001', Interface=0x7f58dc60, Protocol=0x7f58dc80, UserHandle=0x7
fe9ec88) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:476
#6  CoreInstallProtocolInterfaceNotify (UserHandle=0x7fe9ec88, UserHandle@entry=0x7f8eee98, Protocol=0x7f58dc80, Protocol@entry=0x0, InterfaceType=InterfaceType@entry=
EFI_NATIVE_INTERFACE, Interface=Interface@entry=0x7f58dc60, Notify=Notify@entry=1 '\001') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Ha
ndle.c:341
#7  0x000000007feb5f91 in CoreInstallProtocolInterface (UserHandle=0x7f8eee98, UserHandle@entry=0x7fe9ec88, Protocol=0x0, Protocol@entry=0x7f58dc80, InterfaceType=Inte
rfaceType@entry=EFI_NATIVE_INTERFACE, Interface=Interface@entry=0x7f58dc60) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:313
#8  0x000000007feb7073 in CoreInstallMultipleProtocolInterfaces (Handle=0x7fe9ec88) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c
:586
#9  0x000000007f58b14d in InitializeEbcDriver (SystemTable=<optimized out>, ImageHandle=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/U
niversal/EbcDxe/EbcInt.c:547
#10 ProcessModuleEntryPointList (SystemTable=<optimized out>, ImageHandle=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64
/MdeModulePkg/Universal/EbcDxe/EbcDxe/DEBUG/AutoGen.c:196
#11 _ModuleEntryPoint (ImageHandle=<optimized out>, SystemTable=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiDriverEntryPoint/Dr
iverEntryPoint.c:127
#12 0x000000007feba96c in CoreStartImage (ImageHandle=0x7f5a9798 <BasePrintLibSPrintMarker+1091>, ExitDataSize=0x0, ExitData=0x0) at /home/maritns3/core/ld/edk2-workst
ation/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:1653
#13 0x000000007feb18a0 in CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:523
#14 CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:404
#15 0x000000007feaaafd in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:508
#16 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#17 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#18 0x000000007fee10cf in InternalSwitchStack ()
#19 0x0000000000000000 in ?? ()
```
在 InitializeEbcDriver 中的确是 install 了 mPeCoffEmuProtocol 的
```c
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &ImageHandle,
                    &gEfiEbcProtocolGuid, EbcProtocol,
                    &gEdkiiPeCoffImageEmulatorProtocolGuid, &mPeCoffEmuProtocol,
                    NULL
                    );
```

步骤三:
调用 PeCoffEmuProtocolNotify 只是会在此时调用一次
```c
/*
#0  PeCoffEmuProtocolNotify (Event=0x7f8eeb18, Context=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:127
#1  0x000000007feac7a9 in CoreDispatchEventNotifies (Priority=8) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:194
#2  CoreRestoreTpl (NewTpl=4) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Tpl.c:131
#3  0x000000007feb70a5 in CoreInstallMultipleProtocolInterfaces (Handle=0x7fe9ec88) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c
:611
#4  0x000000007f58b14d in InitializeEbcDriver (SystemTable=<optimized out>, ImageHandle=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/U
niversal/EbcDxe/EbcInt.c:547
#5  ProcessModuleEntryPointList (SystemTable=<optimized out>, ImageHandle=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64
/MdeModulePkg/Universal/EbcDxe/EbcDxe/DEBUG/AutoGen.c:196
#6  _ModuleEntryPoint (ImageHandle=<optimized out>, SystemTable=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiDriverEntryPoint/Dr
iverEntryPoint.c:127
#7  0x000000007feba912 in CoreStartImage (ImageHandle=0x7f5a9798 <BasePrintLibSPrintMarker+1091>, ExitDataSize=0x0, ExitData=0x0) at /home/maritns3/core/ld/edk2-workst
ation/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:1654
#8  0x000000007feb1846 in CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:523
#9  CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:404
#10 0x000000007feaaafd in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:508
#11 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#12 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#13 0x000000007fee10cf in InternalSwitchStack ()
#14 0x0000000000000000 in ?? ()
```

## will kernel destroy everything built by uefi
https://edk2-docs.gitbook.io/edk-ii-uefi-driver-writer-s-guide/3_foundation/readme.7/371_applications

当需要释放 UEFI 的影响的时候，将会调用 CoreExitBootServices
- [x] 检查一下内核中是否调用过 ExitBootServices 如果 kernel 直接作为一个 efi 程序启动 启动
- 在代码 drivers/firmware/efi/libstub/x86-stub.c 中的参数为

struct efi_boot_services 和 UEFI 中定义的 EFI_BOOT_SERVICES 是对应的。

## what's happening on segment fault
- [ ] https://edk2-docs.gitbook.io/a-tour-beyond-bios-mitigate-buffer-overflow-in-ue/

似乎，edk2 想要触发 segment 实际上并容易
## Read the doc
- [ ] https://blog.csdn.net/stringNewName
比如跟踪文件操作，最后就到达这个位置了:
/home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.c
但是
```c
///
/// The EFI_FILE_PROTOCOL provides file IO access to supported file systems.
/// An EFI_FILE_PROTOCOL provides access to a file's or directory's contents,
/// and is also a reference to a location in the directory tree of the file system
/// in which the file resides. With any given file handle, other files may be opened
/// relative to this file's location, yielding new file handles.
///
struct _EFI_FILE_PROTOCOL {
  ///
  /// The version of the EFI_FILE_PROTOCOL interface. The version specified
  /// by this specification is EFI_FILE_PROTOCOL_LATEST_REVISION.
  /// Future versions are required to be backward compatible to version 1.0.
  ///
  UINT64                Revision;
  EFI_FILE_OPEN         Open;
  EFI_FILE_CLOSE        Close;
  EFI_FILE_DELETE       Delete;
  EFI_FILE_READ         Read;
  EFI_FILE_WRITE        Write;
  EFI_FILE_GET_POSITION GetPosition;
  EFI_FILE_SET_POSITION SetPosition;
  EFI_FILE_GET_INFO     GetInfo;
  EFI_FILE_SET_INFO     SetInfo;
  EFI_FILE_FLUSH        Flush;
  EFI_FILE_OPEN_EX      OpenEx;
  EFI_FILE_READ_EX      ReadEx;
  EFI_FILE_WRITE_EX     WriteEx;
  EFI_FILE_FLUSH_EX     FlushEx;
};
```
这个东西的注册最后是通过字符串搜索才找到的:
使用其中一个例子:
- CreateFileInterfaceEnv
  - FileInterfaceEnvVolWrite



```c
//
// DXE Core Module Variables
//
EFI_BOOT_SERVICES mBootServices = {

EFI_SYSTEM_TABLE mEfiSystemTableTemplate = {
```
将 mBootServices 注册到 mEfiSystemTableTemplate 上的

似乎执行的入口在: ProcessModuleEntryPointList
- DxeMain

InstallProtocolInterface ?


进行调试的方法:
```c
  DEBUG((DEBUG_INFO, "InstallProtocolInterface: %g %p\n", Protocol, Interface));
```

DXE 设备接受 PEI 阶段的参数
- HOB 数据

https://stackoverflow.com/questions/63400839/how-to-set-dxe-drivers-loading-sequence
> DXE dispatcher first loads the driver that specifed in Apriori file.

## 分析 io stack 从而理解关闭掉 interrupt timer 的效果是什么
根据 beyond chapter 9 中间的 Disk io 和 Block io

```c
/*
#0  FatDiskIo (Volume=Volume@entry=0x7eb09018, IoMode=IoMode@entry=ReadDisk, Offset=514, BufferSize=2, Buffer=Buffer@entry=0x7eb092e8, Task=Task@entry=0x0) at /home/ma
ritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/Misc.c:330
#1  0x000000007ed758f1 in FatAccessVolumeDirty (Volume=Volume@entry=0x7eb09018, IoMode=IoMode@entry=ReadDisk, DirtyValue=DirtyValue@entry=0x7eb092e8) at /home/maritns3
/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/Misc.c:236
#2  0x000000007ed75d41 in FatOpenDevice (Volume=0x7eb09018) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/Init.c:349
#3  FatAllocateVolume (BlockIo=<optimized out>, DiskIo2=<optimized out>, DiskIo=<optimized out>, Handle=0x7eb12b18) at /home/maritns3/core/ld/edk2-workstation/edk2/Fat
Pkg/EnhancedFatDxe/Init.c:67
#4  FatDriverBindingStart (This=0x7ed7bd00, ControllerHandle=0x7eb12b18, RemainingDevicePath=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/En
hancedFatDxe/Fat.c:417
#5  0x000000007feb69f5 in CoreConnectSingleController (RemainingDevicePath=0x0, ContextDriverImageHandles=0x0, ControllerHandle=0x7eb12b18) at /home/maritns3/core/ld/e
dk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/DriverSupport.c:650
#6  CoreConnectController (ControllerHandle=0x7eb12b18, DriverImageHandle=DriverImageHandle@entry=0x0, RemainingDevicePath=RemainingDevicePath@entry=0x0, Recursive=Rec
ursive@entry=1 '\001') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/DriverSupport.c:136
#7  0x000000007feb6bd5 in CoreConnectController (ControllerHandle=0x7eb3a718, DriverImageHandle=DriverImageHandle@entry=0x0, RemainingDevicePath=RemainingDevicePath@en
try=0x0, Recursive=Recursive@entry=1 '\001') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/DriverSupport.c:222
#8  0x000000007feb6bd5 in CoreConnectController (ControllerHandle=0x7ed80c98, DriverImageHandle=<optimized out>, RemainingDevicePath=<optimized out>, Recursive=<optimi
zed out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/DriverSupport.c:222
#9  0x000000007f0533c5 in ConnectRecursivelyIfPciMassStorage (PciHeader=0x7fe9ead0, Instance=0x7ec75998, Handle=0x7ed80c98) at /home/maritns3/core/ld/edk2-workstation/
edk2/OvmfPkg/Library/PlatformBootManagerLib/BdsPlatform.c:1354
#10 ConnectRecursivelyIfPciMassStorage (Handle=Handle@entry=0x7ed80c98, Instance=Instance@entry=0x7ed80428, PciHeader=PciHeader@entry=0x7fe9ead0) at /home/maritns3/cor
e/ld/edk2-workstation/edk2/OvmfPkg/Library/PlatformBootManagerLib/BdsPlatform.c:1312
#11 0x000000007f04c28e in VisitingAPciInstance (Handle=0x7ed80c98, Instance=0x7ed80428, Context=0x7f0532ee <ConnectRecursivelyIfPciMassStorage>) at /home/maritns3/core
/ld/edk2-workstation/edk2/OvmfPkg/Library/PlatformBootManagerLib/BdsPlatform.c:951
#12 0x000000007f0528f9 in VisitAllInstancesOfProtocol (Id=0x7f066700, CallBackFunction=0x7f04c247 <VisitingAPciInstance>, Context=0x7f0532ee <ConnectRecursivelyIfPciMa
ssStorage>) at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/PlatformBootManagerLib/BdsPlatform.c:910
#13 0x000000007f05ffab in PlatformBdsRestoreNvVarsFromHardDisk () at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/PlatformBootManagerLib/BdsPlatform.c:
1539
#14 PlatformBootManagerAfterConsole () at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/PlatformBootManagerLib/BdsPlatform.c:1539
#15 BdsEntry (This=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/BdsDxe/BdsEntry.c:915
#16 0x000000007feaabe3 in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:551
#17 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#18 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#19 0x000000007fee10cf in InternalSwitchStack ()
#20 0x0000000000000000 in ?? ()
```

```c
//
// Template for DiskIo private data structure.
// The pointer to BlockIo protocol interface is assigned dynamically.
//
DISK_IO_PRIVATE_DATA        gDiskIoPrivateDataTemplate = {
  DISK_IO_PRIVATE_DATA_SIGNATURE,
  {
    EFI_DISK_IO_PROTOCOL_REVISION,
    DiskIoReadDisk,
    DiskIoWriteDisk
  },
  {
    EFI_DISK_IO2_PROTOCOL_REVISION,
    DiskIo2Cancel,
    DiskIo2ReadDiskEx,
    DiskIo2WriteDiskEx,
    DiskIo2FlushDiskEx
  }
};
```
- DiskIoReadDisk
  - DiskIo2ReadWriteDisk
    - PartitionWriteBlocks : 似乎在启动的时候会初步的使用一下
      - 为什么感觉是 PartitionWriteBlocks 回去调用 AtaBlockIoWriteBlocks
    - AtaBlockIoWriteBlocks
      - BlockIoReadWrite
        - AccessAtaDevice : 在其中划分为 Blocking mode 和 Non Blocking Mode，实际上，我们选择的是
          - TransferAtaDevice
            - `Packet->Timeout  = EFI_TIMER_PERIOD_SECONDS (DivU64x32 (MultU64x32 (TransferLength, AtaDevice->BlockMedia.BlockSize), 3300000) + 31);` : 计算重新尝试的时间
            - AtaDevicePassThru
              - AtaPassThruPassThru
                - AtaPassThruPassThruExecute
                  - AhciDmaTransfer
                    - AhciBuildCommand
                    - AhciStartCommand
                    - AhciWaitUntilFisReceived
                      - AhciCheckFisReceived
                      - MicroSecondDelay

```c
/*
#0  AccessAtaDevice (AtaDevice=AtaDevice@entry=0x7eb3a398, Buffer=Buffer@entry=0x7e028018 '\257' <repeats 200 times>..., StartLba=StartLba@entry=2685, NumberOfBlocks=N
umberOfBlocks@entry=128, IsWrite=IsWrite@entry=0 '\000', Token=Token@entry=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Ata/AtaBusDxe/AtaPassT
hruExecute.c:794
#1  0x000000007ed9a9e5 in BlockIoReadWrite (This=This@entry=0x7eb3a398, MediaId=MediaId@entry=2114093080, Lba=2685, Token=Token@entry=0x0, BufferSize=<optimized out>,
Buffer=Buffer@entry=0x7e028018, IsBlockIo2=IsBlockIo2@entry=0 '\000', IsWrite=IsWrite@entry=0 '\000') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/
Ata/AtaBusDxe/AtaBus.c:1083
#2  0x000000007ed9ab28 in AtaBlockIoReadBlocks (This=0x7eb3a398, MediaId=2114093080, Lba=<optimized out>, BufferSize=<optimized out>, Buffer=0x7e028018) at /home/marit
ns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBus.c:1120
#3  0x000000007edf3ba2 in QemuFwCfgReadBytes (Size=128, Buffer=0xa7d) at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgDxe.c:124
#4  0x000000007edf393b in ConvertKernelBlobTypeToFileInfo (BlobType=<optimized out>, BufferSize=0x10001 <__sF+65>, Buffer=0x7eb33318) at /home/maritns3/core/ld/edk2-wo
rkstation/edk2/OvmfPkg/QemuKernelLoaderFsDxe/QemuKernelLoaderFsDxe.c:312
#5  0x0000000000000000 in ?? ()


#0  AccessAtaDevice (AtaDevice=AtaDevice@entry=0x7eb3a398, Buffer=Buffer@entry=0x7e67b018 "\360\377\377\377\003", StartLba=StartLba@entry=2049, NumberOfBlocks=NumberOf
Blocks@entry=64, IsWrite=IsWrite@entry=1 '\001', Token=Token@entry=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Ata/AtaBusDxe/AtaPassThruExecu
te.c:794
#1  0x000000007ed9a9e5 in BlockIoReadWrite (This=<optimized out>, MediaId=<optimized out>, Lba=Lba@entry=2049, Token=Token@entry=0x0, BufferSize=BufferSize@entry=32768
, Buffer=Buffer@entry=0x7e67b018, IsBlockIo2=IsBlockIo2@entry=0 '\000', IsWrite=IsWrite@entry=1 '\001') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bu
s/Ata/AtaBusDxe/AtaBus.c:1083
#2  0x000000007ed9aaee in AtaBlockIoWriteBlocks (This=0x7eb3a398, MediaId=2120724504, Lba=2049, BufferSize=32768, Buffer=0x7e67b018) at /home/maritns3/core/ld/edk2-wor
kstation/edk2/MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBus.c:1156
#3  0x000000007edf3c15 in AllocFwCfgDmaAccessBuffer (MapInfo=<synthetic pointer>, Access=<synthetic pointer>) at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/L
ibrary/QemuFwCfgLib/QemuFwCfgDxe.c:170
#4  InternalQemuFwCfgDmaBytes (Control=2, Buffer=0x0, Size=2146033040) at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgDxe.c:384
#5  InternalQemuFwCfgDmaBytes (Control=2, Buffer=0x0, Size=2146033040) at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgDxe.c:348
#6  InternalQemuFwCfgReadBytes (Buffer=0x0, Size=2146033040) at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgLib.c:57
#7  QemuFwCfgReadBytes (Size=2146033040, Buffer=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgLib.c:83
#8  0x0000000000000000 in ?? ()
```
怎么感觉对不上啊!

## 各种 uefi shell 命令对应的源代码
/home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Library
- UefiShellDebug1CommandsLib : edit
- UefiShellDriver1CommandsLib : connect unconnect 之类的
- UefiShellInstall1CommandsLib : install
- UefiShellLevel1CommandsLib : goto exit for if stall
- UefiShellLevel1CommandsLib : cd ls
- UefiShellLevel3CommandsLib : cls echo
- UefiShellNetwork1CommandsLib : ping

## 文件操作
- [x] 实际上，我发现根本无法操纵文件，文件是无法打开的
  - https://krinkinmu.github.io/2020/10/18/handles-guids-and-protocols.html
  - https://stackoverflow.com/questions/39719771/how-to-open-a-file-by-its-full-path-in-uefi

对比 lua 之后，在 inf 中间没有正确引用库导致的

## UEFI shell 可以做什么
甚至差不多集成了一个 vim 进去了
https://linuxhint.com/use-uefi-interactive-shell-and-its-common-commands/


## EFI system Partition
- [x] 使用 ovmf 启动 Ubuntu 的方法了解一下
  - 使用 -cdrom 的时候，就是默认启动到

我认为安装一下 nixos 有助于帮助我们理解这些蛇皮

在 /boot 下
```txt
efi/
└── EFI
    ├── BOOT
    │   ├── BOOTX64.EFI
    │   ├── fbx64.efi
    │   └── mmx64.efi
    └── ubuntu
        ├── BOOTX64.CSV
        ├── grub.cfg
        ├── grubx64.efi
        ├── mmx64.efi
        └── shimx64.efi
```
而 /boot/grub 中内容就比较诡异了

使用 df -h 可以观察到
```txt
/dev/nvme0n1p2                       234G  211G   12G  95% /
/dev/nvme0n1p1                       511M  5.3M  506M   2% /boot/efi
```

其实一直都没有搞懂，为什么 nvme 为什么存在四个 dev
```txt
➜  /boot l /dev/nvme0 /dev/nvme0n1 /dev/nvme0n1p1 /dev/nvme0n1p2
crw------- root root 0 B Wed Nov 24 09:00:37 2021  /dev/nvme0
brw-rw---- root disk 0 B Wed Nov 24 09:00:37 2021 ﰩ /dev/nvme0n1
brw-rw---- root disk 0 B Wed Nov 24 09:00:40 2021 ﰩ /dev/nvme0n1p1
brw-rw---- root disk 0 B Wed Nov 24 09:00:37 2021 ﰩ /dev/nvme0n1p2
```

如果使用 gPartion 的话，实际上就是只有两个分区而已。

- 因为 UEFI 不能支持普通的程序，但是应该是可以支持各种介质 storage 的访问，所以制作出来一个 EFI system Partition

### 启动的过程
有点好奇当 BOOTX64 没有找到的时候，如何自动切换到 Shell 的啊

启动出来 BOOTX64 的位置大约就是在这里了
```c
/*
#0  FatOFileOpen (OFile=OFile@entry=0x7e1b3698, NewIFile=NewIFile@entry=0x7fe9e8e8, FileName=FileName@entry=0x7dd07f9c, OpenMode=OpenMode@entry=1, Attributes=Attribute
s@entry=0 '\000') at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/Open.c:100
#1  0x000000007ed740f1 in FatOpenEx (Token=0x0, Attributes=0, OpenMode=1, FileName=0x7dd07f9c, NewHandle=0x7fe9ea28, FHand=<optimized out>) at /home/maritns3/core/ld/e
dk2-workstation/edk2/FatPkg/EnhancedFatDxe/Open.c:265
#2  FatOpenEx (FHand=<optimized out>, NewHandle=0x7fe9ea28, FileName=0x7dd07f9c, OpenMode=1, Attributes=Attributes@entry=0, Token=Token@entry=0x0) at /home/maritns3/co
re/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/Open.c:196
#3  0x000000007ed74184 in FatOpen (FHand=<optimized out>, NewHandle=<optimized out>, FileName=<optimized out>, OpenMode=<optimized out>, Attributes=0) at /home/maritns
3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/Open.c:319
#4  0x000000007f05b88a in GetFileBufferByFilePath (AuthenticationStatus=0x7fe9e9f0, FileSize=<synthetic pointer>, FilePath=0x7f0a3718, BootPolicy=1 '\001') at /home/ma
ritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeServicesLib/DxeServicesLib.c:762
#5  GetFileBufferByFilePath (BootPolicy=1 '\001', AuthenticationStatus=0x7fe9e9f0, FileSize=<synthetic pointer>, FilePath=0x7f0a3718, BootPolicy=1 '\001') at /home/mar
itns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeServicesLib/DxeServicesLib.c:610
#6  BmGetNextLoadOptionBuffer (Type=LoadOptionTypeBoot, Type@entry=2131072636, FilePath=FilePath@entry=0x7dd13498, FullPath=FullPath@entry=0x7fe9ead0, FileSize=FileSiz
e@entry=0x7fe9eac8) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/UefiBootManagerLib/BmLoadOption.c:1304
#7  0x000000007f05d41a in EfiBootManagerBoot (BootOption=BootOption@entry=0x7dcfe570) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/UefiBootMana
gerLib/BmBoot.c:1874
#8  0x000000007f060cb7 in BootBootOptions (BootManagerMenu=0x7fe9ecd8, BootOptionCount=5, BootOptions=0x7dcfe518) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeMo
dulePkg/Universal/BdsDxe/BdsEntry.c:409
#9  BdsEntry (This=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/BdsDxe/BdsEntry.c:1072
#10 0x000000007feaabe3 in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:551
#11 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#12 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#13 0x000000007fee10cf in InternalSwitchStack ()
#14 0x0000000000000000 in ?? ()
```

而启动 shell 的 backtrace 在这个位置:
```c
/*
#14 0x000000007feba8b5 in CoreStartImage (ImageHandle=0x7ecaae18, ExitDataSize=0x7ec75470, ExitData=0x7ec75468) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModu
lePkg/Core/Dxe/Image/Image.c:1654
#15 0x000000007f05c5e2 in EfiBootManagerBoot (BootOption=BootOption@entry=0x7ec75420) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/UefiBootMana
gerLib/BmBoot.c:1982
#16 0x000000007f05fca2 in BootBootOptions (BootManagerMenu=0x7fe9ecd8, BootOptionCount=4, BootOptions=0x7ec75318) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeMo
dulePkg/Universal/BdsDxe/BdsEntry.c:409
#17 BdsEntry (This=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/BdsDxe/BdsEntry.c:1072
#18 0x000000007feaabe3 in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:551
#19 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#20 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#21 0x000000007fee10cf in InternalSwitchStack ()
#22 0x0000000000000000 in ?? ()
```
总之，是发生在 EfiBootManagerBoot 中间的，似乎是硬编码的 BOOTX64 的路径，但是我猜测是存在方法首先运行 shell 然后。

## Res
EFI_MM_SYSTEM_TABLE;
EFI_LOADED_IMAGE_PROTOCOL

- EFI_SYSTEM_TABLE
  - EFI_BOOT_SERVICES
  - EFI_RUNTIME_SERVICES

[^1]: edk-ii-uefi-driver-writer-s-guide
[^4]: https://edk2-docs.gitbook.io/edk-ii-uefi-driver-writer-s-guide/3_foundation/readme.8
[^5]: https://edk2-docs.gitbook.io/edk-ii-uefi-driver-writer-s-guide/5_uefi_services/51_services_that_uefi_drivers_commonly_use/515_event_services
