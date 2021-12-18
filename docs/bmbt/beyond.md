虽然 chapter 4 的名称叫做 protocols you should know，但是实际上描述一个 os loader 应该做什么的

Table 5.2 中说，EfiBootServicesCode 和 EfiBootServicesData 将会变为普通使用的 memory，如果我们继续使用，只要保证以后的 os 不会访问这些内容，是不是就不会存在问题的?

首先，搞个串口输出，不要对于 UEFI 进行任何的侵入, 验证一下想法，确定需要移植的接口。
之后，从 UEFI 中间确定内存分配的代码，以及各种需要移植的代码，总体来说，感觉还行吧。

既然，os loader 可以调用 ExitBootServices 的，那么说明，
难道 ExitBootServices 连自己都会删除掉自己吗? 那么这些 Application 是放到哪里的，是算作什么类型的 Memory 的?

Assuming you are new to UEFI, the following introduction explains a few of the key UEFI concepts in a helpful framework to keep in mind as you study the specification:[^2]
- Objects managed by UEFI-based firmware - used to manage system state, including I/O devices, memory, and events
- The UEFI System Table - the primary data structure with data information tables and function calls to interface with the systems
- Handle database and protocols - the means by which callable interfaces are registered
- UEFI images - the executable content format by which code is deployed
- Events - the means by which software can be signaled in response to some other activity
- Device paths - a data structure that describes the hardware location of an entity, such as the bus, spindle, partition, and file name of an UEFI image on a formatted disk.

- [ ] The UEFI 2.6 Specification defines over 30 different protocols, and various implementations of UEFI firmware and UEFI drivers may produce additional protocols to extend the functionality of a platform.
  - 这些除了 Boot 和 Runtime 的 protocols 居然存在 UEFI 的规定呀

Any UEFI code can operate with protocols during boot time. However, after `ExitBootServices()` is called, the handle database is no longer available. Several UEFI boot time services work with UEFI protocols.[^2]

However, drivers may create multiple instances of a particular protocol and attach each instance to a different handle.[^2]


To accomplish this task, several important steps must be taken:
- The OS loader must determine from where it was loaded. This determination allows an OS loader to retrieve additional files from the same location.[^4]
  - [ ] 什么意思啊? loaded 的位置还能发生改变 ?
  - [ ] additional files 是什么意思 ?

The DXE phase contains an implementation of UEFI that is compliant with the PI (Platform Initialization) Specification.
As a result, both the DXE Core and DXE drivers share many of the attributes of UEFI images.
The DXE phase is the phase where most of the system initialization is performed. The Pre-EFI Initialization (PEI) phase is responsible for initializing permanent memory in the platform so the DXE phase can be loaded and executed.
The state of the system at the end of the PEI phase is passed to the DXE phase through a list of position-independent data structures called Hand-Off Blocks (HOBs).
The DXE phase consists of several components:[^8]
- DXE Core
- DXE Dispatcher
- DXE Drivers

- [ ] 什么叫做 PI (Platform Initialization) Specification.

The DXE Core produces a set of Boot Services, Runtime Services, and DXE Services.
The DXE Dispatcher is responsible for discovering and executing DXE drivers in the correct order.
The DXE drivers are responsible for initializing the processor, chipset, and platform components as well as providing software abstractions for console and boot devices.
These components work together to initialize the platform and provide the services required to boot an OS.[^8]

The DXE Core produces the EFI System Table and its associated set of EFI Boot Services and EFI Runtime Services. The DXE Core also contains the DXE Dispatcher, whose main purpose is to discover and execute DXE drivers stored in firmware volumes.

从 Figure 2.4 才知道:
- UEFI Driver Model driver
- Application
- Services Driver
主要的区别在于是否处理 device 和产生 protocol 了

## 使用 gDxeCoreImageHandle 为例子了解一下 HANDLE 的作用
- 看文档，老是说，HANDLE 是一组 protocols，没有其他的功能吗?
- 实际上，似乎更加重要的功能在于

- [ ] 那么 IHANDLE 的作用是啥啊?
```c
///
/// IHANDLE - contains a list of protocol handles
///
typedef struct {
  UINTN               Signature;
  /// All handles list of IHANDLE
  LIST_ENTRY          AllHandles;
  /// List of PROTOCOL_INTERFACE's for this handle
  LIST_ENTRY          Protocols;
  UINTN               LocateRequest;
  /// The Handle Database Key value when this handle was last created or modified
  UINT64              Key;
} IHANDLE;
```

gDxeCoreImageHandle 的初始化

- CoreInitializeImageServices
  - CoreInstallProtocolInterface : 实际上，是分配的 IHANDLE 的结构体，最后放到 HANDLE 上的
```c
  //
  // Install the protocol interfaces for this image
  //
  Status = CoreInstallProtocolInterface (
             &Image->Handle,
             &gEfiLoadedImageProtocolGuid,
             EFI_NATIVE_INTERFACE,
             &Image->Info
             );
```

gDxeCoreImageHandle 的使用

- CoreDispatcher
  - 从 mScheduledQueue 中取出 EFI_CORE_DRIVER_ENTRY 来
  - CoreLoadImageCommon
    - CoreLoadedImageInfo : 获取 ParentImage
      - CoreHandleProtocol : 使用参数 UserHandle 和 Protocol=gEfiLoadedImageProtocolGuid 来访问
        - CoreOpenProtocol
          - CoreGetProtocolInterface : Locate a certain GUID protocol interface in a Handle's protocols.
            - Handle = (IHANDLE *)UserHandle;
            - HADNLE 持有的链表中，通过比对 guid 来找到 PROTOCOL_INTERFACE
          - 将找到的 PROTOCOL_INTERFACE 进一步比对 Interface 之类的操作
          - 从 PROTOCOL_INTERFACE::Interface 中获取 interface 也就是 ParentImage，也就是曾经注册的 Image->Info
    - GetFileBufferByFilePath : 这个会根据路径，将 image 读去到 source 中间
    - CoreInstallProtocolInterfaceNotify
    - CoreLoadPeImage
    - CoreReinstallProtocolInterface

以为是两个组合拳
- CoreInstallProtocolInterface
- CoreHandleProtocol : Queries a handle to determine if it supports a specified protocol.
  - OpenProtocol : Locates the installed protocol handler for the handle, and invokes it to obtain the protocol interface.
好吧，原来 CoreHandleProtocol 只是 CoreOpenProtocol 的简单封装

[^2]: chapter 2
[^4]: chapter 4
[^8]: chapter 8
