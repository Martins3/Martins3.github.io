## signal
- [ ] 测试一下信号机制
  - [ ] 不在存在信号屏蔽机制了，小伙子，但是 UEFI 屏蔽的方法没有完全看懂
- [ ] setitimer 中需要使用 callback 的而不是 Application 的
- [ ] 继续向下分析一下，使用这些 Event 函数为什么总是需要 raised priority 的
- [ ] 所以，到底什么是 EVT_NOTIFY_WAIT
  - [ ] 现在文档说，最好是不要长时间的屏蔽中断，否则不是很好
- [ ] TPL_NOTIFY 之类的对应的 TPL level 都是做什么的呀
  - [ ] 所以 TPL_CALLBACK 和 TPL_NOTIFY 存在什么区别吗?
- [ ] 真的有必要使用 signal 机制来实现 timer 吗 ?
  - [ ] CheckEvent() 到底是一个什么等待方法
- [ ] Raise 和 Restore TPL 是如何实现的
- [ ] 可以通过 Raise TPL 实现临时屏蔽 timer 吗?
- [ ] 在 shell 的 FileInterfaceStdInRead 中调用了 hlt 中还是可以醒过来的，所以这个 timer 操作在什么地方啊
- [ ] 为什么 NotifyFunction 需要设置对应的 TPL，存在一个例子说明的确如此吗?
  - 回忆一下 QEMU 中的 machine done function，也许是一种使用的方法

## Lock
```c
  EfiInitializeLock (&(Sock->Lock), TPL_CALLBACK);
```

```c
EFI_STATUS
EFIAPI
EfiAcquireLockOrFail (
  IN EFI_LOCK  *Lock
  )
{

  ASSERT (Lock != NULL);
  ASSERT (Lock->Lock != EfiLockUninitialized);

  if (Lock->Lock == EfiLockAcquired) {
    //
    // Lock is already owned, so bail out
    //
    return EFI_ACCESS_DENIED;
  }

  Lock->OwnerTpl = gBS->RaiseTPL (Lock->Tpl);

  Lock->Lock = EfiLockAcquired;

  return EFI_SUCCESS;
}
```
为了防止，在执行流程中，已经执行了 EfiAcquireLockOrFail 但是中断到了，那么此时函数调用就是需要失败的

屏蔽中断和上锁是两个事情:
- 上锁，将一个变量设置一下为 lockAcquired，之后检查一下变量，还是可以被打断，但是想要上锁就会失败

其实和 TPL 关系不大
## 各种 TPL

### TPL_APPLICATION
- CoreWaitForEvent : 只能在 TPL_APPLICATION 调用 WaitForEvent
```c
  //
  // Can only WaitForEvent at TPL_APPLICATION
  //
  if (gEfiCurrentTpl != TPL_APPLICATION) {
    return EFI_UNSUPPORTED;
  }
```
- CoreCreateEventInternal : 对于 EVT_NOTIFY_WAIT | EVT_NOTIFY_SIGNAL 必须有 NotifyFunction，而且 NotifyFunction 执行的 NotifyTpl 介于 TPL_APPLICATION 和 TPL_HIGH_LEVEL 之间(不包含)
  - NotifyTpl 的作用在 : 在 CoreRestoreTpl 中，当 NewTpl 降到 NotifyTpl 之下（不包含），那么该 NotifyFunction 可以执行
  - 所以 NotifyTpl 是不能等于 TPL_APPLICATION，否则永远无法执行的
  - NotifyTpl 不能等于 TPL_APPLICATION，表示其不能屏蔽中断

```c
  //
  // If it's a notify type of event, check its parameters
  //
  if ((Type & (EVT_NOTIFY_WAIT | EVT_NOTIFY_SIGNAL)) != 0) {
    //
    // Check for an invalid NotifyFunction or NotifyTpl
    //
    if ((NotifyFunction == NULL) ||
        (NotifyTpl <= TPL_APPLICATION) ||
       (NotifyTpl >= TPL_HIGH_LEVEL)) {
      return EFI_INVALID_PARAMETER;
    }

  } else {
    //
    // No notification needed, zero ignored values
    //
    NotifyTpl = 0;
    NotifyFunction = NULL;
    NotifyContext = NULL;
  }
```

### TPL_CALLBACK
```diff
History: #0
Commit:  15cc67e616cad2dad3d3b6f9ba1cba856b5de414
Author:  erictian <erictian@6f19259b-4bc3-4df7-8a09-765794883524>
Date:    Wed 05 May 2010 01:21:38 PM CST

raise TPL to TPL_CALLBACK level at DriverBindingStart() for all usb-related modules, which prevent DriverBindingStop() from being invoked when DriverBindingStart() runs.
```

使用位置基本是:
- `EfiInitializeLock (&(Sock->Lock), TPL_CALLBACK);`
- `OldTpl = gBS->RaiseTPL (TPL_CALLBACK);`
- `Status = gBS->CreateEventEx (`

- [ ] TPL_CALLBACK 和 TPL_NOTIFY 的使用方法非常相似，说实话，根本无法区分两者

## 各种 EVT
```c
//
// These types can be ORed together as needed - for example,
// EVT_TIMER might be Ored with EVT_NOTIFY_WAIT or
// EVT_NOTIFY_SIGNAL.
//
#define EVT_TIMER                         0x80000000
#define EVT_RUNTIME                       0x40000000
#define EVT_NOTIFY_WAIT                   0x00000100
#define EVT_NOTIFY_SIGNAL                 0x00000200
```
- CoreCreateEventInternal
- 似乎 EVT_RUNTIME 是几乎没有作用的
- EVT_NOTIFY_SIGNAL
  - 将 IEvent 放到 gEventSignalQueue 中
- EVT_TIMER : 没有特殊操作
- EVT_NOTIFY_WAIT : 同样没有特殊操作


- 也就是 EVT_NOTIFY_SIGNAL 的 Event 靠执行流程中去调用
  `gBS->SignalEvent (Event);`

所有的 Event 都是挂载到 gEventSignalQueue 上的，每一个 Event 都是可以关联一个
EventGroup
```c
  CoreNotifySignalList (&gEfiEventExitBootServicesGuid);
```

- CoreSignalEvent
  - CoreAcquireEventLock
  - CoreNotifySignalList : 如果 notify 是一个 group，那么将这个 group 的 Event 全部 notify 一下
    - CoreNotifyEvent
  - CoreNotifyEvent
    - 将 Event 加入到 gEventQueue
  - CoreReleaseEventLock
    - CoreDispatchEventNotifies
      - `Event->NotifyFunction` : 注意，这里只会执行 tpl 高于当前的 tpl 的 event 的

- CoreCheckEvent
  - 如果没有 notify ，调用 CoreNotifyEvent 来将 gEventQueue 中
  - 如果已经 notified 过，那么释放掉对应的数据
  - 要求被 check 的 event 一定 ***不是*** EVT_NOTIFY_SIGNAL 类型的
- CoreWaitForEvent : Stops execution until an event is signaled.
  - 对于每一个 Event 调用 CoreCheckEvent
  - 如果存在一个 Event 被 signal 了，那么可以返回
  - 否则调用 CoreSignalEvent (gIdleLoopEvent)，最后执行 CpuSleep

- EVT_TIMER 和 EVT_NOTIFY_SIGNAL 是可以放到一起使用的，这样一个消息可以通过
  - EVT_TIMER 的唯一的区别对待是 CoreCloseEvent 中需要手动关闭一下 pending 的 CoreSetTimer



## Shell 的等待
```c
/*
#0  0x000000007f16f841 in CpuSleep ()
#1  0x000000007feac77d in CoreDispatchEventNotifies (Priority=16) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:194
#2  CoreRestoreTpl (NewTpl=4) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Tpl.c:131
#3  0x000000007feb4d61 in CoreReleaseLock (Lock=0x7fec2470) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Library/Library.c:96
#4  0x000000007fead478 in CoreSignalEvent (UserEvent=0x7f8edd18) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:566
#5  CoreSignalEvent (UserEvent=0x7f8edd18) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:531
#6  0x000000007fead59c in CoreWaitForEvent (UserIndex=<optimized out>, UserEvents=<optimized out>, NumberOfEvents=<optimized out>) at /home/maritns3/core/ld/edk2-works
tation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:707
#7  CoreWaitForEvent (NumberOfEvents=1, UserEvents=0x7edecce0, UserIndex=0x7fe9e808) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.
c:663
#8  0x000000007df58972 in FileInterfaceStdInRead (This=<optimized out>, BufferSize=0x7fe9e908, Buffer=0x7e08c018) at /home/maritns3/core/ld/edk2-workstation/edk2/Shell
Pkg/Application/Shell/FileHandleWrappers.c:532
#9  0x000000007df64332 in DoShellPrompt () at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:1346
#10 UefiMain (ImageHandle=<optimized out>, SystemTable=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:621
#11 0x000000007df4852d in ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7f130218) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DE
BUG_GCC5/X64/ShellPkg/Application/Shell/Shell/DEBUG/AutoGen.c:1013
#12 _ModuleEntryPoint (ImageHandle=0x7f130218, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiApplicationEntryPoint/Applica
tionEntryPoint.c:59
#13 0x000000007feba8b5 in CoreStartImage (ImageHandle=0x7f130218, ExitDataSize=0x7e1d8d48, ExitData=0x7e1d8d40) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModu
lePkg/Core/Dxe/Image/Image.c:1654
#14 0x000000007f05d5e2 in EfiBootManagerBoot (BootOption=BootOption@entry=0x7e1d8cf8) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/UefiBootMana
gerLib/BmBoot.c:1982
#15 0x000000007f060ca2 in BootBootOptions (BootManagerMenu=0x7fe9ecd8, BootOptionCount=5, BootOptions=0x7e1d8b98) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeMo
dulePkg/Universal/BdsDxe/BdsEntry.c:409
#16 BdsEntry (This=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/BdsDxe/BdsEntry.c:1072
#17 0x000000007feaabe3 in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:551
#18 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#19 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#20 0x000000007fee10cf in InternalSwitchStack ()
#21 0x0000000000000000 in ?? ()
```
在 gdb 中 disass 可以:
```txt
Dump of assembler code for function CpuSleep:
   0x000000007fed6760 <+0>:     hlt
   0x000000007fed6761 <+1>:     retq
   0x000000007fed6762 <+2>:     nopw   %cs:0x0(%rax,%rax,1)
   0x000000007fed676c <+12>:    nopl   0x0(%rax)
```
进而找到 MdePkg/Library/BaseCpuLib/BaseCpuLib.inf 可以找到和架构相关的汇编实现

虽然无法在 EfiEventEmptyFunction 上打断点，但是在其中添加输出 DEBUG，最后会发现 EfiEventEmptyFunction 和
IdleLoopEventCallback 都是会被调用的:
- 在 CoreCreateEventInternal 中，可以看到 EventGroup
- 在 CoreSignalEvent 中，虽然是 `CoreSignalEvent (gIdleLoopEvent)` 但是在 CoreSignalEvent 中的实际上会将整个 group 的全部 notify 一下

## Timer
```c
/*
#0  CoreCheckTimers (CheckEvent=0x7f8edc18, Context=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Timer.c:98
#1  0x000000007feac77d in CoreDispatchEventNotifies (Priority=30) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:194
#2  CoreRestoreTpl (NewTpl=16) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Tpl.c:131
#3  0x000000007f144350 in TimerInterruptHandler (InterruptType=<optimized out>, SystemContext=...) at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/8254TimerDxe
/Timer.c:89
#4  0x000000007f168da4 in CommonExceptionHandlerWorker (ExceptionHandlerData=0x7f173920, SystemContext=..., ExceptionType=104) at /home/maritns3/core/ld/edk2-workstati
on/edk2/UefiCpuPkg/Library/CpuExceptionHandlerLib/X64/ArchExceptionHandler.c:94
#5  CommonExceptionHandler (ExceptionType=<optimized out>, SystemContext=...) at /home/maritns3/core/ld/edk2-workstation/edk2/UefiCpuPkg/Library/CpuExceptionHandlerLib
/DxeException.c:40
#6  0x000000007f16fba0 in DrFinish ()
#7  0x0000000000000080 in ?? ()
#8  0x00ffff0000000000 in ?? ()
#9  0x0000000000000000 in ?? ()
8?
```
处理中断的代码主要分布在 ./UefiCpuPkg/Library/CpuExceptionHandlerLib/X64/ExceptionHandlerAsm.nasm 中

基本的处理套路:
- AsmGetTemplateAddressMap : 制作入口
  - AsmIdtVectorBegin : 之后这就是 idt 的入口了
    - CommonInterruptEntry
      - DrFinish
        - CommonExceptionHandler : 好的，现在进入到我们熟悉的位置了
          - ExternalInterruptHandler = ExceptionHandlerData->ExternalInterruptHandler; 获取 hook，其实就是 TimerInterruptHandler 了
            - TimerInterruptHandler
              - `gBS->RaiseTPL (TPL_HIGH_LEVEL);` : 这个区间是需要屏蔽中断的
              - mTimerNotifyFunction : 被注册为 CoreTimerTick
                - mEfiSystemTime += Duration; : 刷新系统时间
                - CoreSignalEvent (mEfiCheckTimerEvent);
              - `gBS->RestoreTPL (OriginalTPL);`
- UpdateIdtTable : 进行安装

* 注册 ExternalInterruptHandler 为 TimerInterruptHandler 的位置 :  TimerDriverInitialize => `mCpu->RegisterInterruptHandler`
* TimerInterruptHandler 中将 mTimerNotifyFunction 注册为 : CoreTimerTick
* mEfiCheckTimerEvent 注册的 hook 为 CoreCheckTimers，会将其中过期的 timer 处理一下

### 如何调整系统 periodic timer 的时间的
- TimerDriverInitialize
  - TimerDriverSetTimerPeriod
    - SetPitCount : 向一些端口写数值

所以，系统中存在一个 timer 总是按照固定频率触发，从而保证系统的时钟，实际上，这天然的是制造出来了问题。

## EVT_NOTIFY_WAIT
使用 `ConInPrivate->TextIn.WaitForKey` 作为例子:

- 初始化 : ConSplitterTextInConstructor
  - hook : ConSplitterTextInWaitForKey
- 调用位置 : FileInterfaceStdInRead

最后的执行流程:
- FileInterfaceStdInRead
  - CoreWaitForEvent
    - CpuSleep
    - FileInterfaceStdInRead

所以 CoreWaitForEvent 会将 Event 上的 hook 执行一次，而且需要一直等待到 hook 返回 EFI_READY
用于监听键盘输入实在是极度合适啊。

## EVT_NOTIFY_SIGNAL
使用 gIdleLoopEvent 作为例子
- 在 CoreSignalEvent 中调用 `CoreSignalEvent (gIdleLoopEvent)`


使用 PeCoffEmuProtocolNotify 作为例子:

- CoreInstallMultipleProtocolInterfaces : 使用这个作为例子
  - CoreRestoreTpl
    - CoreDispatchEventNotifies
      - PeCoffEmuProtocolNotify

- CoreInitializeImageServices
  - CoreInstallProtocolInterface : gEfiLoadedImageProtocolGuid
    - CoreInstallProtocolInterfaceNotify
      - CoreNotifyProtocolEntry
  - CoreCreateEvent : mPeCoffEmuProtocolRegistrationEvent
  - CoreRegisterProtocolNotify : 将 mPeCoffEmuProtocolRegistrationEvent 注册上，最后会在 CoreNotifyProtocolEntry 中被 signal 然后在 CoreRestoreTpl 时候调用

```c
/*
#0  CoreNotifyProtocolEntry (ProtEntry=ProtEntry@entry=0x7f8ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Notify.c:28
#1  0x000000007feb5e66 in CoreInstallProtocolInterfaceNotify (InterfaceType=<optimized out>, Notify=1 '\001', Interface=0x7fec25e8, Protocol=0x7fec1bb0, UserHandle=0x7
fec25c8) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:476
#2  CoreInstallProtocolInterfaceNotify (UserHandle=0x7fec25c8, UserHandle@entry=0x7f8ee018, Protocol=0x7fec1bb0, Protocol@entry=0x0, InterfaceType=InterfaceType@entry=
EFI_NATIVE_INTERFACE, Interface=Interface@entry=0x7fec25e8, Notify=Notify@entry=1 '\001') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Ha
ndle.c:341
#3  0x000000007feb5ef8 in CoreInstallProtocolInterface (UserHandle=0x7f8ee018, UserHandle@entry=0x7fec25c8, Protocol=0x0, InterfaceType=InterfaceType@entry=EFI_NATIVE_
INTERFACE, Interface=Interface@entry=0x7fec25e8) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:313
#4  0x000000007feb98d8 in CoreInitializeImageServices (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:22
9
#5  0x000000007fea98d2 in DxeMain (HobStart=0x7bf56000) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:285
#6  0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#7  _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#8  0x000000007fee10cf in InternalSwitchStack ()
#9  0x0000000000000000 in ?? ()
```

```c
/*
#0  PeCoffEmuProtocolNotify (Event=0x7f8eeb18, Context=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:127
#1  0x000000007feac77d in CoreDispatchEventNotifies (Priority=8) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:194
#2  CoreRestoreTpl (NewTpl=4) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Tpl.c:131
#3  0x000000007feb7066 in CoreInstallMultipleProtocolInterfaces (Handle=0x7fe9ec88) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c
:611
#4  0x000000007f58b14d in InitializeEbcDriver (SystemTable=<optimized out>, ImageHandle=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/U
niversal/EbcDxe/EbcInt.c:547
#5  ProcessModuleEntryPointList (SystemTable=<optimized out>, ImageHandle=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64
/MdeModulePkg/Universal/EbcDxe/EbcDxe/DEBUG/AutoGen.c:196
#6  _ModuleEntryPoint (ImageHandle=<optimized out>, SystemTable=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiDriverEntryPoint/Dr
iverEntryPoint.c:127
```

```c
/**
  Notification event handler registered by CoreInitializeImageServices () to
  keep track of which PE/COFF image emulators are available.

  @param  Event          The Event that is being processed, not used.
  @param  Context        Event Context, not used.

**/
STATIC
VOID
EFIAPI
PeCoffEmuProtocolNotify (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
```

```c
/**
  Installs a list of protocol interface into the boot services environment.
  This function calls InstallProtocolInterface() in a loop. If any error
  occures all the protocols added by this function are removed. This is
  basically a lib function to save space.
**/
CoreInstallMultipleProtocolInterfaces (
```

### TimerDriverRegisterHandler

***步骤 1***

GenericProtocolNotify 是在 CoreNotifyOnProtocolEntryTable 中注册的，很多 protocol 的 hook 都是注册的这个

- DxeMain
  - CoreNotifyOnProtocolInstallation : mArchProtocols 和 mOptionalProtocols 定义一堆 EFI_CORE_PROTOCOL_NOTIFY_ENTRY
    - CoreNotifyOnProtocolEntryTable (mArchProtocols);
    - CoreNotifyOnProtocolEntryTable (mOptionalProtocols);

- CoreNotifyOnProtocolEntryTable : Creates an event for each entry in a table that is fired everytime a Protocol of a specific type is installed.
  - CoreCreateEvent : 创建 Event 的出来
  - CoreRegisterProtocolNotify : 将 Event 和 protocol 关联起来
    - CoreFindProtocolEntry : 根据 EFI_GUID 找到的 ProtEntry
    - 创建出来 PROTOCOL_NOTIFY ProtEntry
    - `InsertTailList (&ProtEntry->Notify, &ProtNotify->Link);`


***步骤 2***

GenericProtocolNotify 对应的 hook 是从 gEventQueue 取出来的，添加的时候在下面:

```c
/*
#1  CoreNotifyEvent (Event=Event@entry=0x7f8e7118) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:252
#2  0x000000007fead4df in CoreSignalEvent (UserEvent=0x7f8e7118) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:589
#3  CoreSignalEvent (UserEvent=0x7f8e7118) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:551
#4  0x000000007feb442a in CoreNotifyProtocolEntry (ProtEntry=ProtEntry@entry=0x7f8e7b18) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Not
ify.c:32
#5  0x000000007feb5ea0 in CoreInstallProtocolInterfaceNotify (InterfaceType=<optimized out>, Notify=1 '\001', Interface=0x7fae7040, Protocol=0x7fae7090, UserHandle=0x7
fae7120) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:476
#6  CoreInstallProtocolInterfaceNotify (UserHandle=0x7fae7120, UserHandle@entry=0x7f8e7b18, Protocol=0x7fae7090, Protocol@entry=0x0, InterfaceType=InterfaceType@entry=
EFI_NATIVE_INTERFACE, Interface=Interface@entry=0x7fae7040, Notify=Notify@entry=1 '\001') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Ha
ndle.c:341
#7  0x000000007feb5f32 in CoreInstallProtocolInterface (UserHandle=0x7f8e7b18, UserHandle@entry=0x7fae7120, Protocol=0x0, Protocol@entry=0x7fae7090, InterfaceType=Inte
rfaceType@entry=EFI_NATIVE_INTERFACE, Interface=Interface@entry=0x7fae7040) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:313
#8  0x000000007feb7014 in CoreInstallMultipleProtocolInterfaces (Handle=0x7fae7120) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c
:586
#9  0x000000007fae5901 in RuntimeDriverInitialize (SystemTable=0x7f9ee018, ImageHandle=0x7f5ad118) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Ru
ntimeDxe/Runtime.c:415
#10 ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7f5ad118) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/RuntimeDxe/RuntimeDxe/DEBUG/AutoGen.c:361
#11 _ModuleEntryPoint (ImageHandle=0x7f5ad118, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiDriverEntryPoint/DriverEntryP
oint.c:127
#12 0x000000007feba90d in CoreStartImage (ImageHandle=0x7f5ad118, ExitDataSize=0x0, ExitData=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe
/Image/Image.c:1654
#13 0x000000007feb1841 in CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:523
#14 CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:404
#15 0x000000007feaaafd in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:508
#16 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#17 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#18 0x000000007fee10cf in InternalSwitchStack ()
```
- CoreInstallProtocolInterfaceNotify
  - CoreFindProtocolEntry : 可以根据 guid 找到 PROTOCOL_ENTRY (遍历 mProtocolDatabase)
  - CoreNotifyProtocolEntry : 因为 PROTOCOL_ENTRY 持有 PROTOCOL_NOTIFY 的，进而添加 notify

***步骤 3***

```c
/*
#0  TimerDriverRegisterHandler (This=0x7f145c40, NotifyFunction=0x7fead493 <CoreTimerTick>) at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/8254TimerDxe/Timer.
c:132
#1  0x000000007feab988 in GenericProtocolNotify (Event=<optimized out>, Context=0x7fec15f8) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMa
in/DxeProtocolNotify.c:155
#2  0x000000007feac77d in CoreDispatchEventNotifies (Priority=8) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Event.c:194
#3  CoreRestoreTpl (NewTpl=4) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Event/Tpl.c:131
#4  0x000000007feb7062 in CoreInstallMultipleProtocolInterfaces (Handle=0x7f145cb0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c
:611
#5  0x000000007f145328 in TimerDriverInitialize (SystemTable=<optimized out>, ImageHandle=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/8254
TimerDxe/Timer.c:393
#6  ProcessModuleEntryPointList (SystemTable=<optimized out>, ImageHandle=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64
/OvmfPkg/8254TimerDxe/8254Timer/DEBUG/AutoGen.c:194
#7  _ModuleEntryPoint (ImageHandle=<optimized out>, SystemTable=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiDriverEntryPoint/Dr
iverEntryPoint.c:127
#8  0x000000007feba8cf in CoreStartImage (ImageHandle=0x7f151c98, ExitDataSize=0x0, ExitData=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe
/Image/Image.c:1654
#9  0x000000007feb1803 in CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:523
#10 CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:404
#11 0x000000007feaaafd in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:508
#12 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#13 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#14 0x000000007fee10cf in InternalSwitchStack ()
#15 0x0000000000000000 in ?? ()
```

- [ ] 步骤二 和 步骤三都是在 CoreInstallMultipleProtocolInterfaces 调用路径上的
  - 在步骤二中，CoreInstallMultipleProtocolInterfaces 最后会调用 CoreRestoreTpl 的，那么当时 notfiy event 的时候，不会在那个时候执行
- [ ] 步骤二中 CoreInstallMultipleProtocolInterfaces 调用的 guid 都是怎么获取的，和 步骤一 的 guid 是什么关系

每一个 IEVENT::NotifyTpl 表示该 evnet 的 callback 只有在程序运行 gEfiCurrentTpl 小于等于该 event 的才可以的呀。
这就是我们发现有非常多的 callback 都是在 CoreRestoreTpl 的时候执行的

```c
/*
Loading driver F099D67F-71AE-4C36-B2A3-DCEB0EB2B7D8
InstallProtocolInterface: 5B1B31A1-9562-11D2-8E3F-00A0C969723B 7F0EC040
Loading driver at 0x0007F0BD000 EntryPoint=0x0007F0BDFBE WatchdogTimer.efi
InstallProtocolInterface: BC62157E-3E33-4FEC-9920-2D3B36D750DF 7F0ED298
ProtectUefiImageCommon - 0x7F0EC040
  - 0x000000007F0BD000 - 0x0000000000001EC0
InstallProtocolInterface: 665E3FF5-46CC-11D4-9A38-0090273FC14D 7F0BED30
fuck
begin 7FEAB91A
in GenericProtocolNotify 7FEAB91A
execute 7FEAB91A
end

Loading driver AD608272-D07F-4964-801E-7BD3B7888652
InstallProtocolInterface: 5B1B31A1-9562-11D2-8E3F-00A0C969723B 7F0EC440
Loading driver at 0x0007FAB8000 EntryPoint=0x0007FAB9D8F MonotonicCounterRuntimeDxe.efi
InstallProtocolInterface: BC62157E-3E33-4FEC-9920-2D3B36D750DF 7F0EC998
ProtectUefiImageCommon - 0x7F0EC440
  - 0x000000007FAB8000 - 0x0000000000004000
SetUefiImageMemoryAttributes - 0x000000007FAB8000 - 0x0000000000001000 (0x0000000000004008)
SetUefiImageMemoryAttributes - 0x000000007FAB9000 - 0x0000000000002000 (0x0000000000020008)
SetUefiImageMemoryAttributes - 0x000000007FABB000 - 0x0000000000001000 (0x0000000000004008)
InstallProtocolInterface: 1DA97072-BDDC-4B30-99F1-72A0B56FFF2A 0
fuck
begin 7FEAB91A
in GenericProtocolNotify 7FEAB91A
execute 7FEAB91A
end
```
看上去这些 Event 都是在步骤二中同时注册的，但是实际上是一个个执行的。

## 我们到底是如何 install image
这两个函数是交错出现的
1. 首先 load image
2. 然后 start image

```c
/*
#0  CoreInstallProtocolInterface (UserHandle=0x7f162020, Protocol=0x7fec1c60, InterfaceType=EFI_NATIVE_INTERFACE, Interface=0x7f162f18) at /home/maritns3/core/ld/edk2-
workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:312
#1  0x000000007fea6397 in CoreLoadImageCommon.part.0.constprop.0 (BootPolicy=<optimized out>, ParentImageHandle=<optimized out>, FilePath=<optimized out>, SourceBuffer
=<optimized out>, SourceSize=<optimized out>, ImageHandle=0x7f881098, Attribute=3, EntryPoint=0x0, NumberOfPages=0x0, DstBuffer=0) at /home/maritns3/core/ld/edk2-works
tation/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:1393
#2  0x000000007feb4b55 in CoreLoadImageCommon (DstBuffer=0, NumberOfPages=0x0, EntryPoint=0x0, Attribute=3, ImageHandle=0x7f881098, SourceSize=0, SourceBuffer=0x0, Fil
ePath=0x7f881f98, ParentImageHandle=0x7f174218, BootPolicy=64 '@') at /home/maritns3/core/ld/edk2-workstation/edk2/OvmfPkg/Library/PlatformDebugLibIoPort/DebugLib.c:28
2
#3  CoreLoadImage (BootPolicy=<optimized out>, ParentImageHandle=0x7f174218, FilePath=0x7f881f98, SourceBuffer=0x0, SourceSize=0, ImageHandle=0x7f881098) at /home/mari
tns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Image/Image.c:1511
#4  0x000000007feb17d8 in CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:458
#5  CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:404
#6  0x000000007feaaafd in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:508
#7  0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#8  _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#9  0x000000007fee10cf in InternalSwitchStack ()
#10 0x0000000000000000 in ?? ()
```

```c
/*
#0  CoreInstallProtocolInterface (UserHandle=UserHandle@entry=0x7fe9eca8, Protocol=Protocol@entry=0x7fae1140, InterfaceType=InterfaceType@entry=EFI_NATIVE_INTERFACE, I
nterface=Interface@entry=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:312
#1  0x000000007feb706f in CoreInstallMultipleProtocolInterfaces (Handle=0x7fe9eca8) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c
:586
#2  0x000000007fadfd58 in InitializeResetSystem (SystemTable=0x7f9ee018, ImageHandle=0x7f174218) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal
/ResetSystemRuntimeDxe/ResetSystem.c:190
#3  ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7f174218) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe/DEBUG/AutoGen.c:417
#4  _ModuleEntryPoint (ImageHandle=0x7f174218, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiDriverEntryPoint/DriverEntryP
oint.c:127
#5  0x000000007feba968 in CoreStartImage (ImageHandle=0x7f174218, ExitDataSize=0x0, ExitData=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe
/Image/Image.c:1654
#6  0x000000007feb189c in CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:523
#7  CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:404
#8  0x000000007feaaafd in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:508
#9  0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#10 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#11 0x000000007fee10cf in InternalSwitchStack ()
#12 0x0000000000000000 in ?? ()
```

但是 CoreInstallProtocolInterfaceNotify 的形式差不多就总是这个样子的了:
```c
/*
#0  CoreInstallProtocolInterfaceNotify (InterfaceType=<optimized out>, Notify=1 '\001', Interface=0x0, Protocol=0x7fae1140, UserHandle=0x7fe9eca8) at /home/maritns3/co
re/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:376
#1  CoreInstallProtocolInterfaceNotify (UserHandle=UserHandle@entry=0x7fe9eca8, Protocol=Protocol@entry=0x7fae1140, InterfaceType=InterfaceType@entry=EFI_NATIVE_INTERF
ACE, Interface=Interface@entry=0x0, Notify=Notify@entry=1 '\001') at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:341
#2  0x000000007feb5f8d in CoreInstallProtocolInterface (UserHandle=UserHandle@entry=0x7fe9eca8, Protocol=Protocol@entry=0x7fae1140, InterfaceType=InterfaceType@entry=E
FI_NATIVE_INTERFACE, Interface=Interface@entry=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c:313
#3  0x000000007feb706f in CoreInstallMultipleProtocolInterfaces (Handle=0x7fe9eca8) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Hand/Handle.c
:586
#4  0x000000007fadfd58 in InitializeResetSystem (SystemTable=0x7f9ee018, ImageHandle=0x7f174218) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal
/ResetSystemRuntimeDxe/ResetSystem.c:190
#5  ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7f174218) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe/DEBUG/AutoGen.c:417
#6  _ModuleEntryPoint (ImageHandle=0x7f174218, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiDriverEntryPoint/DriverEntryP
oint.c:127
#7  0x000000007feba968 in CoreStartImage (ImageHandle=0x7f174218, ExitDataSize=0x0, ExitData=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe
/Image/Image.c:1654
#8  0x000000007feb189c in CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:523
#9  CoreDispatcher () at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/Dispatcher/Dispatcher.c:404
#10 0x000000007feaaafd in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:508
#11 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#12 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#13 0x000000007fee10cf in InternalSwitchStack ()
#14 0x0000000000000000 in ?? ()
```

## 我们可以直通 keyboard 吗
- FileInterfaceStdInRead
  - `gBS->WaitForEvent`
  - `Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);`

- [ ] 也许调查一下 gKeyboardControllerDriver 是如何被使用的



## notes
似乎只是支持下面两个:
```c
int raise(int sig);
__sighandler_t  *signal(int sig, __sighandler_t *func);
```
和 /usr/include/signal.h 对比，这个几乎叫做什么都没有实现啊

我们发现无法简单的使用 setitimer， 出现问题的位置不在于当时调用的，而是参数 NotifyTpl 中的，
在 CoreCreateEventEx 中间会对于


## poll
所以，实际上就是会卡到这个代码上，不会出现异步的情况

### 附录

#### source code
```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <time.h>

int main(int argc, char *argv[]) {
  int numPipes, ready, j;
  struct pollfd *pollFd;
  int pfds[1]; /* File descriptors for all pipes */

  /* Allocate the arrays that we use. The arrays are sized according
     to the number of pipes specified on command line */

  numPipes = 1;

  pollFd = calloc(numPipes, sizeof(struct pollfd));
  if (pollFd == NULL) {
    exit(EXIT_FAILURE);
  }

  /* Create the number of pipes specified on command line */

  for (j = 0; j < numPipes; j++)
    pfds[j] = 0;

  /* Build the file descriptor list to be supplied to poll(). This list
     is set to contain the file descriptors for the read ends of all of
     the pipes. */

  for (j = 0; j < numPipes; j++) {
    pollFd[j].fd = pfds[j];
    pollFd[j].events = POLLIN;
  }

  printf("huxueshi:%s before \n", __FUNCTION__);
  ready = poll(pollFd, numPipes, -1);
  printf("huxueshi:%s after \n", __FUNCTION__);
  if (ready == -1) {
    exit(EXIT_FAILURE);
  }

  printf("poll() returned: %d\n", ready);

  /* Check which pipes have data available for reading */

  for (j = 0; j < numPipes; j++)
    if (pollFd[j].revents & POLLIN) {
      printf("Readable: %3d\n", pollFd[j].fd);
      char x[100];
      scanf("%s", x);
      printf("[%s]", x);
    }

  exit(EXIT_SUCCESS);
}
```

#### backtrace

```c
/*
#0  da_ConPoll (filp=0x7e06e018, events=1) at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/LibC/Uefi/Devices/Console/daConsole.c:658
#1  0x000000007e001bbd in poll (nfds=1, timeout=-1, pfd=0x7e06a038) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/BaseDebugLibNull/DebugLib.c:166
#2  main (argc=<optimized out>, argv=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/AppPkg/Applications/Main/Main.c:76
#3  ShellAppMain (Argc=<optimized out>, Argv=0x7e076d98) at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/LibC/Main/Main.c:182
#4  0x000000007e0052f9 in ShellCEntryLib (SystemTable=0x7f9ee018, ImageHandle=0x7e076898) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Library/UefiShellCEn
tryLib/UefiShellCEntryLib.c:84
#5  ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7e076898) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/AppPkg/DEBUG_GCC5/X64/AppPkg/App
lications/Main/Main/DEBUG/AutoGen.c:375
#6  _ModuleEntryPoint (ImageHandle=0x7e076898, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiApplicationEntryPoint/Applica
tionEntryPoint.c:59
#7  0x000000007feba8b5 in CoreStartImage (ImageHandle=0x7e076898, ExitDataSize=0x0, ExitData=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe
/Image/Image.c:1653
#8  0x000000007df658b7 in InternalShellExecuteDevicePath (ParentImageHandle=0x7dfe5ad8, DevicePath=DevicePath@entry=0x7e076a98, CommandLine=CommandLine@entry=0x7e08e69
8, Environment=Environment@entry=0x0, StartImageStatus=StartImageStatus@entry=0x7fe9e718) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Sh
ellProtocol.c:1540
#9  0x000000007df68ab4 in RunCommandOrFile (CommandStatus=0x0, ParamProtocol=0x7e0ca198, FirstParameter=0x7e08e198, CmdLine=0x7e08e698, Type=Efi_Application) at /home/
maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2505
#10 SetupAndRunCommandOrFile (CommandStatus=0x0, ParamProtocol=0x7e0ca198, FirstParameter=0x7e08e198, CmdLine=<optimized out>, Type=Efi_Application) at /home/maritns3/
core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2589
#11 RunShellCommand (CommandStatus=0x0, CmdLine=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2713
#12 RunShellCommand (CmdLine=<optimized out>, CommandStatus=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2625
#13 0x000000007df0c0ed in RunCommand (CmdLine=0x7e077018) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2765
#14 RunScriptFileHandle (Name=<optimized out>, Handle=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:3026
#15 RunScriptFile (Handle=0x0, ParamProtocol=<optimized out>, CmdLine=<optimized out>, ScriptPath=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Shel
lPkg/Application/Shell/Shell.c:3130
#16 RunScriptFile.constprop.0 (ScriptPath=<optimized out>, CmdLine=<optimized out>, ParamProtocol=<optimized out>, Handle=0x0) at /home/maritns3/core/ld/edk2-workstati
on/edk2/ShellPkg/Application/Shell/Shell.c:3099
#17 0x000000007df6c15a in DoStartupScript (FilePath=0x7e0c8318, ImagePath=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/S
hell.c:1282
#18 UefiMain (ImageHandle=<optimized out>, SystemTable=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:594
#19 0x000000007df5052d in ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7f130218) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DE
BUG_GCC5/X64/ShellPkg/Application/Shell/Shell/DEBUG/AutoGen.c:1013
#20 _ModuleEntryPoint (ImageHandle=0x7f130218, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiApplicationEntryPoint/Applica
tionEntryPoint.c:59
#21 0x000000007feba8b5 in CoreStartImage (ImageHandle=0x7f130218, ExitDataSize=0x7e1e06c8, ExitData=0x7e1e06c0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModu
lePkg/Core/Dxe/Image/Image.c:1653
#22 0x000000007f05d5e2 in EfiBootManagerBoot (BootOption=BootOption@entry=0x7e1e0678) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/UefiBootMana
gerLib/BmBoot.c:1982
#23 0x000000007f060ca2 in BootBootOptions (BootManagerMenu=0x7fe9ecd8, BootOptionCount=5, BootOptions=0x7e1e0518) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeMo
dulePkg/Universal/BdsDxe/BdsEntry.c:409
#24 BdsEntry (This=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/BdsDxe/BdsEntry.c:1072
#25 0x000000007feaabe3 in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:551
#26 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#27 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#28 0x000000007fee10cf in InternalSwitchStack ()
#29 0x0000000000000000 in ?? ()
*/
```

如果在正在 poll 的时候在 gdb 中 Ctrl + C，然后 bt

```c
/*
#0  KeyReadStatusRegister (ConsoleIn=ConsoleIn@entry=0x7ec76018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/BaseIoLibIntrinsic/IoLibGcc.c:50
#1  0x000000007edacc38 in KeyboardTimerHandler (Event=Event@entry=0x0, Context=Context@entry=0x7ec76018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/B
us/Isa/Ps2KeyboardDxe/Ps2KbdCtrller.c:807
#2  0x000000007edad371 in KeyboardReadKeyStrokeWorker (ConsoleInDev=ConsoleInDev@entry=0x7ec76018, KeyData=KeyData@entry=0x7fe9e1e4) at /home/maritns3/core/ld/edk2-wor
kstation/edk2/MdeModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2KbdTextIn.c:156
#3  0x000000007edad42d in KeyboardReadKeyStroke (This=<optimized out>, Key=0x7fe9e244) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Isa/Ps2Keyboard
Dxe/Ps2KbdTextIn.c:279
#4  0x000000007ede8f56 in ConSplitterTextInPrivateReadKeyStroke (Key=0x7fe9e2ac, Private=0x7edeccc0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Unive
rsal/Console/ConSplitterDxe/ConSplitter.c:3562
#5  ConSplitterTextInReadKeyStroke (This=<optimized out>, Key=0x7fe9e2ac) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/Console/ConSplitterDxe
/ConSplitter.c:3623
#6  0x000000007dff9538 in da_ConRawRead (filp=filp@entry=0x7e074018, Character=Character@entry=0x7e091344 L"") at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/L
ibC/Uefi/Devices/Console/daConsole.c:257
#7  0x000000007dff9626 in da_ConPoll (filp=0x7e074018, events=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/LibC/Uefi/Devices/Console/daConso
le.c:672
#8  0x000000007e001bbd in poll (nfds=1, timeout=-1, pfd=0x7e06af38) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/BaseDebugLibNull/DebugLib.c:166
*/
```
