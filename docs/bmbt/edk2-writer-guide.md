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

The following two basic types of events can be created:
- EVT_NOTIFY_SIGNAL
- EVT_NOTIFY_WAIT

The type of event determines when an event's notification function is invoked.
The notification function for signal type events is invoked when an event is placed into the signaled state with a call to `SignalEvent()`.
The notification function for wait type events is invoked when the event is passed to the `CheckEvent()` or `WaitForEvent()` services.


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
UEFI drivers following the UEFI driver model are required to implement the Driver Binding Protocol. This requirement includes the following drivers:
- Device drivers
- Bus drivers
- Hybrid drivers

*Root bridge driver, service drivers, and initializing drivers* do not produce this protocol.

- [ ] 看来几个 driver 都是没有分清楚了啊

The EFI_DRIVER_BINDING_PROTOCOL is installed onto the driver's image handle.
- [ ] 什么叫做 image handle，什么叫做 EFI_DRIVER_BINDING_PROTOCOL install 上去了

It is possible for a driver to produce more than one instance of the Driver Binding Protocol. All additional instances of the Driver Binding Protocol must be installed onto new handles.

The Driver Binding Protocol can be installed directly using the UEFI Boot Service `InstallMultipleProtocolInterfaces()`.

### 9.1
Installs all the Driver Binding Protocols in the driver entry point

## 10 UEFI Service Binding Protocol
- [ ] 和 chapter 9 是啥关系

The Service Binding Protocol is not associated with a single GUID value.
Instead, each Service Binding Protocol GUID value is paired with another protocol providing a specific set of services.
The protocol interfaces for all Service Binding Protocols are identical and contain the services `CreateChild()` and `DestroyChild()`.
When `CreateChild()` is called, a new handle is created with the associated protocol installed. When `DestroyChild()` is called,
the associated protocol is uninstalled and the handle is freed.

```c
///
/// The EFI_SERVICE_BINDING_PROTOCOL provides member functions to create and destroy
/// child handles. A driver is responsible for adding protocols to the child handle
/// in CreateChild() and removing protocols in DestroyChild(). It is also required
/// that the CreateChild() function opens the parent protocol BY_CHILD_CONTROLLER
/// to establish the parent-child relationship, and closes the protocol in DestroyChild().
/// The pseudo code for CreateChild() and DestroyChild() is provided to specify the
/// required behavior, not to specify the required implementation. Each consumer of
/// a software protocol is responsible for calling CreateChild() when it requires the
/// protocol and calling DestroyChild() when it is finished with that protocol.
///
struct _EFI_SERVICE_BINDING_PROTOCOL {
  EFI_SERVICE_BINDING_CREATE_CHILD         CreateChild;
  EFI_SERVICE_BINDING_DESTROY_CHILD        DestroyChild;
};
```

## 11

## 18
The PCI bus driver consumes the services of the `PCI_ROOT_BRIDGE_IO_PROTOCOL` and uses those services to enumerate the PCI controllers present in the system.
In this example, the PCI bus driver detected a disk controller, a graphics controller, and a USB host controller.
As a result, the PCI bus driver produces three child handles with the `EFI_DEVICE_PATH_PROTOCOL` and the `EFI_PCI_IO_PROTOCOL`.

- [ ] 当检测到了三个 child 的时候，那么就会产生三个 EFI_DEVICE_PATH_PROTOCOL 出来

```c
/**
  This protocol can be used on any device handle to obtain generic path/location
  information concerning the physical device or logical device. If the handle does
  not logically map to a physical device, the handle may not necessarily support
  the device path protocol. The device path describes the location of the device
  the handle is for. The size of the Device Path can be determined from the structures
  that make up the Device Path.
**/
typedef struct {
  UINT8 Type;       ///< 0x01 Hardware Device Path.
                    ///< 0x02 ACPI Device Path.
                    ///< 0x03 Messaging Device Path.
                    ///< 0x04 Media Device Path.
                    ///< 0x05 BIOS Boot Specification Device Path.
                    ///< 0x7F End of Hardware Device Path.

  UINT8 SubType;    ///< Varies by Type
                    ///< 0xFF End Entire Device Path, or
                    ///< 0x01 End This Instance of a Device Path and start a new
                    ///< Device Path.

  UINT8 Length[2];  ///< Specific Device Path data. Type and Sub-Type define
                    ///< type of data. Size of data is included in Length.

} EFI_DEVICE_PATH_PROTOCOL;
```


## 30
按照 30 的操作执行就可以了
