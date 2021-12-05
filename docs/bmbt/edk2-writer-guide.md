- [ ] 顺便读读这个东西吧: https://edk2-docs.gitbook.io/edk-ii-dsc-specification/

## Foundation
- [ ] 关于 Binding 的问题，如果一个 driver 创建出来了，如何提供接口给另一个 driver 用

### 3.1
There is only one interrupt: the timer. This means that data structures accessed by both in-line code and timer-driven code must take care to synchronize access to critical paths. This is accomplished via privilege levels.

- [ ] 既然可以处理串口，那么串口中断算什么?
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
- [ ] 5.1.4 Task Priority Level(TPL) Services : 权限问题到现在还是没能理解

## 7
The driver entry point is the function called when a UEFI driver is started with the `StartImage()` service.
At this point the driver has already been loaded into memory with the `LoadImage()` service.

The image handle of the UEFI driver and a pointer to the UEFI system table are passed into every UEFI driver. The image handle allows the UEFI driver to discover information about itself, and the pointer to the UEFI system table allows the UEFI driver to make UEFI Boot Service and UEFI Runtime Service calls.

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
