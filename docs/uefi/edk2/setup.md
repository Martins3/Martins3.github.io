# Linux UEFI 学习环境搭建

<!-- vim-markdown-toc GitLab -->

* [运行第一个 UEFI 程序](#运行第一个-uefi-程序)
	* [编译 efi](#编译-efi)
	* [运行 efi](#运行-efi)
* [交叉编译](#交叉编译)
* [构建基于 StdLib 的 HelloWorld](#构建基于-stdlib-的-helloworld)
* [在 Linux 上调试 edk2](#在-linux-上调试-edk2)
	* [使用 gdb 调试](#使用-gdb-调试)
	* [使用 debugcon 调试](#使用-debugcon-调试)
* [内核作为 efi 文件启动](#内核作为-efi-文件启动)
* [让程序运行 shell 命令](#让程序运行-shell-命令)
* [使用 Rust 编写 UEFI Application](#使用-rust-编写-uefi-application)
* [资源](#资源)

<!-- vim-markdown-toc -->

## 运行第一个 UEFI 程序
### 编译 efi
参考教程 https://www.rodsbooks.com/efi-programming/hello.html
但是这个教程有点老，参考 [stackoverflow](https://stackoverflow.com/questions/31514866/how-to-compile-uefi-application-using-gnu-efi/31517520) 可以修复。

使用 vn/code/module/gnuefi/ 来构建。


### 运行 efi
参考 [osdev](https://wiki.osdev.org/UEFI#Linux.2C_root_not_required) 上，我构建出来了一个小脚本

[uefi.sh](https://github.com/Martins3/Martins3.github.io/tree/master/docs/uefi/uefi/uefi.sh)，其参数为将要测试的 efi.
然后在 QEMU 的图形界面中，可以看到 UEFI shell, 在其中输入 `FS0:`，最后执行程序。

## 交叉编译
类似，如果想要在 x86 电脑上编译安装 ARM 版本的 edk2，其 Conf/target.txt 对应的配置为:
```txt
ACTIVE_PLATFORM       = ArmVirtPkg/ArmVirtQemu.dsc
TARGET_ARCH           = AARCH64
TOOL_CHAIN_TAG        = GCC5
MAX_CONCURRENT_THREAD_NUMBER = 50
```

参考[这篇 blog](https://damn99.com/2021-06-19-edk2-cross-build-for-amd64/)

运行 build 之前，首先执行：
```sh
export GCC5_AARCH64_PREFIX=aarch64-linux-gnu-          # ubuntu 中
export GCC5_AARCH64_PREFIX=aarch64-unknown-linux-gnu-  # nixos 中
```

## 构建基于 StdLib 的 HelloWorld
上面是调用原生的 uefi 接口来构建的程序，实际上，UEFI 提供了 [StdLib](https://github.com/tianocore/edk2-libc)，其尽可能提供和 glibc 相同的接口，这样，很多用户态程序几乎不需要做任何修改就可以
直接编译为 .efi 文件，在 UEFI shell 中运行了。

使用方法很简单:
```sh
# 下载
git clone https://github.com/tianocore/edk2-libc
# 将 edk2-libc 的内容拷贝到 edk2 中
mv edk2-libc/* path/to/edk2
cd path/to/edk2
# 编译
build -p AppPkg/AppPkg.dsc
```
其实 edk2-libc 主要就是两个文件夹:
1. StdLib : 利用 UEFI native 的接口实现 glib 的接口
2. AppPkg : 各种测试程序，甚至包括 lua 解释器


## 在 Linux 上调试 edk2
### 使用 gdb 调试

https://github.com/tianocore/tianocore.github.io/wiki/How-to-debug-OVMF-with-QEMU-using-GDB
这里需要手动计算偏移，不是 debuginfo 的格式有问题，而是 debuginfo 不知道 qemu 会把 efi 加载到哪里，。
所以


这里介绍了如何自动加载 debuginfo
https://retrage.github.io/2019/12/05/debugging-ovmf-en.html

其实就是 edk2 生成的符号信息进行一些转换之后才可以被 gdb 识别，


1. 准备环节，只需要操作一次
```sh
# 生成 /tmp/ovmf.log 启动包含各个 module 加载的地址信息
./uefi.sh
# 根据 /tmp/ovmf.log 和 Build 下 .debug 生成 gdb 可识别调试信息
./uefi.sh -g
```
2. 调试:
```sh
# 在第一次窗口，启动 QEMU
./uefi.sh -s
# 在第二个窗口，启动 gdb
./uefi.sh -d
```

最后效果:
![](./uefi/img/gdb.png)

需要注意的事情是，打断点需要使用 [hardware breakpoint](https://stackoverflow.com/questions/8878716/what-is-the-difference-between-hardware-and-software-breakpoints)

### 使用 debugcon 调试
在源码中添加调试语句，然后重新编译运行
```c
  DEBUG((DEBUG_INFO, "%s\n", "hello"));
```

## 内核作为 efi 文件启动
内核实际上可以作为 efi 文件在 UEFI 上执行,
具体参考[内核文档](https://www.kernel.org/doc/Documentation/efi-stub.txt)

## 让程序运行 shell 命令
参考[^2]

```c
#include <Library/ShellLib.h>
#include <Library/UefiLib.h>
#include <Uefi.h>

EFI_STATUS
EFIAPI
UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS Status;

  ShellExecute(&ImageHandle, L"echo Hello World!", FALSE, NULL, &Status);

  return Status;
}
```

```inf
## @file
#  A simple, basic, EDK II native, "hello" application.
#
#   Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
#   SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  INF_VERSION                    = 0x00010006
  BASE_NAME                      = Hello
  FILE_GUID                      = a912f198-7f0e-4803-b908-b757b806ec83
  MODULE_TYPE                    = UEFI_APPLICATION
  VERSION_STRING                 = 0.1
  ENTRY_POINT                    = UefiMain

#
#  VALID_ARCHITECTURES           = IA32 X64
#

[Sources]
  Hello.c

[Packages]
  MdePkg/MdePkg.dec
  ShellPkg/ShellPkg.dec

[LibraryClasses]
  UefiLib
  ShellCEntryLib
  ShellLib
```
## 使用 Rust 编写 UEFI Application
- 在 https://gil0mendes.io/blog/an-efi-app-a-bit-rusty/ 介绍了一下使用 Rust 构建 UEFI 的动机。
- 进一步的，在 https://github.com/rust-embedded/book 中介绍了在嵌入式项目中如何使用 Rust.

我们使用 [uefi-rs](https://github.com/rust-osdev/uefi-rs) 来感受一下。
进入到 template 目录中，按照 https://github.com/rust-osdev/uefi-rs/blob/master/BUILDING.md 操作即可。

相对 edk2 而言，uefi-rs 的代码量非常少，如果你恰好是 Rust 高手，读读其代码还是相当有意思的。

## 资源
- Robin 的 blog: http://yiiyee.cn/blog/
- https://wiki.osdev.org/GNU-EFI
- https://wiki.osdev.org/POSIX-UEFI
- https://edk2-docs.gitbook.io/edk-ii-build-specification/

[^1]: https://stackoverflow.com/questions/800030/remove-carriage-return-in-unix
[^2]: https://stackoverflow.com/questions/38738862/run-a-uefi-shell-command-from-inside-uefi-application

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
