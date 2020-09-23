---
title: Bus
date: 2018-04-19 18:09:37
tags: cpu
---

# 硬件层面
Main types of devices found on the PCI bus
1. Network cards (wired or wireless)
1. SCSI adapters
1. Bus controllers: USB, PCMCIA, I2C, FireWire, IDE
1. Graphics and video cards
1. Sound cards

# 软件层面

lspci

xx:yy.zz 设备名:制造公司
xx --> PCI bus number
yy --> PCI device number
zz --> Function number  

`lscpi t` shows the bus device tree


`lspci -x` show device configuration
Each PCI device has a 256 byte address space containing configuration registers.



# links

[bus introduction](http://www.cut.ac.zw/espace/mchinyuku/1410878742.pdf)

