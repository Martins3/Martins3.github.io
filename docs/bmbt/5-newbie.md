# BMBT newbie 必读

## 技能清单
- QEMU
  - 二进制翻译器基本原理
  - memory model
  - PCI 模拟
- Linux Kernel
  - irq domain
  - memory
    - page fault 的过程
    - 进程地址空间
- LoongArch
  - TLB
- [BTMMU](https://liuty10.github.io/TianyiLiu_files/download/btmmu.pdf)
- HSPT
- ESPT
- Dune
- Linux Programming Interface
- 深入理解计算机体系结构
- [gdb bash makefile and ...](https://missing-semester-cn.github.io/)
- busybox

## 调试细节
- [ ] guest ip 来反汇编

## 裸机环境搭建

### 使用 U 盘

### 使用网络
[gnu grub doc](https://www.gnu.org/software/grub/manual/grub/html_node/Network.html)

> 似乎是一个深渊

## minicom 的使用注意点
如果在 grub 中在 Linux Kernel 的启动项中添加上 `console=ttyS0,115200`，
那么可以就可以使用串口来获取这个 Kernel 的 dmesg 输出了，调试 3A5000 的时候发现
无法输出 "密码" 两个中文字，使用下面的参数就可以了:
```sh
minicom -D /dev/ttyUSB0 -R UTF-8
```

minicom 无法发送字符，解决方法参考 [^2]，因为 uart 不需要中断的时候同样可以
正常工作（采用 poll ） 模式，所以如果 Guest 的 shell 不能交互，那么应该是 minicom 的配置有问题。

## 还可以继续开发的事情

- [ ] 从论文中抄过来

[^1]: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=872051
[^2]: https://stackoverflow.com/questions/3913246/cannot-send-character-with-minicom
