## HMI
这里描述在 graphic 和 non-graphic 的模式下访问 HMI 的方法，并且说明了从 HMI 中间如何获取各种信息
https://web.archive.org/web/20180104171638/http://nairobi-embedded.org/qemu_monitor_console.html

自己尝试的效果:
```c
(qemu) info chardev
virtiocon0: filename=pty:/dev/pts/6
serial1: filename=pipe
parallel0: filename=vc
gdb: filename=disconnected:tcp:0.0.0.0:1234,server
compat_monitor0: filename=stdio
serial0: filename=pipe
```
如果什么都不配置，结果如下:
```c
(qemu) info chardev
parallel0: filename=vc
compat_monitor0: filename=stdio
serial0: filename=vc
```
## 源码分析
以 hmp_info_mtree 为例:
