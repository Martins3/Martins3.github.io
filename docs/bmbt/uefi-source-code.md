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

## 跟踪一下 CoreExitBootServices
- 释放内存 : CoreTerminateMemoryMap : 根据其注释，实际上，并不会清空内容
- ZeroMem (gBS, sizeof (EFI_BOOT_SERVICES)); 好的，现在 memory 真的清空了
- `gCpu->DisableInterrupt (gCpu);` : 关闭中断
- CoreNotifySignalList (&gEfiEventExitBootServicesGuid); 是通过 gEfiEventExitBootServicesGuid 来实现找到对应的 Event 的，主要的内容都各种设备的 reset 而已

```c
/*
ArmPkg/Drivers/MmCommunicationDxe/MmCommunication.c:259:  &gEfiEventExitBootServicesGuid,
OvmfPkg/Csm/BiosThunk/VideoDxe/BiosVideo.c:601:                    &gEfiEventExitBootServicesGuid,
NetworkPkg/SnpDxe/Snp.c:660:                    &gEfiEventExitBootServicesGuid,
NetworkPkg/IScsiDxe/IScsiMisc.c:1770:                  &gEfiEventExitBootServicesGuid,
StandaloneMmPkg/Core/StandaloneMmCore.c:90:  { MmExitBootServiceHandler,&gEfiEventExitBootServicesGuid,     NULL, FALSE },
StandaloneMmPkg/Core/StandaloneMmCore.c:160:               &gEfiEventExitBootServicesGuid,
SourceLevelDebugPkg/DebugAgentDxe/DebugAgentDxe.c:100:                  &gEfiEventExitBootServicesGuid,
IntelFsp2WrapperPkg/FspWrapperNotifyDxe/FspWrapperNotifyDxe.c:270:                  &gEfiEventExitBootServicesGuid,
MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/ReportStatusCodeLib.c:156:                  &gEfiEventExitBootServicesGuid,
MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.c:1825:         &gEfiEventExitBootServicesGuid,
MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/SerialStatusCodeWorker.c:155:  // If register an unregister function of gEfiEventExitBootServicesGuid,
MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableDxe/FirmwarePerformanceDxe.c:695:                  &gEfiEventExitBootServicesGuid,
MdeModulePkg/Core/PiSmmCore/PiSmmIpl.c:363:  { FALSE, FALSE, &gEfiEventExitBootServicesGuid,     SmmIplGuidedEventNotify,           &gEfiEventExitBootServicesGuid,     TPL_CALLBACK, NULL },
MdeModulePkg/Core/PiSmmCore/PiSmmCore.c:87:  { SmmExitBootServicesHandler, &gEfiEventExitBootServicesGuid,      NULL, FALSE },
MdeModulePkg/Core/PiSmmCore/PiSmmCore.c:189:    if (CompareGuid (mSmmCoreSmiHandlers[Index].HandlerType, &gEfiEventExitBootServicesGuid)) {
MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:774:  CoreNotifySignalList (&gEfiEventExitBootServicesGuid);
MdeModulePkg/Core/Dxe/Event/Event.c:417:    if (CompareGuid (EventGroup, &gEfiEventExitBootServicesGuid)) {
MdeModulePkg/Core/Dxe/Event/Event.c:427:      EventGroup = &gEfiEventExitBootServicesGuid;
MdeModulePkg/Bus/Pci/UhciDxe/Uhci.c:1759:                  &gEfiEventExitBootServicesGuid,
MdeModulePkg/Bus/Pci/XhciDxe/Xhci.c:2048:                  &gEfiEventExitBootServicesGuid,
MdeModulePkg/Bus/Pci/EhciDxe/Ehci.c:1914:                  &gEfiEventExitBootServicesGuid,
RedfishPkg/RedfishCredentialDxe/RedfishCredentialDxe.c:189:                  &gEfiEventExitBootServicesGuid,
RedfishPkg/RedfishConfigHandler/RedfishConfigHandlerCommon.c:149:                  &gEfiEventExitBootServicesGuid,
BaseTools/Scripts/SmiHandlerProfileSymbolGen.py:181:  '27ABF055-B1B8-4C26-8048-748F37BAA2DF':'gEfiEventExitBootServicesGuid',
MdePkg/Include/Guid/EventGroup.h:16:extern EFI_GUID gEfiEventExitBootServicesGuid;
MdePkg/MdePkg.dec:403:  gEfiEventExitBootServicesGuid  = { 0x27ABF055, 0xB1B8, 0x4C26, { 0x80, 0x48, 0x74, 0x8F, 0x37, 0xBA, 0xA2, 0xDF }}
SecurityPkg/Tcg/TcgDxe/TcgDxe.c:1436:                    &gEfiEventExitBootServicesGuid,
SecurityPkg/Tcg/Tcg2Dxe/Tcg2Dxe.c:2765:                    &gEfiEventExitBootServicesGuid,

```


## InternalSwitchStack 之前发生的内容
```c
/*
#0  SwitchStack (EntryPoint=EntryPoint@entry=0x7feaac72 <_ModuleEntryPoint>, Context1=Context1@entry=0x7bf56000, Context2=0x0, NewStack=NewStack@entry=0x7fe9eff0, Cont
ext2=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/BaseLib/SwitchStack.c:42
#1  0x000000007fee0ebc in HandOffToDxeCore (HobList=..., DxeCoreEntryPoint=2146086002) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/DxeIplPeim/X64
/DxeLoadFunc.c:126
#2  DxeLoadCore (This=<optimized out>, PeiServices=<optimized out>, HobList=...) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/DxeIplPeim/DxeLoad.c
:449
#3  0x000000007feef94e in PeiCore (SecCoreDataPtr=<optimized out>, PpiList=<optimized out>, Data=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeMo
dulePkg/Core/Pei/PeiMain/PeiMain.c:512
#4  0x0000000000826b25 in ?? ()
#5  0x000000007bf548a0 in ?? ()
#6  0x0000000000000000 in ?? ()
```

- This routine is invoked by main entry of PeiMain module during transition
from SEC to PEI. After switching stack in the PEI core, it will restart
with the old core data.

## 关键缩写
Driver Execution Environment (DXE)
pre-EFI initialization (PEI)
Platform Initialization (PI)

## device path
主要参考:
- https://zhuanlan.zhihu.com/p/351065844
- https://zhuanlan.zhihu.com/p/351926214

每个 Physical Device 都会对应 1 个 Controller Handle，在该 Handle 下会安装其对应的 Device Path。

UEFI Bus Driver Connect 时为 Child Device Controller 产生 Device Path

## driver binding
```c
//
// DriverBinding protocol instance
//
EFI_DRIVER_BINDING_PROTOCOL gFatDriverBinding = {
  FatDriverBindingSupported,
  FatDriverBindingStart,
  FatDriverBindingStop,
  0xa,
  NULL,
  NULL
};
```

```c
///
/// This protocol provides the services required to determine if a driver supports a given controller.
/// If a controller is supported, then it also provides routines to start and stop the controller.
///
struct _EFI_DRIVER_BINDING_PROTOCOL {
  EFI_DRIVER_BINDING_SUPPORTED  Supported;
  EFI_DRIVER_BINDING_START      Start;
  EFI_DRIVER_BINDING_STOP       Stop;

  ///
  /// The version number of the UEFI driver that produced the
  /// EFI_DRIVER_BINDING_PROTOCOL. This field is used by
  /// the EFI boot service ConnectController() to determine
  /// the order that driver's Supported() service will be used when
  /// a controller needs to be started. EFI Driver Binding Protocol
  /// instances with higher Version values will be used before ones
  /// with lower Version values. The Version values of 0x0-
  /// 0x0f and 0xfffffff0-0xffffffff are reserved for
  /// platform/OEM specific drivers. The Version values of 0x10-
  /// 0xffffffef are reserved for IHV-developed drivers.
  ///
  UINT32                        Version;

  ///
  /// The image handle of the UEFI driver that produced this instance
  /// of the EFI_DRIVER_BINDING_PROTOCOL.
  ///
  EFI_HANDLE                    ImageHandle;

  ///
  /// The handle on which this instance of the
  /// EFI_DRIVER_BINDING_PROTOCOL is installed. In most
  /// cases, this is the same handle as ImageHandle. However, for
  /// UEFI drivers that produce more than one instance of the
  /// EFI_DRIVER_BINDING_PROTOCOL, this value may not be
  /// the same as ImageHandle.
  ///
  EFI_HANDLE                    DriverBindingHandle;
};
```

DXE 阶段将所有的 driver 都加载进来，执行所有的 entrypoint

- 结合 Fat.c 作为例子分析一下

首先，将 binding 找出来
- ProcessModuleEntryPointList
  - FatEntryPoint
    - EfiLibInstallDriverBindingComponentName2

```c
/*
#0  FatEntryPoint (ImageHandle=ImageHandle@entry=0x7edd9918, SystemTable=SystemTable@entry=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/U
efiLib/UefiDriverModel.c:377
#1  0x000000007ed74d9b in ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7edd9918) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DE
BUG_GCC5/X64/FatPkg/EnhancedFatDxe/Fat/DEBUG/AutoGen.c:319
#2  _ModuleEntryPoint (ImageHandle=0x7edd9918, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiDriverEntryPoint/DriverEntryP
oint.c:127
#3  0x000000007feba912 in CoreStartImage (ImageHandle=0x7edd9918, ExitDataSize=0x0, ExitData=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe
/Image/Image.c:1654
#4  0x000000007feb1846 in CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:523
#5  CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:404
#6  0x000000007feaaafd in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:508
#7  0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#8  _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#9  0x000000007fee10cf in InternalSwitchStack ()
```
这个操作，将 gFatDriverBinding 和 fat driver ImageHandle 关联起来

FatDriverBindingSupported 的 backtrace:
```c
/*
#0  FatDriverBindingSupported (This=0x7ed7bd00, ControllerHandle=0x7edfb398, RemainingDevicePath=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFa
tDxe/Fat.c:286
#1  0x000000007feb6a88 in CoreConnectSingleController (RemainingDevicePath=0x0, ContextDriverImageHandles=0x0, ControllerHandle=0x7edfb398) at /home/maritns3/core/ld/e
dk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/DriverSupport.c:635
#2  CoreConnectController (ControllerHandle=0x7edfb398, ControllerHandle@entry=0x7fec28a0, DriverImageHandle=DriverImageHandle@entry=0x0, RemainingDevicePath=Remaining
DevicePath@entry=0x0, Recursive=Recursive@entry=1 '\001') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/DriverSupport.c:136
#3  0x000000007feb71cf in CoreReinstallProtocolInterface (UserHandle=0x7fec28a0, Protocol=0x0, OldInterface=0x7edfeb40, NewInterface=0x7edfeb40) at /home/maritns3/core
/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Notify.c:265
#4  0x000000007fea6348 in CoreLoadImageCommon.part.0.constprop.0 (BootPolicy=<optimized out>, ParentImageHandle=<optimized out>, FilePath=<optimized out>, SourceBuffer
=<optimized out>, SourceSize=<optimized out>, ImageHandle=0x7f72c698, Attribute=3, EntryPoint=0x0, NumberOfPages=0x0, DstBuffer=0) at /home/maritns3/core/ld/edk2-works
tation/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:1372
#5  0x000000007feb4b55 in CoreLoadImageCommon (DstBuffer=0, NumberOfPages=0x0, EntryPoint=0x0, Attribute=3, ImageHandle=0x7f72c698, SourceSize=0, SourceBuffer=0x0, Fil
ePath=0x7f72c398, ParentImageHandle=0x7edfb398, BootPolicy=64 '@') at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/PlatformDebugLibIoPort/DebugLib.c:28
2
#6  CoreLoadImage (BootPolicy=<optimized out>, ParentImageHandle=0x7edfb398, FilePath=0x7f72c398, SourceBuffer=0x0, SourceSize=0, ImageHandle=0x7f72c698) at /home/mari
tns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:1511
#7  0x000000007feb17d8 in CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:458
#8  CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:404
#9  0x000000007feaaafd in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:508
#10 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#11 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#12 0x000000007fee10cf in InternalSwitchStack ()
#13 0x0000000000000000 in ?? ()
```
似乎 FatDriverBindingSupported 会被调用非常多次，实际上，在任何一次:
```txt
InstallProtocolInterface: 5B1B31A1-9562-11D2-8E3F-00A0C969723B 7E1D26C0
Loading driver at 0x0007EC6D000 EntryPoint=0x0007EC70B36 QemuVideoDxe.efi
```
其实都是会执行一次 FatDriverBindingSupported 和 FatDriverBindingStart
主要出现在 MdeModulePkg/Core/Dxe/Hand/DriverSupport.c

## 代码量分析 cloc
定义了主要分布的位置:
```txt
➜  edk2 git:(master) ✗ cloc /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe
C                               32           3954           7995          14700
```

## gBS gST 和 gDS
需要说明的事情是: EFI_DXE_SERVICES 并没有太考虑清楚。

注册位置:
```c
EFI_STATUS
EFIAPI
UefiBootServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Cache the Image Handle
  //
  gImageHandle = ImageHandle;
  ASSERT (gImageHandle != NULL);

  //
  // Cache pointer to the EFI System Table
  //
  gST = SystemTable;
  ASSERT (gST != NULL);

  //
  // Cache pointer to the EFI Boot Services Table
  //
  gBS = SystemTable->BootServices;
  ASSERT (gBS != NULL);

  return EFI_SUCCESS;
}
```

- 在任何程序中打印出来的 gBS 和 gST 的位置都是相同的，这就是最神奇的地方，没有虚拟地址空间了，隔离方式才是最大的问题。

gBS 和 gST 是在 DxeMain 初始化的
```c
/*
#0  UefiBootServicesTableLibConstructor (SystemTable=0x7f9ee018, ImageHandle=0x7f8eef98) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiBootService
sTableLib/UefiBootServicesTableLib.c:44
#1  ProcessLibraryConstructorList (SystemTable=0x7f9ee018, ImageHandle=0x7f8eef98) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModu
lePkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:449
#2  DxeMain (HobStart=0x7f8ea018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:297
#3  0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#4  _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#5  0x000000007fee10cf in InternalSwitchStack ()
```
gDS 是在 DxeServicesTableLibConstructor 中初始化的

## Driver 和 Application
3.7 [^1]

 EFI_LOADED_IMAGE_PROTOCOL

## 文件系统
其实就是在: edk2/FatPkg
总共代码只有 5000 行而已

Loading driver at 0x0007ED72000 EntryPoint=0x0007ED79AD2 Fat.efi

```c
/*
#0  FatOFileOpen (OFile=OFile@entry=0x7ed14118, NewIFile=NewIFile@entry=0x7fe9e8e8, FileName=FileName@entry=0x7ec55f9c, OpenMode=OpenMode@entry=1, Attributes=Attribute
s@entry=0 '\000') at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/Open.c:100
#1  0x000000007ed790f1 in FatOpenEx (Token=0x0, Attributes=0, OpenMode=1, FileName=0x7ec55f9c, NewHandle=0x7fe9ea28, FHand=<optimized out>) at /home/maritns3/core/ld/e
dk2-workstation/edk2/FatPkg/EnhancedFatDxe/Open.c:265
#2  FatOpenEx (FHand=<optimized out>, NewHandle=0x7fe9ea28, FileName=0x7ec55f9c, OpenMode=1, Attributes=Attributes@entry=0, Token=Token@entry=0x0) at /home/maritns3/co
re/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/Open.c:196
#3  0x000000007ed79184 in FatOpen (FHand=<optimized out>, NewHandle=<optimized out>, FileName=<optimized out>, OpenMode=<optimized out>, Attributes=0) at /home/maritns
3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/Open.c:319
#4  0x000000007f05a875 in GetFileBufferByFilePath (AuthenticationStatus=0x7fe9e9f0, FileSize=<synthetic pointer>, FilePath=0x7f0a3718, BootPolicy=1 '\001') at /home/ma
ritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeServicesLib/DxeServicesLib.c:762
#5  GetFileBufferByFilePath (BootPolicy=1 '\001', AuthenticationStatus=0x7fe9e9f0, FileSize=<synthetic pointer>, FilePath=0x7f0a3718, BootPolicy=1 '\001') at /home/mar
itns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeServicesLib/DxeServicesLib.c:610
#6  BmGetNextLoadOptionBuffer (Type=LoadOptionTypeBoot, Type@entry=2131068519, FilePath=FilePath@entry=0x7ec5bf18, FullPath=FullPath@entry=0x7fe9ead0, FileSize=FileSiz
e@entry=0x7fe9eac8) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/UefiBootManagerLib/BmLoadOption.c:1304
#7  0x000000007f05c405 in EfiBootManagerBoot (BootOption=BootOption@entry=0x7ec753c8) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/UefiBootMana
gerLib/BmBoot.c:1874
#8  0x000000007f05fca2 in BootBootOptions (BootManagerMenu=0x7fe9ecd8, BootOptionCount=4, BootOptions=0x7ec75318) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeMo
dulePkg/Universal/BdsDxe/BdsEntry.c:409
#9  BdsEntry (This=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/BdsDxe/BdsEntry.c:1072
#10 0x000000007feaabf8 in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:553
#11 0x000000007feaac9d in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#12 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#13 0x000000007fee10cf in InternalSwitchStack ()
#14 0x0000000000000000 in ?? ()
```

## 进程地址空间
- [ ] 是不是因为所有的 Application 都是逐个运行的，所以实际上所有的 Application 都是相同的地址
- [ ] 修改代码重新写，gdb script 会发现变化吗?

- [ ] 现在运行多个程序，每个程序装载的位置如何确定的啊
  - Loading driver at 0x0007E5C2000 EntryPoint=0x0007E5CDF6A Main.efi
  - Loading driver at 0x0007E63B000 EntryPoint=0x0007E63C001 Hello.efi
    - Loading driver at 0x0007E63C000 EntryPoint=0x0007E63D001 Hello.efi
  - 位置不是确定的，而且不是顺序的


秘密都是在此处的，此处划分了加载地址是否固定还是动态分配的

[^1]3.7 中分析 LoadImage 的实现:
```c
    DEBUG ((DEBUG_INFO | DEBUG_LOAD,
           "Loading GG driver at 0x%11p EntryPoint=0x%11p ",
           (VOID *)(UINTN) Image->ImageContext.ImageAddress,
           FUNCTION_ENTRY_POINT (Image->ImageContext.EntryPoint)));
```

在 InternalShellExecuteDevicePath 中调用 `gBS->LoadImage` 的:
```c
/*
#0  CoreLoadPeImage (DstBuffer=0, EntryPoint=0x0, Attribute=3, BootPolicy=<optimized out>, Image=0x7e64e398, Pe32Handle=0x7fe9e4b0) at /home/maritns3/core/ld/edk2-work
station/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:569
#1  CoreLoadImageCommon.part.0.constprop.0 (BootPolicy=<optimized out>, ParentImageHandle=0x7ecaae18, FilePath=<optimized out>, SourceBuffer=<optimized out>, SourceSiz
e=<optimized out>, ImageHandle=0x7fe9e6c0, Attribute=3, EntryPoint=0x0, NumberOfPages=0x0, DstBuffer=0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Co
re/Dxe/Image/Image.c:1348
#2  0x000000007feb4aa2 in CoreLoadImageCommon (DstBuffer=0, NumberOfPages=0x0, EntryPoint=0x0, Attribute=3, ImageHandle=0x7fe9e6c0, SourceSize=0, SourceBuffer=0x0, Fil
ePath=0x7e64e018, ParentImageHandle=0x0, BootPolicy=160 '\240') at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/PlatformDebugLibIoPort/DebugLib.c:282
#3  CoreLoadImage (BootPolicy=<optimized out>, ParentImageHandle=0x0, FilePath=0x7e64e018, SourceBuffer=0x0, SourceSize=0, ImageHandle=0x7fe9e6c0) at /home/maritns3/co
re/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:1511
#4  0x000000007e52458d in InternalShellExecuteDevicePath (ParentImageHandle=0x7e5a4ad8, DevicePath=DevicePath@entry=0x7e64e018, CommandLine=CommandLine@entry=0x7e64d21
8, Environment=Environment@entry=0x0, StartImageStatus=StartImageStatus@entry=0x7fe9e838) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Sh
ellProtocol.c:1439
#5  0x000000007e527ab4 in RunCommandOrFile (CommandStatus=0x0, ParamProtocol=0x7e682e98, FirstParameter=0x7e678a18, CmdLine=0x7e64d218, Type=Efi_Application) at /home/
maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2505
#6  SetupAndRunCommandOrFile (CommandStatus=0x0, ParamProtocol=0x7e682e98, FirstParameter=0x7e678a18, CmdLine=<optimized out>, Type=Efi_Application) at /home/maritns3/
core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2589
#7  RunShellCommand (CommandStatus=0x0, CmdLine=0x7e64d218) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2713
#8  RunShellCommand (CmdLine=CmdLine@entry=0x7e64f018, CommandStatus=0x0, CommandStatus@entry=0x7e64e018) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Appl
ication/Shell/Shell.c:2625
#9  0x000000007e52b370 in RunCommand (CmdLine=0x7e64f018) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2765
#10 DoShellPrompt () at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:1358
#11 UefiMain (ImageHandle=<optimized out>, SystemTable=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:621
#12 0x000000007e50f52d in ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7ecaae18) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DE
BUG_GCC5/X64/ShellPkg/Application/Shell/Shell/DEBUG/AutoGen.c:1013
#13 _ModuleEntryPoint (ImageHandle=0x7ecaae18, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiApplicationEntryPoint/Applica
tionEntryPoint.c:59
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
## MdePkg 和 MdeModulePkg 的关系是什么，其中各自主要包含的代码
This package provides the modules that conform to UEFI/PI Industry standards.
It also provides the defintions(including PPIs/PROTOCOLs/GUIDs and library classes) and libraries instances,
which are used for those modules.[^2]

MdeModulePkg [^3]

好吧，只是知道 Mde 比 MdeModulePkg 要更加基础一点。

## OpenProtocol 和 InstallProtocol 的基本操作
找到这些东西对应的代码:
```c
/*
Loading driver at 0x0007E5C2000 EntryPoint=0x0007E5CDFAE Main.efi
InstallProtocolInterface: BC62157E-3E33-4FEC-9920-2D3B36D750DF 7E64EB98
ProtectUefiImageCommon - 0x7E64E040
  - 0x000000007E5C2000 - 0x0000000000020500
InstallProtocolInterface: 752F3136-4E16-4FDC-A22A-E5F46812F4CA 7FE9E6D8
```
在 /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiApplicationEntryPoint/ApplicationEntryPoint.c 似乎是所有 Application 的标准入口
- `_ModuleEntryPoint`
  - ProcessLibraryConstructorList
  - ProcessModuleEntryPointList
    - ShellCEntryLib
      - `SystemTable->BootServices->OpenProtocol` : 利用 EfiShellParametersProtocol 来获取参数，获取标准输入输出
        - CoreOpenProtocol : 使用 gEfiShellInterfaceGuid 来填充 EFI_SHELL_PARAMETERS_PROTOCOL
          - CoreGetProtocolInterface : 使用 EFI_GUID 也就是 gEfiShellInterfaceGuid 获取 PROTOCOL_INTERFACE
      - ShellAppMain
        - `gMD = AllocateZeroPool(sizeof(struct __MainData))` : `__MainData` 记录一些 argc argV 之类的东西，是的，我们的程序是不需要链接器的
        - main
  - ProcessLibraryDestructorList

再看 ShellAppMain 的程序，其只是从 ShellAppMain 开始的而已:


使用 edk2/AppPkg/Applications/Main 作为例子:
- 调用的入口其实是自动生成的: uild/AppPkg/DEBUG_GCC5/X64/AppPkg/Applications/Main/Main/DEBUG/AutoGen.c
- gEfiShellParametersProtocolGuid 的定义也是在 AutoGen.c 中的


注册上 gEfiShellParametersProtocolGuid 的位置
```c
/*
#0  CoreInstallProtocolInterfaceNotify (UserHandle=UserHandle@entry=0x7fe9e6c0, Protocol=Protocol@entry=0x7df83850, InterfaceType=EFI_NATIVE_INTERFACE, Interface=0x7fe
9e6d8, Notify=Notify@entry=1 '\001') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:348
#1  0x000000007feb5eda in CoreInstallProtocolInterface (UserHandle=0x7fe9e6c0, Protocol=0x7df83850, InterfaceType=<optimized out>, Interface=<optimized out>) at /home/
maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:313
#2  0x000000007df65751 in InternalShellExecuteDevicePath (ParentImageHandle=0x7dfe5ad8, DevicePath=DevicePath@entry=0x7e08eb98, CommandLine=CommandLine@entry=0x7e0c849
8, Environment=Environment@entry=0x0, StartImageStatus=StartImageStatus@entry=0x7fe9e838) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Sh
ellProtocol.c:1531
#3  0x000000007df68ab4 in RunCommandOrFile (CommandStatus=0x0, ParamProtocol=0x7e0ca198, FirstParameter=0x7e08e998, CmdLine=0x7e0c8498, Type=Efi_Application) at /home/
maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2505
#4  SetupAndRunCommandOrFile (CommandStatus=0x0, ParamProtocol=0x7e0ca198, FirstParameter=0x7e08e998, CmdLine=<optimized out>, Type=Efi_Application) at /home/maritns3/
core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2589
#5  RunShellCommand (CommandStatus=0x0, CmdLine=0x7e0c8498) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2713
#6  RunShellCommand (CmdLine=CmdLine@entry=0x7e0b8018, CommandStatus=0x0, CommandStatus@entry=0x7df829ac) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Appl
ication/Shell/Shell.c:2625
#7  0x000000007df6c370 in RunCommand (CmdLine=0x7e0b8018) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2765
#8  DoShellPrompt () at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:1358
#9  UefiMain (ImageHandle=<optimized out>, SystemTable=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:621
#10 0x000000007df5052d in ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7f130218) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DE
BUG_GCC5/X64/ShellPkg/Application/Shell/Shell/DEBUG/AutoGen.c:1013
#11 _ModuleEntryPoint (ImageHandle=0x7f130218, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiApplicationEntryPoint/Applica
tionEntryPoint.c:59
#12 0x000000007feba8b5 in CoreStartImage (ImageHandle=0x7f130218, ExitDataSize=0x7e1e06c8, ExitData=0x7e1e06c0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModu
lePkg/Core/Dxe/Image/Image.c:1653
#13 0x000000007f05d5e2 in EfiBootManagerBoot (BootOption=BootOption@entry=0x7e1e0678) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/UefiBootMana
gerLib/BmBoot.c:1982
#14 0x000000007f060ca2 in BootBootOptions (BootManagerMenu=0x7fe9ecd8, BootOptionCount=5, BootOptions=0x7e1e0518) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeMo
dulePkg/Universal/BdsDxe/BdsEntry.c:409
#15 BdsEntry (This=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/BdsDxe/BdsEntry.c:1072
#16 0x000000007feaabe3 in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:551
#17 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#18 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#19 0x000000007fee10cf in InternalSwitchStack ()
#20 0x0000000000000000 in ?? ()
```
从这个 backtrace 看， Protocol 的 interface 的注册发生在 InternalShellExecuteDevicePath 中的:
```c
    //
    // Initialize and install a shell parameters protocol on the image.
    //
    ShellParamsProtocol.StdIn   = ShellInfoObject.NewShellParametersProtocol->StdIn;
    ShellParamsProtocol.StdOut  = ShellInfoObject.NewShellParametersProtocol->StdOut;
    ShellParamsProtocol.StdErr  = ShellInfoObject.NewShellParametersProtocol->StdErr;
    Status = UpdateArgcArgv(&ShellParamsProtocol, NewCmdLine, Efi_Application, NULL, NULL);

    Status = gBS->InstallProtocolInterface(&NewHandle, &gEfiShellParametersProtocolGuid, EFI_NATIVE_INTERFACE, &ShellParamsProtocol);
```

才发现，PROTOCOL_INTERFACE 的定义是一个很随意的存在的呀!
```c

///
/// PROTOCOL_INTERFACE - each protocol installed on a handle is tracked
/// with a protocol interface structure
///
typedef struct {
  UINTN                       Signature;
  /// Link on IHANDLE.Protocols
  LIST_ENTRY                  Link;
  /// Back pointer
  IHANDLE                     *Handle;
  /// Link on PROTOCOL_ENTRY.Protocols
  LIST_ENTRY                  ByProtocol;
  /// The protocol ID
  PROTOCOL_ENTRY              *Protocol;
  /// The interface value
  VOID                        *Interface;
  /// OPEN_PROTOCOL_DATA list
  LIST_ENTRY                  OpenList;
  UINTN                       OpenListCount;

} PROTOCOL_INTERFACE;
```

## UEFI System Table 到底是在如何被使用的
- [x] 在 ShellCEntryLib 中的参数就是

- CoreStartImage
  - `Image->EntryPoint (ImageHandle, Image->Info.SystemTable);` 其实就是 `_ModuleEntryPoint`，其 SystemTable 就是在此处传递的
    - ProcessModuleEntryPointList
      - UefiMain

## InternalShellExecuteDevicePath
- `gBS->LoadImage`
  - CoreLoadImage
    - CoreLoadImageCommon
      - 在此处创建出来一个 EFI_LOADED_IMAGE_PROTOCOL
      - CoreInstallProtocolInterfaceNotify : 注册这个 EFI_LOADED_IMAGE_PROTOCOL ，然后在下面的 OpenProtocol 中使用
- `Status = gBS->OpenProtocol( NewHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);`
  - 现在获取了一个 EFI_LOADED_IMAGE_PROTOCOL
- `Status = gBS->InstallProtocolInterface(&NewHandle, &gEfiShellParametersProtocolGuid, EFI_NATIVE_INTERFACE, &ShellParamsProtocol);`


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

## LoadedImage
每次加载的时候都是相同的位置:
Loading driver at 0x0007E5C2000 EntryPoint=0x0007E5CE00C Main.efi

下面的 backtrace 实际上非常的经典:
DXE 加载 Shell 需要搞一次: CoreLoadImage / CoreStartImage 组合，
然后 Shell 加载具体的程序需要重新走一次。
```c
/*
#0  malloc (Size=Size@entry=7900) at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/LibC/StdLib/Malloc.c:85
#1  0x000000007e5cc40c in tzsetwall () at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/LibC/Time/ZoneProc.c:778
#2  tzset () at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/LibC/Time/ZoneProc.c:796
#3  0x000000007e5cccc1 in mktime (timeptr=0x7e63bba4) at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/LibC/Time/Time.c:520
#4  time (timer=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/LibC/Time/Time.c:558
#5  0x000000007e5cd20a in ShellAppMain (Argc=1, Argv=0x7e64ec98) at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/LibC/Main/Main.c:153
#6  0x000000007e5ce956 in ShellCEntryLib (SystemTable=0x7f9ee018, ImageHandle=0x7e64d298) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Library/UefiShellCEn
tryLib/UefiShellCEntryLib.c:84
#7  ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7e64d298) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/AppPkg/DEBUG_GCC5/X64/AppPkg/App
lications/Main/Main/DEBUG/AutoGen.c:375
#8  _ModuleEntryPoint (ImageHandle=0x7e64d298, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiApplicationEntryPoint/Applica
tionEntryPoint.c:59
#9  0x000000007feba8f7 in CoreStartImage (ImageHandle=0x7e64d298, ExitDataSize=0x0, ExitData=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe
/Image/Image.c:1654
#10 0x000000007e5248b7 in InternalShellExecuteDevicePath (ParentImageHandle=0x7e5a4ad8, DevicePath=DevicePath@entry=0x7e64d698, CommandLine=CommandLine@entry=0x7e65029
8, Environment=Environment@entry=0x0, StartImageStatus=StartImageStatus@entry=0x7fe9e838) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Sh
ellProtocol.c:1540
#11 0x000000007e527ab4 in RunCommandOrFile (CommandStatus=0x0, ParamProtocol=0x7e682f98, FirstParameter=0x7e678c98, CmdLine=0x7e650298, Type=Efi_Application) at /home/
maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2505
#12 SetupAndRunCommandOrFile (CommandStatus=0x0, ParamProtocol=0x7e682f98, FirstParameter=0x7e678c98, CmdLine=<optimized out>, Type=Efi_Application) at /home/maritns3/
core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2589
#13 RunShellCommand (CommandStatus=0x0, CmdLine=0x7e650298) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2713
#14 RunShellCommand (CmdLine=CmdLine@entry=0x7e64f018, CommandStatus=0x0, CommandStatus@entry=0x7e5419ac) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Appl
ication/Shell/Shell.c:2625
#15 0x000000007e52b370 in RunCommand (CmdLine=0x7e64f018) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2765
#16 DoShellPrompt () at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:1358
#17 UefiMain (ImageHandle=<optimized out>, SystemTable=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:621
#18 0x000000007e50f52d in ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7ec55f98) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DE
BUG_GCC5/X64/ShellPkg/Application/Shell/Shell/DEBUG/AutoGen.c:1013
#19 _ModuleEntryPoint (ImageHandle=0x7ec55f98, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiApplicationEntryPoint/Applica
tionEntryPoint.c:59
#20 0x000000007feba8f7 in CoreStartImage (ImageHandle=0x7ec55f98, ExitDataSize=0x7ec75470, ExitData=0x7ec75468) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModu
lePkg/Core/Dxe/Image/Image.c:1654
#21 0x000000007f05c5e2 in EfiBootManagerBoot (BootOption=BootOption@entry=0x7ec75420) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/UefiBootMana
gerLib/BmBoot.c:1982
#22 0x000000007f05fca2 in BootBootOptions (BootManagerMenu=0x7fe9ecd8, BootOptionCount=4, BootOptions=0x7ec75318) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeMo
dulePkg/Universal/BdsDxe/BdsEntry.c:409
#23 BdsEntry (This=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/BdsDxe/BdsEntry.c:1072
#24 0x000000007feaabf8 in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:553
#25 0x000000007feaac9d in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#26 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#27 0x000000007fee10cf in InternalSwitchStack ()
#28 0x0000000000000000 in ?? ()
```
在 malloc 中，gBS::AllocatePool 实际上是 CoreAllocatePool
```c
  Status = gBS->AllocatePool( EfiLoaderData, NodeSize, (void**)&Head);
```
而 CoreAllocatePool 的注册在 mEfiSystemTableTemplate 中

其实，其内存实现就是在: edk2/MdeModulePkg/Core/Dxe/Mem/Pool.c 中

- CoreDispatcher : 从 mScheduledQueue 中可以取出来 EFI_CORE_DRIVER_ENTRY 然后来加载
  - CoreLoadImage
    - CoreLoadImageCommon

```c
typedef struct {
  UINTN                           Signature;
  LIST_ENTRY                      Link;             // mDriverList

  LIST_ENTRY                      ScheduledLink;    // mScheduledQueue

  EFI_HANDLE                      FvHandle;
  EFI_GUID                        FileName;
  EFI_DEVICE_PATH_PROTOCOL        *FvFileDevicePath;
  EFI_FIRMWARE_VOLUME2_PROTOCOL   *Fv;

  VOID                            *Depex;
  UINTN                           DepexSize;

  BOOLEAN                         Before;
  BOOLEAN                         After;
  EFI_GUID                        BeforeAfterGuid;

  BOOLEAN                         Dependent;
  BOOLEAN                         Unrequested;
  BOOLEAN                         Scheduled;
  BOOLEAN                         Untrusted;
  BOOLEAN                         Initialized;
  BOOLEAN                         DepexProtocolError;

  EFI_HANDLE                      ImageHandle;
  BOOLEAN                         IsFvImage;

} EFI_CORE_DRIVER_ENTRY;
```

https://stackoverflow.com/questions/63400839/how-to-set-dxe-drivers-loading-sequence
> DXE dispatcher first loads the driver that specifed in Apriori file.

- DxeMain
  - CoreInitializeDispatcher
    - CoreFwVolEventProtocolNotify : While you are at it read the Ariori file into memory. Place drivers in the A Priori list onto the mScheduledQueue.

## CoreAllocatePages
```c
#0  CoreAllocatePages (Type=AllocateMaxAddress, MemoryType=EfiACPIReclaimMemory, NumberOfPages=1, Memory=0x7fe9ec80) at /home/maritns3/core/ld/edk2-workstation/edk2/Md
eModulePkg/Core/Dxe/Mem/Page.c:1436
#1  0x000000007f13e321 in AcpiTableAcpiTableConstructor (AcpiTableInstance=0x7f15c398) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/Acpi/Acpi
TableDxe/AcpiTableProtocol.c:1905
#2  InitializeAcpiTableDxe (SystemTable=<optimized out>, ImageHandle=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/Acpi/AcpiT
ableDxe/AcpiTable.c:53
#3  ProcessModuleEntryPointList (SystemTable=<optimized out>, ImageHandle=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64
/MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe/DEBUG/AutoGen.c:331
#4  _ModuleEntryPoint (ImageHandle=<optimized out>, SystemTable=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiDriverEntryPoint/Dr
iverEntryPoint.c:127
#5  0x000000007feba8f7 in CoreStartImage (ImageHandle=0x7f15c198, ExitDataSize=0x0, ExitData=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe
/Image/Image.c:1654
#6  0x000000007feb182b in CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:523
#7  CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:404
#8  0x000000007feaab12 in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:510
#9  0x000000007feaac9d in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#10 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#11 0x000000007fee10cf in InternalSwitchStack ()
#12 0x0000000000000000 in ?? ()
```
对应的代码是这个:

```c
InstallProtocolInterface: 18A031AB-B443-4D1A-A5C0-0C09261E9F71 7ED71140
InstallProtocolInterface: 107A772C-D5E1-11D4-9A46-0090273FC14D 7ED71110
InstallProtocolInterface: 6A7A5CFF-E8D9-4F70-BADA-75AB3025CE14 7ED710F0
Loading driver 7BD9DDF7-8B83-488E-AEC9-24C78610289C
InstallProtocolInterface: 5B1B31A1-9562-11D2-8E3F-00A0C969723B 7EDE7240
```

```c
#0  CoreAllocatePages (Type=AllocateMaxAddress, MemoryType=EfiReservedMemoryType, NumberOfPages=4, Memory=0x7fe9eaf0) at /home/maritns3/core/ld/edk2-workstation/edk2/M
deModulePkg/Core/Dxe/Mem/Page.c:1436
#1  0x000000007f135aeb in S3BootScriptGetBootTimeEntryAddAddress (EntryLength=11 '\v') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/PiDxeS3Boot
ScriptLib/BootScriptSave.c:671
#2  S3BootScriptGetEntryAddAddress (EntryLength=EntryLength@entry=11 '\v') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/PiDxeS3BootScriptLib/Bo
otScriptSave.c:852
#3  0x000000007f13650a in S3BootScriptSaveInformation (Information=0x7f065235 <Info.20912>, InformationLength=<optimized out>) at /home/maritns3/core/ld/edk2-workstati
on/edk2/MdeModulePkg/Library/PiDxeS3BootScriptLib/BootScriptSave.c:1785
#4  S3BootScriptSaveInformation (Information=0x7f065235 <Info.20912>, InformationLength=4) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/PiDxeS3
BootScriptLib/BootScriptSave.c:1768
#5  BootScriptWriteInformation (Marker=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/Acpi/S3SaveStateDxe/S3SaveState.c:414
#6  0x000000007f13712e in BootScriptWrite (This=<optimized out>, OpCode=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/Acpi/S3
SaveStateDxe/S3SaveState.c:612
#7  0x000000007f05e7bd in SaveS3BootScript () at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/PlatformBootManagerLib/BdsPlatform.c:1501
#8  PlatformBootManagerBeforeConsole () at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/PlatformBootManagerLib/BdsPlatform.c:388
#9  BdsEntry (This=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/BdsDxe/BdsEntry.c:873
#10 0x000000007feaabf8 in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:553
#11 0x000000007feaac9d in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#12 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#13 0x000000007fee10cf in InternalSwitchStack ()
#14 0x0000000000000000 in ?? ()
```

### Main.efi 触发的 CoreAllocatePages
```c
/*
#0  CoreAllocatePages (Type=AllocateMaxAddress, MemoryType=EfiBootServicesData, NumberOfPages=17, Memory=0x7fe9dab8) at /home/maritns3/core/ld/edk2-workstation/edk2/Md
eModulePkg/Core/Dxe/Mem/Page.c:1436
#1  0x000000007f0fedf4 in RootBridgeIoAllocateBuffer (Attributes=0, HostAddress=0x7fe9dbc8, Pages=17, MemoryType=EfiBootServicesData, Type=AllocateAnyPages, This=<opti
mized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciRootBridgeIo.c:1561
#2  RootBridgeIoAllocateBuffer (This=<optimized out>, Type=AllocateAnyPages, MemoryType=EfiBootServicesData, Pages=17, HostAddress=0x7fe9dbc8, Attributes=0) at /home/m
aritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciRootBridgeIo.c:1495
#3  0x000000007f015492 in PciIoAllocateBuffer (Attributes=0, HostAddress=0x7fe9dbc8, Pages=<optimized out>, MemoryType=EfiBootServicesData, Type=AllocateAnyPages, This
=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Pci/PciBusDxe/PciIo.c:1121
#4  PciIoAllocateBuffer (This=<optimized out>, Type=AllocateAnyPages, MemoryType=EfiBootServicesData, Pages=<optimized out>, HostAddress=0x7fe9dbc8, Attributes=0) at /
home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Pci/PciBusDxe/PciIo.c:1098
#5  0x000000007eda3573 in AtaUdmaInOut (Instance=Instance@entry=0x7ec75698, IdeRegisters=0x7ec75754, Read=<optimized out>, DataBuffer=0x7e79b018, DataLength=32768, Ata
CommandBlock=0x7ec625c0, AtaStatusBlock=0x7ec61000, Timeout=310000000, Task=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Ata/AtaAtapiPassThru/
IdeMode.c:1381
#6  0x000000007eda2c87 in AtaPassThruPassThruExecute (Port=<optimized out>, PortMultiplierPort=<optimized out>, Packet=0x7ec62588, Instance=0x7ec75698, Task=0x0) at /h
ome/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.c:262
#7  0x000000007ed99987 in AtaDevicePassThru (AtaDevice=AtaDevice@entry=0x7ec62498, TaskPacket=<optimized out>, Event=Event@entry=0x0) at /home/maritns3/core/ld/edk2-wo
rkstation/edk2/MdeModulePkg/Bus/Ata/AtaBusDxe/AtaPassThruExecute.c:145
#8  0x000000007ed9a0ec in TransferAtaDevice (AtaDevice=AtaDevice@entry=0x7ec62498, TaskPacket=<optimized out>, Buffer=<optimized out>, StartLba=<optimized out>, Transf
erLength=<optimized out>, IsWrite=<optimized out>, Event=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Ata/AtaBusDxe/AtaPassThruExecute.c:555
#9  0x000000007ed9a553 in AccessAtaDevice (AtaDevice=AtaDevice@entry=0x7ec62498, Buffer=Buffer@entry=0x7e79b018 "", StartLba=StartLba@entry=2049, NumberOfBlocks=0, Num
berOfBlocks@entry=64, IsWrite=IsWrite@entry=0 '\000', Token=Token@entry=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Ata/AtaBusDxe/AtaPassThru
Execute.c:910
#10 0x000000007ed9a9e5 in BlockIoReadWrite (This=This@entry=0x7ec62498, MediaId=MediaId@entry=0, Lba=2049, Token=Token@entry=0x0, BufferSize=<optimized out>, Buffer=Bu
ffer@entry=0x7e79b018, IsBlockIo2=IsBlockIo2@entry=0 '\000', IsWrite=IsWrite@entry=0 '\000') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Ata/AtaBu
sDxe/AtaBus.c:1083
#11 0x000000007ed9ab02 in AtaBlockIoReadBlocks (This=0x7ec62498, MediaId=0, Lba=<optimized out>, BufferSize=<optimized out>, Buffer=0x7e79b018) at /home/maritns3/core/
ld/edk2-workstation/edk2/MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBus.c:1120
#12 0x000000007edd8b5f in DiskIo2ReadWriteDisk (Instance=0x7ec5d998, Write=Write@entry=208 '\320', MediaId=MediaId@entry=0, Offset=Offset@entry=1049088, Token=Token@en
try=0x7fe9ded0, BufferSize=<optimized out>, Buffer=Buffer@entry=0x7e79b018 "") at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/Disk/DiskIoDxe/Di
skIo.c:911
#13 0x000000007edd8e45 in DiskIoReadDisk (This=<optimized out>, MediaId=0, Offset=1049088, BufferSize=<optimized out>, Buffer=0x7e79b018) at /home/maritns3/core/ld/edk
2-workstation/edk2/OvmfPkg/Library/PlatformDebugLibIoPort/DebugLib.c:282
#14 0x000000007edd8b5f in DiskIo2ReadWriteDisk (Instance=0x7ec5d198, Write=Write@entry=208 '\320', MediaId=MediaId@entry=0, Offset=Offset@entry=512, Token=Token@entry=
0x7fe9dfd0, BufferSize=<optimized out>, Buffer=Buffer@entry=0x7e79b018 "") at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIo
.c:911
#15 0x000000007edd8e45 in DiskIoReadDisk (This=<optimized out>, MediaId=0, Offset=512, BufferSize=<optimized out>, Buffer=0x7e79b018) at /home/maritns3/core/ld/edk2-wo
rkstation/edk2/OvmfPkg/Library/PlatformDebugLibIoPort/DebugLib.c:282
#16 0x000000007ed75598 in FatDiskIo (Volume=Volume@entry=0x7ec5c018, IoMode=IoMode@entry=ReadDisk, Offset=Offset@entry=512, BufferSize=BufferSize@entry=32768, Buffer=B
uffer@entry=0x7e79b018, Task=Task@entry=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/Misc.c:346
#17 0x000000007ed76e25 in FatExchangeCachePage (Volume=Volume@entry=0x7ec5c018, DataType=<optimized out>, IoMode=IoMode@entry=ReadDisk, CacheTag=0x7ec5c3b8, Task=Task@
entry=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/DiskCache.c:142
#18 0x000000007ed76ec0 in FatGetCachePage (CacheTag=0x7ec5c3b8, PageNo=0, CacheDataType=CacheFat, Volume=0x7ec5c018) at /home/maritns3/core/ld/edk2-workstation/edk2/Fa
tPkg/EnhancedFatDxe/DiskCache.c:201
#19 FatAccessUnalignedCachePage (Volume=Volume@entry=0x7ec5c018, CacheDataType=CacheDataType@entry=CacheFat, IoMode=IoMode@entry=ReadDisk, PageNo=PageNo@entry=0, Offse
t=Offset@entry=6, Length=Length@entry=2, Buffer=0x7ec5c0b0) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/DiskCache.c:245
#20 0x000000007ed753da in FatAccessCache (Task=0x0, Buffer=0x7ec5c0b0 "", BufferSize=2, Offset=<optimized out>, IoMode=<optimized out>, CacheDataType=CacheFat, Volume=
0x7ec5c018) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/DiskCache.c:331
#21 FatDiskIo (Volume=Volume@entry=0x7ec5c018, IoMode=IoMode@entry=ReadFat, Offset=<optimized out>, BufferSize=2, Buffer=Buffer@entry=0x7ec5c0b0, Task=Task@entry=0x0)
at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/Misc.c:335
#22 0x000000007ed756fe in FatLoadFatEntry (Volume=0x7ec5c018, Index=Index@entry=3) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/FileSpace.c:57
#23 0x000000007ed75742 in FatLoadFatEntry (Index=3, Volume=0x7ec5c018) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/FileSpace.c:97
#24 FatGetFatEntry (Volume=Volume@entry=0x7ec5c018, Index=Index@entry=3) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/FileSpace.c:95
#25 0x000000007ed76660 in FatOFilePosition (PosLimit=132224, Position=0, OFile=0x7e63f918) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/FileSp
ace.c:629
#26 FatAccessOFile (OFile=OFile@entry=0x7e63f918, IoMode=IoMode@entry=ReadData, Position=0, DataBufferSize=DataBufferSize@entry=0x7fe9e488, UserBuffer=UserBuffer@entry
=0x7e5e3018 '\257' <repeats 200 times>..., Task=Task@entry=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/ReadWrite.c:478
#27 0x000000007ed799e9 in FatIFileAccess (FHand=FHand@entry=0x7ec5c018, IoMode=ReadData, IoMode@entry=ReadDisk, BufferSize=0x7fe9e488, Buffer=0x7e5e3018, Token=Token@e
ntry=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/EnhancedFatDxe/ReadWrite.c:307
#28 0x000000007ed79ace in FatRead (FHand=0x7ec5c018, BufferSize=<optimized out>, Buffer=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/FatPkg/Enhance
dFatDxe/ReadWrite.c:361
#29 0x000000007fea51f5 in GetFileBufferByFilePath (AuthenticationStatus=0x7fe9e450, FileSize=0x7fe9e4c8, FilePath=0x7e64e018, BootPolicy=0 '\000') at /home/maritns3/co
re/ld/edk2-workstation/edk2/MdePkg/Library/DxeServicesLib/DxeServicesLib.c:819
#30 GetFileBufferByFilePath (AuthenticationStatus=0x7fe9e450, FileSize=0x7fe9e4c8, FilePath=0x7e64e018, BootPolicy=0 '\000') at /home/maritns3/core/ld/edk2-workstation
/edk2/MdePkg/Library/DxeServicesLib/DxeServicesLib.c:610
#31 CoreLoadImageCommon.part.0.constprop.0 (BootPolicy=<optimized out>, ParentImageHandle=0x7ec55f98, FilePath=0x7e64e018, SourceBuffer=<optimized out>, SourceSize=<op
timized out>, ImageHandle=0x7fe9e6c0, Attribute=3, EntryPoint=0x0, NumberOfPages=0x0, DstBuffer=0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dx
e/Image/Image.c:1215
#32 0x000000007feb4ae4 in CoreLoadImageCommon (DstBuffer=0, NumberOfPages=0x0, EntryPoint=0x0, Attribute=3, ImageHandle=0x7fe9e6c0, SourceSize=0, SourceBuffer=0x0, Fil
ePath=0x7e64e018, ParentImageHandle=0x7ec55f98, BootPolicy=152 '\230') at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/PlatformDebugLibIoPort/DebugLib.
c:282
#33 CoreLoadImage (BootPolicy=<optimized out>, ParentImageHandle=0x7ec55f98, FilePath=0x7e64e018, SourceBuffer=0x0, SourceSize=0, ImageHandle=0x7fe9e6c0) at /home/mari
tns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:1511
#34 0x000000007e52458d in InternalShellExecuteDevicePath (ParentImageHandle=0x7e5a4ad8, DevicePath=DevicePath@entry=0x7e64e018, CommandLine=CommandLine@entry=0x7e64ea1
8, Environment=Environment@entry=0x0, StartImageStatus=StartImageStatus@entry=0x7fe9e838) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Sh
ellProtocol.c:1439
#35 0x000000007e527ab4 in RunCommandOrFile (CommandStatus=0x0, ParamProtocol=0x7e682f98, FirstParameter=0x7e678f98, CmdLine=0x7e64ea18, Type=Efi_Application) at /home/
maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2505
#36 SetupAndRunCommandOrFile (CommandStatus=0x0, ParamProtocol=0x7e682f98, FirstParameter=0x7e678f98, CmdLine=<optimized out>, Type=Efi_Application) at /home/maritns3/
core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2589
#37 RunShellCommand (CommandStatus=0x0, CmdLine=0x7e64ea18) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2713
#38 RunShellCommand (CmdLine=CmdLine@entry=0x7e64f018, CommandStatus=0x0, CommandStatus@entry=0x7e64e018) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Appl
ication/Shell/Shell.c:2625
#39 0x000000007e52b370 in RunCommand (CmdLine=0x7e64f018) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2765
#40 DoShellPrompt () at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:1358
#41 UefiMain (ImageHandle=<optimized out>, SystemTable=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:621
#42 0x000000007e50f52d in ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7ec55f98) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DE
BUG_GCC5/X64/ShellPkg/Application/Shell/Shell/DEBUG/AutoGen.c:1013
#43 _ModuleEntryPoint (ImageHandle=0x7ec55f98, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiApplicationEntryPoint/Applica
tionEntryPoint.c:59
#44 0x000000007feba8f7 in CoreStartImage (ImageHandle=0x7ec55f98, ExitDataSize=0x7ec75470, ExitData=0x7ec75468) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModu
lePkg/Core/Dxe/Image/Image.c:1654
#45 0x000000007f05c5e2 in EfiBootManagerBoot (BootOption=BootOption@entry=0x7ec75420) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/UefiBootMana
gerLib/BmBoot.c:1982
#46 0x000000007f05fca2 in BootBootOptions (BootManagerMenu=0x7fe9ecd8, BootOptionCount=4, BootOptions=0x7ec75318) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeMo
dulePkg/Universal/BdsDxe/BdsEntry.c:409
#47 BdsEntry (This=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/BdsDxe/BdsEntry.c:1072
#48 0x000000007feaabf8 in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:553
#49 0x000000007feaac9d in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#50 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#51 0x000000007fee10cf in InternalSwitchStack ()
#52 0x0000000000000000 in ?? ()
```

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

## StdLib
因为一些原因，edk2 将其实现的 libc 和 edk2 的主要库分离开了，使用方法很简单
1. git clone https://github.com/tianocore/edk2-libc
2. 将 edk2-libc 中的三个文件夹拷贝到 edk2 中，然后就可以当做普通的 pkg 使用

https://www.mail-archive.com/edk2-devel@lists.01.org/msg17266.html
- [ ] 使用 StdLib 只能成为 Application 不能成为 Driver 的
  - [ ] Application 不能直接启动，只能从 UEFI shell 上启动

- [ ] I told you to read "AppPkg/ReadMe.txt"; that file explains what is
necessary for what "flavor" of UEFI application.

- [ ] It even mentions two
example programs, "Main" and "Hello", which don't do anything but
highlight the differences.

- [ ] For another (quite self-contained) example,
"AppPkg/Applications/OrderedCollectionTest" is an application that I
wrote myself; it uses fopen() and fprintf(). This is a unit tester for
an MdePkg library that I also wrote, so it actually exemplifies how you
can use both stdlib and an edk2 library, as long as they don't step on
each other's toes.


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

## 集成 musl
https://github.com/Openwide-Ingenierie/uefi-musl

## 一个游戏
https://github.com/Openwide-Ingenierie/Pong-UEFI


## 一些也许有用的项目
- https://stackoverflow.com/questions/66399748/qemu-hangs-after-booting-a-gnu-efi-os
  - https://github.com/xubury/myos

- https://github.com/evanpurkhiser/rEFInd-minimal
  - 虽然不太相关，但是可以换壁纸也实在是有趣

- https://github.com/vvaltchev/tilck
  - 同时处理了 acpi 和 uefi 的一个 Linux kernel 兼容的 os

- https://github.com/linuxboot/linuxboot
  - 什么叫做使用 Linux 来替换 firmware 啊

- https://github.com/limine-bootloader/limine
  - 一个新的 bootloader

- https://gil0mendes.io/blog/an-efi-app-a-bit-rusty/
  - 使用 rust 封装 UEFI，并且分析了一下 efi 程序的功能

- https://github.com/rust-osdev/uefi-rs/issues/218


- https://blog.system76.com/post/139138591598/howto-uefi-qemu-guest-on-ubuntu-xenial-host
  - 分析了一下使用 ovmf 的事情，但是没有仔细看

On the x86 and ARM platforms, a kernel zImage/bzImage can masquerade
as a PE/COFF image, thereby convincing EFI firmware loaders to load
it as an EFI executable.

The bzImage located in arch/x86/boot/bzImage must be copied to the EFI
System Partition (ESP) and renamed with the extension ".efi".


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
[^2]: https://github.com/tianocore/tianocore.github.io/wiki/MdeModulePkg
[^3]: https://github.com/tianocore/tianocore.github.io/wiki/MdePkg
[^4]: https://edk2-docs.gitbook.io/edk-ii-uefi-driver-writer-s-guide/3_foundation/readme.8
[^5]: https://edk2-docs.gitbook.io/edk-ii-uefi-driver-writer-s-guide/5_uefi_services/51_services_that_uefi_drivers_commonly_use/515_event_services
