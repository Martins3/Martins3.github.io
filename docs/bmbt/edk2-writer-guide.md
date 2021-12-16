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

### 3.1
There is only one interrupt: the timer. This means that data structures accessed by both in-line code and timer-driven code must take care to synchronize access to critical paths. This is accomplished via privilege levels.

- [x] 既然可以处理串口，那么串口中断算什么?
  - 串口是通过轮询实现的，所以没有串口中断的
- [ ] 为什么 privilege levels 来实现 cirtical paths

### 3.6
The extensible nature of UEFI is built, to a large degree, around protocols. Protocols serve to enable communication between separately built modules, including drivers.

Every protocol has a GUID associated with it.

Protocols are gathered into a single database. The database is not "flat."Instead, it allows protocols to be grouped together. Each group is known as a handle, and the handle is also the data type that refers to the group.

三个层次: database handle protocols , handle 应该是处理一类的，比如访存，而 protocol 就是一个具体的实现。

注册 protocols 和访问 protocols:
- InstallMultipleProtocolInterfaces()

EFI_MM_SYSTEM_TABLE

### 3.7
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
A lock can be created by temporarily raising the task priority level to `TPL_HIGH_LEVEL`.

- [ ] 无法理解，定义出来两个中断就差不多了吧，

## 4
There are a few portability issues that apply specifically to IPF and EBC, and these are presented in individual sections later in this guide as well.
- [ ] IPC 和 EBC 是什么东西
### 4.1
The EDK II contains many more UEFI drivers than those listed in Appendix B.

### 4.2
> Because UEFI drivers now have HII functionality, the UEFI Driver Model requires that no console I/O operations take place in the UEFI Driver Binding Protocol functions.

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



#### 5.1.6 SetTimer()
首先创建 Event 的，然后创建出来 timer 的

#### 5.1.7 Stall()

## 7
