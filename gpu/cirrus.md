## cirrus 入手也是不错的

原来背后是一家公司:
https://cn.cirrus.com/

code/qemu/alpine.sh
2065:           # gpu=" -device cirrus-vga "

## 原来 cirrus 中的代码量这么小啊

CONFIG_FB_CIRRUS
CONFIG_DRM_CIRRUS_QEMU

docs/qemu/qom/options.md 中提到的这两个设备，他们是什么关系:
```txt
name "cirrus-vga", bus PCI, desc "Cirrus CLGD 54xx VGA"
name "isa-cirrus-vga", bus ISA
```

```txt
🧀  sudo lspci -s 00:07.0 -vv
00:07.0 VGA compatible controller: Cirrus Logic GD 5446 (prog-if 00 [VGA controller])
        Subsystem: Red Hat, Inc. QEMU Virtual Machine
        Control: I/O+ Mem+ BusMaster- SpecCycle- MemWINV- VGASnoop- ParErr- Stepping- SERR+ FastB2B- DisINTx-
        Status: Cap- 66MHz- UDF- FastB2B- ParErr- DEVSEL=fast >TAbort- <TAbort- <MAbort- >SERR- <PERR- INTx-
        Region 0: Memory at fa000000 (32-bit, prefetchable) [size=32M]
        Region 1: Memory at fe0d5000 (32-bit, non-prefetchable) [size=4K]
        Expansion ROM at 000c0000 [disabled] [size=128K]
```

似乎的确是不需要驱动的

从 qemu 的
```txt
666:    0000000040000000-0000000041ffffff (prio 1, i/o): cirrus-pci-bar0
668:      0000000040000000-00000000403fffff (prio 0, i/o): cirrus-linear-io
669:      0000000041000000-00000000413fffff (prio 0, i/o): cirrus-bitblt-mmio
681:    0000000042088000-0000000042088fff (prio 1, i/o): cirrus-mmio
```

