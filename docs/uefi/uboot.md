# uboot
https://github.com/u-boot/u-boot

为什么路由器总是使用 uboot ?

https://docs.u-boot.org/en/latest/board/emulation/qemu-x86.html

```txt
make qemu_arm64_defconfig
```

当然，需要 python 的支持，很诡异的
```txt
python -m venv .venv
source .venv/bin/activate
```

qemu-system-x86_64 -m 8G -smp 4 -bios /tmp/b/qemu-x86_64/u-boot.rom \
  -drive file=root.img,if=virtio,driver=raw \
  -drive file=ubuntu-23.04-desktop-amd64.iso,if=virtio,driver=raw

## 相当有趣
doc/arch/x86/x86.rst

> U-Boot also supports booting directly from x86 reset vector, without coreboot.
> In this case, known as bare mode, from the fact that it runs on the
> 'bare metal', U-Boot acts like a BIOS replacement. The following platforms
> are supported:

## linuxboot
https://www.linuxboot.org/

https://stackoverflow.com/questions/53681838/how-does-linuxboot-differs-from-coreboot-in-the-firmware-phase : 仔细阅读

- [ ] 为什么感觉 linuxboot 就是 coreboot 了啊?

### https://lwn.net/Articles/748586/

> Most notably, the Intel Management Engine (ME) runs a complete Minix operating system, while System Management Mode (SMM) is used to run code for certain events (e.g. laptop lid gets closed) in a way that is completely invisible to the running OS.

完全合理，拯救者的 Fn + Q 在 windows 上和 Linux 上完全一样。

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
