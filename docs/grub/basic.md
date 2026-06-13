# GRUB 配置详解

### /etc/grub.d

/etc/grub.d/ 是 GRUB2 的启动菜单生成脚本目录。当你运行 grub2-mkconfig 命令时，
它会按数字顺序执行这些脚本，拼接生成最终的 /etc/grub2.cfg 或者 /etc/grub2-efi.cfg

- 00_header : 生成 GRUB 的全局头部配置（图形终端、超时时间、默认项、终端设置等）
- 10_linux : 核心脚本，自动检测本机安装的 Linux 内核并生成启动菜单项

/etc/grub2.cfg 和 /etc/grub2-efi.cfg 都是软链接，指向到这里。

## 产物

### bios 目录

fonts grub.cfg grubenv

/boot/grub2/grubenv /boot/efi/EFI/openEuler/grubenv

存在这个软链接:

```txt
/boot/grub2/grubenv -> ../efi/EFI/openEuler/grubenv
```

### uefi 目录

```txt
root@localhost:/boot/efi/EFI# tree
.
├── BOOT
│   ├── BOOTX64.EFI
│   └── fbx64.efi
└── fedora
    ├── BOOTX64.CSV
    ├── grub.cfg
    ├── grubx64.efi
    ├── mmx64.efi
    ├── shim.efi
    └── shimx64.efi
```

## 记录一次 grub 修复

系统只能进入的 grub shell 中

1. ls 可以看到很多盘，例如 (hd0, gpt3) (hd0,gpt1)
2. 逐个尝试，看看那个盘是正常的，
3. 在此进入到 grub shell 中，linux /vmlinux root=/dev/sdb3

登录入系统，检查内容: 发现启动项是没问题的:

```txt
$ efibootmgr
BootCurrent: 0007
Timeout: 3 seconds
BootOrder: 000D,0007,0005,0006,0008,0009,000A,000C,0004,0000,0001,0002,0003,000B
Boot0000  Enter Setup
Boot0001  UEFI BootManagerMenuApp
Boot0002  UEFI PXE App
Boot0003  OBR OS
Boot0004* UEFI Shell
Boot0005* UEFI Hard Drive 0
Boot0006* UEFI Hard Drive 1
Boot0007* UEFI Hard Drive 2
Boot0008* UEFI Hard Drive 3
Boot0009* UEFI Hard Drive 4
Boot000A* UEFI Hard Drive 5
Boot000B  Kunlun Update Tool
Boot000C* UEFI OpenBMC Virtual Media Device
Boot000D* openEuler
```

检查 grub 配置，其中的发现已经被清空了:

```txt
$ ls -la /etc/grub2-efi.cfg
lrwxrwxrwx. 1 root root 34 Jan  1  2022 /etc/grub2-efi.cfg -> ../boot/efi/EFI/openEuler/grub.cfg
```

重新生成即可。

总结，相当于 grub.cfg 损坏了，所以手动从 grub shell 中进入。

参考: https://linuxhint.com/grub_rescue_ubuntu_1804/

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
