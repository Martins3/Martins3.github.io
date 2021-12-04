## 继续分析代码
- [ ] 不知道对于 signal 的支持到底有多强
- [ ] poll 之类的实现了解一下

- [ ] EFI_BOOT_SERVICES 的这个结构体什么时候注册的

- [ ] 测试一下信号机制
- [ ] CoreLoadPeImage 为什么不是 AppPkg 的基础设施
- [ ] MdePkg 和 MdeModulePkg 的关系是啥呀?
  - 分析其中主要的代码有什么
- [ ] 那么还可以检查 TLB refill 的入口吗?
  - [ ] 类似 la 的这种总是在虚拟地址上的怎么处理的呀

- [ ] 重新修改一下 uefi 的 compile_commands.json 的脚本，让所有的 compile_commands 都是自动注入的 home 上的

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

[^1]3.7 分钟分析 LoadImage 的实现:
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

## `_ModuleEntryPoint` 的写法是从哪里来的
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
      - `SystemTable->BootServices->OpenProtocol`
      - ShellAppMain
        - `gMD = AllocateZeroPool(sizeof(struct __MainData))` : `__MainData` 记录一些 argc argV 之类的东西，是的，我们的程序是不需要链接器的
        - main
  - ProcessLibraryDestructorList

再看 ShellAppMain 的程序，其只是从 ShellAppMain 开始的而已:

调用的入口其实是自动生成的:
/home/maritns3/core/ld/edk2-workstation/edk2/Build/AppPkg/DEBUG_GCC5/X64/AppPkg/Applications/Main/Main/DEBUG/AutoGen.c


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

## 无法理解一些东西啊
每次加载的时候都是相同的位置:
Loading driver at 0x0007E5C2000 EntryPoint=0x0007E5CE00C Main.efi

发现，不是 ccls 跳转的问题，而是 UEFI 的协议确实让这些就是从内存中加载的:
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

## Res
EFI_MM_SYSTEM_TABLE;
EFI_LOADED_IMAGE_PROTOCOL

- EFI_SYSTEM_TABLE
  - EFI_BOOT_SERVICES
  - EFI_RUNTIME_SERVICES

[^1]: edk-ii-uefi-driver-writer-s-guide
