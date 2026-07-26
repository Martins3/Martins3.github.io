## efibootmgr 和 efivar
<!-- 69be8b20-a57f-4b36-9b77-d60a4967b3f7 -->

```txt
efibootmgr version 17
usage: efibootmgr [options]
        -a | --active         sets bootnum active
        -A | --inactive       sets bootnum inactive
        -b | --bootnum XXXX   modify BootXXXX (hex)
        -B | --delete-bootnum delete bootnum
        -c | --create         create new variable bootnum and add to bootorder
        -C | --create-only      create new variable bootnum and do not add to bootorder
        -D | --remove-dups      remove duplicate values from BootOrder
        -d | --disk disk       (defaults to /dev/sda) containing loader
        -r | --driver         Operate on Driver variables, not Boot Variables.
        -e | --edd [1|3|-1]   force EDD 1.0 or 3.0 creation variables, or guess
        -E | --device num      EDD 1.0 device number (defaults to 0x80)
        -g | --gpt            force disk with invalid PMBR to be treated as GPT
        -i | --iface name     create a netboot entry for the named interface
        -l | --loader name     (defaults to "\EFI\openEuler\grub.efi")
        -L | --label label     Boot manager display label (defaults to "Linux")
        -m | --mirror-below-4G t|f mirror memory below 4GB
        -M | --mirror-above-4G X percentage memory to mirror above 4GB
        -n | --bootnext XXXX   set BootNext to XXXX (hex)
        -N | --delete-bootnext delete BootNext
        -o | --bootorder XXXX,YYYY,ZZZZ,...     explicitly set BootOrder (hex)
        -O | --delete-bootorder delete BootOrder
        -p | --part part        partition containing loader (defaults to 1 on partitioned devices)
        -q | --quiet            be quiet
        -t | --timeout seconds  set boot manager timeout waiting for user input.
        -T | --delete-timeout   delete Timeout.
        -u | --unicode | --UCS-2  handle extra args as UCS-2 (default is ASCII)
        -v | --verbose          print additional information
        -V | --version          return version and exit
        -w | --write-signature  write unique sig to MBR if needed
        -y | --sysprep          Operate on SysPrep variables, not Boot Variables.
        -@ | --append-binary-args file  append extra args from file (use "-" for stdin)
        -h | --help             show help/usage
```

https://nixos.wiki/wiki/Bootloader 中最后提到如何增加 efi

```sh
efibootmgr -c -d /dev/nvme0n1 -p 1 -L NixOS-boot -l '\EFI\NixOS-boot\grubx64.efi'
```

1. 注意，-p 1 来设置那个 partition 的。
2. 后面的那个路径需要将 boot 分区 mount 然后具体产看，还有一次是设置的 "\EFI\nixo\BOOTX64.efi"

sudo efibootmgr -o 0,1,2

## 2026-07-09 实机观察: efibootmgr 最后写哪里

当前是 UEFI 启动，`efivarfs` 已挂载:

```txt
efivarfs on /sys/firmware/efi/efivars type efivarfs (rw,nosuid,nodev,noexec,relatime)
```

所以 `efibootmgr` 最后不是写 ESP 分区里的文件，而是写 EFI runtime variable。
在 Linux 上对应的文件视图是:

```txt
/sys/firmware/efi/efivars/BootOrder-8be4df61-93ca-11d2-aa0d-00e098032b8c
/sys/firmware/efi/efivars/BootCurrent-8be4df61-93ca-11d2-aa0d-00e098032b8c
/sys/firmware/efi/efivars/Boot0078-8be4df61-93ca-11d2-aa0d-00e098032b8c
```

这里的 `8be4df61-93ca-11d2-aa0d-00e098032b8c` 是 EFI global variable GUID。

当前启动项:

```txt
BootCurrent: 0078
BootOrder: 0078,003B,003E,003F,0064,0065,007D,007E,0042,0043,0068,0069,007F,0080,0046,0047,006C,006D,0081,0082,003A,0060,0061,007B,007C,007A,0000,0084
Boot0078* OpenEuler grubx64 HD(1,GPT,92ea950b-d2be-4c37-ab85-dea7eecf1dc3,0x800,0x400000)/File(\EFI\OPENEULER\GRUBX64.EFI)
```

磁盘对应关系:

```txt
/dev/sde1 vfat PARTUUID=92ea950b-d2be-4c37-ab85-dea7eecf1dc3 mounted on /boot/efi
/dev/sde2 ext4 mounted on /boot
/dev/sde3 ext4 mounted on /
```

也就是说 `efibootmgr -c -d /dev/sde -p 1 -l '\EFI\openEuler\grubx64.efi'`
里的 `-d/-p/-l` 用来构造 BootXXXX 变量中的 EFI device path。它不会把
grub 写进 ESP；grub 文件本身必须已经在 ESP 里。

### create-only 小实验

执行:

```sh
efibootmgr -C -d /dev/sde -p 1 -L codex-efibootmgr-test -l '\EFI\openEuler\grubx64.efi'
```

结果创建了临时项:

```txt
Boot0001* codex-efibootmgr-test	HD(1,GPT,92ea950b-d2be-4c37-ab85-dea7eecf1dc3,0x800,0x400000)/File(\EFI\openEuler\grubx64.efi)
efivarfs file=/sys/firmware/efi/efivars/Boot0001-8be4df61-93ca-11d2-aa0d-00e098032b8c
```

变量文件开头:

```txt
00000000  07 00 00 00 01 00 00 00  68 00 63 00 6f 00 64 00  |........h.c.o.d.|
00000010  65 00 78 00 2d 00 65 00  66 00 69 00 62 00 6f 00  |e.x.-.e.f.i.b.o.|
00000020  6f 00 74 00 6d 00 67 00  72 00 2d 00 74 00 65 00  |o.t.m.g.r.-.t.e.|
00000030  73 00 74 00 00 00 04 01  2a 00 01 00 00 00 00 08  |s.t.....*.......|
```

解释:

1. 前 4 字节 `07 00 00 00` 是 efivarfs 文件格式里的 variable attributes:
   `NON_VOLATILE | BOOTSERVICE_ACCESS | RUNTIME_ACCESS`。
2. 后面开始是 UEFI `EFI_LOAD_OPTION`，包括 active flag、description
   `codex-efibootmgr-test` 的 UCS-2 编码、device path。
3. `-C/--create-only` 不把新建项加入 `BootOrder`，实验前后 `BootOrder` 没变。

实验后已删除:

```sh
efibootmgr -b 0001 -B
```

### 源码调用链

用户态工具
- https://github.com/rhboot/efibootmgr.git
- https://github.com/rhboot/efivar.git

以及内核的参与:

```txt
efibootmgr CLI
  -> 构造 EFI_LOAD_OPTION / BootOrder 数据
  -> libefivar: efi_set_variable() / efi_del_variable()
  -> efivarfs: open/write/unlink /sys/firmware/efi/efivars/BootXXXX-<EFI_GLOBAL_GUID>
  -> kernel EFI runtime services
  -> firmware NVRAM
```

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
