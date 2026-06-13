## 虚拟机中到底是否需要 firmware
- https://bugzilla.redhat.com/show_bug.cgi?id=1386202

特殊的情况下需要:

> Some VF devices may well need firmware support, and I could easily
imagine that firmware would be needed for many usb devices that are assigned via
usb passthrough.


## 需要理解的 firmware 的三个问题
- UEFI
- 设备中的 firmware
- 虚拟化环境中的 firmware，QEMU 如何模拟内容给 Guest，在 Guest 中观测

## TODO
- [UEFI 引导与 BIOS 引导在原理上有什么区别？](https://www.zhihu.com/question/21672895/answer/774538058)

## linux-firmware 这个包包含什么?

https://github.com/NixOS/nixpkgs/blob/nixos-23.05/pkgs/os-specific/linux/firmware/linux-firmware/default.nix#L28

可以找到:
https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git

下载之后，发现几乎全都都是二进制。
