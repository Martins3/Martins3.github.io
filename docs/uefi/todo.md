## 看看这个
https://github.com/LongSoft/UEFITool

解析 uefi 的 var 的东西

## 原来虚拟机使用 ovfm 启动，那么 grub 就是 /etc/grub2-efi.cfg 了

## 看看这个
https://mp.weixin.qq.com/s/AvW9-fuV-nesglTWKaIttw

## 就是这个东西
https://www.phoronix.com/news/OVMF-Debug-Log-Driver

## 最近写了不少，都可以看看
- https://www.kraxel.org/blog/2025/10/firmware-logging/
	- https://github.com/tianocore/tianocore.github.io/wiki/EDK-II-Debugging
- https://www.kraxel.org/blog/2022/05/edk2-virt-quickstart/


## nixos 中的 canTouchEfiVariables 到底是什么来头

https://nixos.wiki/wiki/Bootloader 中最后提到如何增加 efi

```sh
efibootmgr -c -d /dev/nvme0n1 -p 1 -L NixOS-boot -l '\EFI\NixOS-boot\grubx64.efi'
```

1. 注意，-p 1 来设置那个 partition 的。
2. 后面的那个路径需要将 boot 分区 mount 然后具体产看，还有一次是设置的 "\EFI\nixo\BOOTX64.efi"

这个说的是什么意思来着:

```nix
efiSysMountPoint = "/boot/efi"; # ← use the same mount point here.
```

我设置的是 /boot 似乎影响也不大啊!

不知道为什么 efibootmgr 在 home.cli 中无法安装。

删除一个:

```txt
sudo efibootmgr  -B -b 3 # 3 是参数
```

设置优先级
sudo efibootmgr -o 0,1,2


## js binding
https://codeberg.org/smnx/promethee

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
