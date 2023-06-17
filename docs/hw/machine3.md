## 记录下 windows 真正可用的还需要的东西
1. 额外的一个显示器
2. 尝试用这个解决 windows 的剪切板问题
  1. https://github.com/quackduck/uniclip
3. windows 静态情况存在 20% 的 CPU ，不知道为什么?
4. 是不是传递参数需要将 nv 的声音部分也传递进去吗? 还是说这是自动的?

- [virtio-gpu and qemu graphics in 2021](https://www.kraxel.org/blog/2021/05/virtio-gpu-qemu-graphics-update/)
- [The three(ish) levels of QEMU VM graphics](https://czak.pl/2020/04/09/three-levels-of-qemu-graphics.html)

https://wiki.gentoo.org/wiki/GPU_passthrough_with_libvirt_qemu_kvm

## usb 直通
- https://unix.stackexchange.com/questions/452934/can-i-pass-through-a-usb-port-via-qemu-command-line

```txt
/:  Bus 02.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/9p, 20000M/x2
    |__ Port 8: Dev 2, If 0, Class=Hub, Driver=hub/4p, 5000M
/:  Bus 01.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/16p, 480M
    |__ Port 1: Dev 21, If 1, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 1: Dev 21, If 2, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 1: Dev 21, If 0, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 2: Dev 3, If 2, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 2: Dev 3, If 0, Class=Vendor Specific Class, Driver=, 12M
    |__ Port 7: Dev 15, If 2, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 7: Dev 15, If 0, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 7: Dev 15, If 3, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 7: Dev 15, If 1, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 9: Dev 4, If 0, Class=Hub, Driver=hub/4p, 480M
        |__ Port 4: Dev 20, If 0, Class=Human Interface Device, Driver=usbhid, 12M
        |__ Port 4: Dev 20, If 1, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 11: Dev 5, If 0, Class=Human Interface Device, Driver=usbhid, 1.5M
    |__ Port 14: Dev 7, If 0, Class=Wireless, Driver=btusb, 12M
    |__ Port 14: Dev 7, If 1, Class=Wireless, Driver=btusb, 12M
```

```txt
Bus 002 Device 002: ID 174c:3074 ASMedia Technology Inc. ASM1074 SuperSpeed hub
Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
Bus 001 Device 020: ID 0c45:7638 Microdia AKKO 3084BT
Bus 001 Device 004: ID 174c:2074 ASMedia Technology Inc. ASM1074 High-Speed hub
Bus 001 Device 015: ID 2717:003b Xiaomi Inc. MI Wireless Mouse
Bus 001 Device 003: ID 0b05:19af ASUSTek Computer, Inc. AURA LED Controller
Bus 001 Device 007: ID 8087:0026 Intel Corp. AX201 Bluetooth
Bus 001 Device 005: ID 17ef:6019 Lenovo M-U0025-O Mouse
Bus 001 Device 021: ID 2f68:0082 Hoksi Technology DURGOD Taurus K320
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
```
