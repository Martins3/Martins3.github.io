## Foundation

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
