# 在 Linux 快速入手 UEFI 指南

- [ ] 如何使用 QEMU 调试
- [ ] 也许阅读一下 https://edk2-docs.gitbook.io/edk-ii-uefi-driver-writer-s-guide/ 然后快速的感受一下其提供的接口是什么
- [ ] 所以 libc 可以干啥呀
- [ ] 写 UEFI 的程序可以干啥呀?

## 运行第一个 UEFI 程序

- 关于构建一个 hello world 的程序，分析每一个参数的使用
- https://www.rodsbooks.com/efi-programming/hello.html
  - https://stackoverflow.com/questions/31514866/how-to-compile-uefi-application-using-gnu-efi/31517520
    - 上面的教程很老，实际上编译不通过，需要在 Makefile 中进行一下修改
  - 实际上，osdev 上描述更加清楚好用


## 使用
source edksetup.sh

## compile_commands.json
https://bugzilla.tianocore.org/show_bug.cgi?id=2850

几乎是按照这个 patch 来搞的，但是似乎这个 patch 有点问题:
```txt
{'cmd': '"$(CC)" $(DEPS_FLAGS) $(CC_FLAGS) -c -o '
        '/home/maritns3/core/ld/edk2-workstation/edk2/Build/Bootloader/DEBUG_GCC5/X64/MdePkg/Library/BaseMemoryLib/BaseMemoryLib/OUTPUT/./CompareMemWrapper.obj '
        '$(INC) '
        '/home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/BaseMemoryLib/CompareMemWrapper.c',
 'deps': ['$(MAKE_FILE)',
          '$(DEBUG_DIR)/AutoGen.h',
          '$(WORKSPACE)/MdePkg/Library/BaseMemoryLib/CompareMemWrapper.c'],
 'target': '$(OUTPUT_DIR)/CompareMemWrapper.obj'}
```

最后的报错总是，结果发现在
Error: cc or cc_flags is not defined!

分析
/home/maritns3/core/ld/edk2-workstation/edk2/Build/Bootloader/DEBUG_GCC5/X64/TOOLS_DEF.X64 中内容，发现，原来是将
其中的 CC 修改为 CC_PATH，修改之后，这个 patch 就可以使用了。


## stdlib 的事情分析一下
https://github.com/tianocore/edk2-libc

- 很容易，按照文档，把这个项目拷贝进去就好了。

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

### 分下一下这几个项目吧
- sizeof : 可以启动的时候使用
- printf
- [ ] 似乎文件是无法打开的
- [ ] 不知道对于 signal 的支持到底有多强
- setjmp

## 第一个项目
几乎可以参照
https://damn99.com/2020-05-18-edk2-first-app/ 这个来写，但是需要在 .dsc 中添加上
```c
!include MdePkg/MdeLibs.dsc.inc
```

- https://blog.system76.com/post/139138591598/howto-uefi-qemu-guest-on-ubuntu-xenial-host
  - 分析了一下使用 ovmf 的事情，但是没有仔细看

- https://stackoverflow.com/questions/63725239/build-edk2-in-linux
  - 原来 GCC5 就是包含 GCC9 的，甚至 GCC10 也是没有问题的

https://unix.stackexchange.com/questions/52996/how-to-boot-efi-kernel-using-qemu-kvm


- https://wiki.osdev.org/GNU-EFI
- https://wiki.osdev.org/POSIX-UEFI

## GNU UEFI



## 如果启动不起来，也许是有帮助的
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

- https://www.kernel.org/doc/Documentation/efi-stub.txt
  - 实际上，内核上的是提供了对应的接口的

On the x86 and ARM platforms, a kernel zImage/bzImage can masquerade
as a PE/COFF image, thereby convincing EFI firmware loaders to load
it as an EFI executable.

The bzImage located in arch/x86/boot/bzImage must be copied to the EFI
System Partition (ESP) and renamed with the extension ".efi".

## EFI system Partition
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
- [ ] 那么 /boot/grub 的内容为什么可以被加载啊?

## omvf
- [ ] 那么 Loongson 上有没有这个东西啊
- [ ] 在物理机上的是什么样子的呀
- [ ] 变化体现在什么地方啊

## 资源
Robin 的 blog: http://yiiyee.cn/blog/
