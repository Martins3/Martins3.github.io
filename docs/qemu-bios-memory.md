# QEMU 中的 seabios : 地址空间

seabios 的基础知识可以参考李强的《QEMU/KVM 源码解析与应用》, 下面来分析一下和地址空间相关的几个小问题。

## pc.bios
seabios 的 src/fw/shadow.c 中存在下面

```c
// On the emulators, the bios at 0xf0000 is also at 0xffff0000
#define BIOS_SRC_OFFSET 0xfff00000
```

## PAM
- [ ]

## SMM
