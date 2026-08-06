- The notes based on [**fpga4fun**](https://www.fpga4fun.com/FPGAinfo1.html)
- https://github.com/LeiWang1999/FPGA : 各种资料，主要是中文的

- [fpga 专用语言](https://github.com/sylefeb/Silice)

https://learn.sparkfun.com/tutorials/how-does-an-fpga-work/all

https://github.com/fischermoseley/manta/

iverilog

https://news.ycombinator.com/item?id=39678374

## hdlbits

https://hdlbits.01xz.net/wiki/Main_Page

重新理解下时序逻辑是什么吧!

## 仿真

1. 随机约束 和 UVM 是什么?

## 教科书

model sim

vcs

https://www.reddit.com/r/VHDL/comments/t2r239/questasim_vs_modelsim/

EDAplayground

vitis petalinux

- https://peridot-echidna-851.notion.site/ZUBoard-MPSoC-c8f01f8379a549c4abadb22965db1db1

  - https://www.bilibili.com/video/BV1uH4y1K7sp : 的确是保姆级的

- https://www.bilibili.com/video/BV11y4y1i7Lv

就这个就很好玩了:

- https://github.com/Xilinx
  - https://pynq.readthedocs.io/en/latest/getting_started.html

## 有趣的尝试

1. qemu 和 fpga 中支持
2. kernel 中的 fpga 的出现的问题

## 细品

```txt
🧀  fd xilinx
Documentation/ABI/testing/sysfs-driver-xilinx-tmr-manager
Documentation/driver-api/xilinx/
Documentation/misc-devices/xilinx_sdfec.rst
arch/microblaze/include/asm/xilinx_mb_manager.h
drivers/crypto/xilinx/
drivers/firmware/xilinx/
drivers/gpio/gpio-xilinx.c
drivers/input/serio/xilinx_ps2.c
drivers/irqchip/irq-xilinx-intc.c
drivers/misc/xilinx_sdfec.c
drivers/misc/xilinx_tmr_inject.c
drivers/misc/xilinx_tmr_manager.c
drivers/pmdomain/xilinx/
drivers/pwm/pwm-xilinx.c
drivers/soc/xilinx/
drivers/mtd/spi-nor/xilinx.c
drivers/spi/spi-xilinx.c
drivers/tty/serial/xilinx_uartps.c
drivers/usb/dwc3/dwc3-xilinx.c
drivers/usb/gadget/udc/udc-xilinx.c
drivers/usb/host/ehci-xilinx-of.c
drivers/video/fbdev/xilinxfb.c
drivers/watchdog/of_xilinx_wdt.c
drivers/watchdog/xilinx_wwdt.c
include/dt-bindings/media/xilinx-vip.h
include/linux/platform_data/xilinx-ll-temac.h
include/uapi/linux/xilinx-v4l2-controls.h
include/uapi/misc/xilinx_sdfec.h
sound/soc/xilinx/
```

### pmdomain

drivers/pmdomain/xilinx/zynqmp-pm-domains.c

不知道做什么的

### drivers/firmware/xilinx/

### dma

Documentation/devicetree/bindings/dma/xilinx/xilinx_dma.txt
drivers/dma/xilinx/
drivers/dma/xilinx/xilinx_dma.c
drivers/dma/xilinx/xilinx_dpdma.c

什么叫

### spi

### pcie

这几个文件似乎都是对称的:

- drivers/pci/controller/pcie-xilinx-common.h
- drivers/pci/controller/pcie-xilinx-cpm.c
- drivers/pci/controller/pcie-xilinx-dma-pl.c
- drivers/pci/controller/pcie-xilinx-nwl.c
- drivers/pci/controller/pcie-xilinx.c

### media

drivers/media/platform/xilinx/

就是视频而已

### fpga

drivers/fpga/xilinx-core.c
drivers/fpga/xilinx-core.h
drivers/fpga/xilinx-pr-decoupler.c
drivers/fpga/xilinx-selectmap.c
drivers/fpga/xilinx-spi.c

### icap

drivers/char/xilinx_hwicap/
drivers/char/xilinx_hwicap/xilinx_hwicap.c
drivers/char/xilinx_hwicap/xilinx_hwicap.h

### clk

drivers/clk/xilinx/

### iio

drivers/iio/adc/xilinx-ams.c
drivers/iio/adc/xilinx-xadc-core.c
drivers/iio/adc/xilinx-xadc-events.c
drivers/iio/adc/xilinx-xadc.h

### ethernet

三种网络内容:
drivers/net/ethernet/xilinx/

drivers/net/can/xilinx_can.c

drivers/net/phy/xilinx_gmii2rgmii.c

### phy

drivers/phy/xilinx/

## pynq 是什么?

https://pynq.readthedocs.io/en/latest/getting_started/pynq_z1_setup.html

似乎买一个 pynz z2 来

## 搞懂都是搞什么才可以

[ZYNQ UltraScale+ MPSoc FPGA 初学笔记](https://zhuanlan.zhihu.com/p/163300131)

- FPGA 产品就是我们以前比较熟悉的 Spartan、Artix、Kintex 和 Vertex 系列的产品，是纯逻辑产品
- SOC 被 xilinx 叫做 ZYNQ ， 比较常见的 ZYNQ-7000 系列
- 高端系列里面的 UltraScale+ MPSoc 有 EV 和 EG 两个系列

如果是 FPGA 产品， 买一个似乎 AX309 即可

- Spartan : 低端
- Artix : 中低端
- Kintex : 高端
- Virtex : 旗舰

## 这个很好的，很有精神
https://www.zhihu.com/question/381684248/answer/3210970751

https://github.com/enjoy-digital/litex

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
