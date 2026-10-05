# usb serial
## 似乎 usb serila 完全可以的

https://github.com/qemu/qemu/blob/master/hw/usb/dev-serial.c

## ttyUSB0 应该可以在 QEMU 中测试


## 如何理解 usb serial 结构
  最常见的例子是用 USB 转串口线连接开发板：

  电脑                         USB 转串口芯片                 开发板
  应用程序
      │
   /dev/ttyUSB0
      │
   Linux USB 串口驱动
      │
   USB 控制器 ─── USB 数据包 ─── CH340 / CP2102 / FT232
                                       │
                                    UART TX/RX ─────────── UART

```txt
  应用：open / read / write / termios
                      │
                /dev/ttyUSB0
                      │
            TTY 子系统、线路规程
                      │
         USB serial core：usb-serial.c
                      │
        具体芯片驱动：ch341.c、cp210x.c 等
            ＋通用传输代码：generic.c
                      │
               USB core / URB
                      │
               USB 主机控制器驱动
                      │
                    硬件
```

```txt
  write(fd, "A", 1)
      → TTY 子系统
      → serial_write()
      → 具体驱动的 write 回调
      → 通常复用 usb_serial_generic_write()
      → usb_submit_urb()
      → USB 设备收到数据
      → 芯片通过 UART 发出字符 A
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
