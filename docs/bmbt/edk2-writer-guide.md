- [ ] 顺便读读这个东西吧: https://edk2-docs.gitbook.io/edk-ii-dsc-specification/

- [ ] 分析一下关于 Start() 和 Unload，也就是 chapter 7 中间的内容，下面是 Start 的例子
```c
/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
NvmExpressDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );
```
- [ ] EFI_EVENT 是个啥

## Foundation
- [ ] 关于 Binding 的问题，如果一个 driver 创建出来了，如何提供接口给另一个 driver 用

### 3.1
There is only one interrupt: the timer. This means that data structures accessed by both in-line code and timer-driven code must take care to synchronize access to critical paths. This is accomplished via privilege levels.

- [x] 既然可以处理串口，那么串口中断算什么?
  - 串口是通过轮询实现的，所以没有串口中断的
- [ ] 为什么 privilege levels 来实现 cirtical paths

### 3.3
通过 UEFI system table 可以实现访问三个内容:
1. UEFI boot services
2. UEFI runtime services
3. Services provided by protocols

在 EFI_SYSTEM_TABLE 直接持有 EFI_BOOT_SERVICES EFI_RUNTIME_SERVICES

> Protocol services are groups of related functions and data fields that are named by a Globally Unique Identifier (GUID; see Appendix A of the UEFI Specification).
- [ ] 暂时没有找到到底是什么设备来处理这些的

### 3.6
The extensible nature of UEFI is built, to a large degree, around protocols. Protocols serve to enable communication between separately built modules, including drivers.

Every protocol has a GUID associated with it.

Protocols are gathered into a single database. The database is not "flat."Instead, it allows protocols to be grouped together. Each group is known as a handle, and the handle is also the data type that refers to the group.

三个层次: database handle protocols , handle 应该是处理一类的，比如访存，而 protocol 就是一个具体的实现。

注册 protocols 和访问 protocols:
- InstallMultipleProtocolInterfaces()

EFI_MM_SYSTEM_TABLE

### 3.7
- [ ] 据说有的 driver 永远不会消失，application 是会消失的

> Creates a new image handle in the handle database, which installs an instance of the EFI_LOADED_IMAGE_PROTOCOL

为什么每一个 image 都是需要注册 image handle 啊

The ability to stay resident in system memory is what differentiates a driver from an application.

> Only the services produced by runtime drivers are allowed to persist past ExitBootServices().

那么操作系统怎么知道 UEFI 的 driver 占用一些空间啊

A driver that produces one or more protocols on one or more new service handles and returns EFI_SUCCESS from its entry point.

> 创建 service handle 或者 protocols

- [ ] EfiRuntimeServicesCode
- [ ] EfiRuntimeServicesData

Runtime service 需要更加小心，因为其运行环境会从物理地址切换到虚拟地址上


### 3.8
| **Type of events** | **Description** |
| ----------------------------- | ---------------------------------------------------------------------------------------------------- |
| Wait event | An event whose notification function is executed whenever the event is checked or waited upon. |
| Signal event | An event whose notification function is scheduled for execution whenever the event goes from the waiting state to the signaled state. |
| Exit Boot Services event | A special type of signal event that is moved from the waiting state to the signaled state when the EFI Boot Service `ExitBootServices()` is called. This call is the point in time when ownership of the platform is transferred from the firmware to an operating system. The event's notification function is scheduled for execution when `ExitBootServices()` is called. |
| Set Virtual Address Map event | A special type of signal event that is moved from the waiting state to the signaled state when the UEFI runtime service `SetVirtualAddressMap()` is called. This call is the point in time when the operating system is making a request for the runtime components of UEFI to be converted from a physical addressing mode to a virtual addressing mode. The operating system provides the map of virtual addresses to use. The event's notification function is scheduled for execution when `SetVirtualAddressMap()` is called. |
| Timer event | A type of signal event that is moved from the waiting state to the signaled state when at least a specified amount of time has elapsed. Both periodic and one-shot timers are supported. The event's notification function is scheduled for execution when a specific amount of time has elapsed. |
| Periodic timer event | A type of timer event that is moved from the waiting state to the signaled state at a specified frequency. The event's notification function is scheduled for execution when a specific amount of time has elapsed. | | One-shot timer event | A type of timer event that is moved from the waiting state to the signaled state after the specified time period has elapsed. The event's notification function is scheduled for execution when a specific amount of time has elapsed. The following three elements are associated with every event: * The task priority level (TPL) of the event * A notification function * A notification context The notification function for a wait event is executed when the state of the event is checked or when the event is being waited upon. The notification function of a signal event is executed whenever the event transitions from the waiting state to the signaled state. The notification context is passed into the notification function each time the notification function is executed. The TPL is the priority at which the notification function is executed. The four TPL levels that are defined in UEFI are listed in the table below.

- [ ] 检查一下什么时候 x86 调用过 SetVirtualAddressMap 的

| Task Priority Level | Description                                                                               |
|---------------------|-------------------------------------------------------------------------------------------|
| TPL_APPLICATION     | The priority level at which UEFI images are executed.                                     |
| TPL_CALLBACK        | The priority level for most notification functions.                                       |
| TPL_NOTIFY          | The priority level at which most I/O operations are performed.                            |
| TPL_HIGH_LEVEL      | The priority level for the one timer interrupt supported in UEFI. (Not usable by drivers) |

A lock can be created by temporarily raising the task priority level to `TPL_HIGH_LEVEL`.

- [ ] 无法理解，定义出来两个中断就差不多了吧，

## 4
There are a few portability issues that apply specifically to IPF and EBC, and these are presented in individual sections later in this guide as well.
- [ ] IPC 和 EBC 是什么东西
### 4.1
The EDK II contains many more UEFI drivers than those listed in Appendix B.

### 4.2
- [ ] 4.2.11 和 4.2.12 没太看懂

> Because UEFI drivers now have HII functionality, the UEFI Driver Model requires that no console I/O operations take place in the UEFI Driver Binding Protocol functions.

- [ ] 4.2.17 中，到底什么是 HII 啊

## 5
### 5.1 Services that UEFI drivers commonly use

#### 5.1.4 Task Priority Level(TPL) Services
- One use case is a UEFI Driver that is required to raise the priority because the implementation of a service of a specific protocol requires execution at a specific TPL to be UEFI conformant.
- Another use case is a UEFI Driver that needs to implement a simple lock, or critical section, on global data structures maintained by the UEFI Driver.

**Event notification functions, covered in the next section, always execute at raised priority levels.**

#### 5.1.5 Event services
> Implementation of protocols that produce an EFI_EVENT to inform protocol consumers when input is available.

The type of event determines when an event's notification function is invoked.

#### 5.1.6 SetTimer()
首先创建 Event 的，然后创建出来 timer 的

#### 5.1.7 Stall()

## 7
The driver entry point is the function called when a UEFI driver is started with the `StartImage()` service.
At this point the driver has already been loaded into memory with the `LoadImage()` service.

The image handle of the UEFI driver and a pointer to the UEFI system table are passed into every UEFI driver.
The image handle allows the UEFI driver to discover information about itself,
and the pointer to the UEFI system table allows the UEFI driver to make UEFI Boot Service and UEFI Runtime Service calls.

## 8 Private Context Data Structures
- [ ] 分配私有数据有什么神奇的地方吗?

## 9 Driver Binding Protocol
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

## 10 UEFI Service Binding Protocol
- [ ] 和 chapter 9 是啥关系

## 11

## 30
按照 30 的操作执行就可以了
