# Linux Programming Interface: Chapter 62: Terminals

All of the above is a preamble to saying that the need to program terminal devices is less frequent than it used to be.

## 62.1 Overview
Both a conventional terminal and a terminal emulator have an associated terminal
driver that handles input and output on the device.
 (In the case of a terminal emulator, the device is a pseudoterminal.)
> driver 的作用 handle input and output

When performing input, the driver is capable of operating in either of the following modes:
1. Canonical mode
2. Noncanonical mode
> 如何设置这两种模式 ?

The terminal driver also interprets a range of special characters, such as the
interrupt character (normally Control-C) and the end-of-file character (normally
Control-D).
> 原来是driver的工作，我想找到对应的代码的位置

A terminal driver operates two queues (Figure 62-1): one for input characters
transmitted from the terminal device to the reading process(es) and the other for
output characters transmitted from processes to the terminal
> terminal device ---> the process ---> terminal
> 图62-1 不错的

## 62.2 Retrieving and Modifying Terminal Attributes
The tcgetattr() and tcsetattr() functions retrieve and modify the attributes of a terminal

Setting the line discipline can be relevant when programming serial lines.g

## 62.3 The stty Command
> 讲解使用stty 命令　可以替代tcgetattr 和 tcsetattr　的作用
> some part is skiped


## 62.4 Terminal Special Characters
> 一个列表
> 举的例子还是很有意思

## 62.5 Terminal Flags
The following paragraphs provide more details about some of the `termios` flags.
> 继续描述termios的几个flags ，flags 中间包含的很多变量已经没有作用了

## 62.6 Terminal I/O Modes
1. describe these two modes in detail.
2. describe three useful terminal modes—cooked, cbreak, and raw
3. show how these modes are *emulated* on modern UNIX systems by setting appropriate values in the termios structure.

#### 62.6.1 Canonical Mode

#### 62.6.2 Noncanonical Mode
> 无论是 MIN 还是 TIME 的作用是限制read 函数何时返回，本文介绍了四种取值的含义和适用的场景

#### 62.6.3 Cooked, Cbreak, and Raw Modes
> 通过配置 termios 参数形成三种常用的模式
> 例子似乎非常的有意思

## 62.7 Terminal Line Speed (Bit Rate)
The `cfgetispeed()` and `cfsetispeed()` functions retrieve and
modify the input line speed. The `cfgetospeed()` and `cfsetospeed()` functions retrieve and modify the output line speed.

## 62.8 Terminal Line Control
The `tcsendbreak()`, `tcdrain()`, `tcflush()`, and `tcflow()` functions perform tasks that are
usually collectively grouped under the term line control. (These functions are POSIX
inventions designed to replace various ioctl() operations.)
> 对于driver 两个queue 的控制

## 62.9 Terminal Window Size
运行于终端中间的程序可以通过ioctl 捕获signal 获取当前的中断的大小
> skip the example

## 62.10 Terminal Identification
The isatty() function enables us to determine whether a file descriptor, fd, is
associated with a terminal (as opposed to some other file type).

The isatty() function is useful in editors and other screen-handling programs that
need to determine whether their standard input and output are directed to a terminal.

## 62.11 Summary
> to read !
> 总体来说，都是操纵termios的内容而已，并且逐个讲解其中的作用是什么。
