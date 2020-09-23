# Linux Programming Interface: Chapter 64 : pseudoterminal
A pseudoterminal is a virtual device that provides an IPC channel. On one end of the
channel is a program that expects to be connected to a terminal device. On the
other end is a program that drives the terminal-oriented program by using the
channel to send it input and read its output.

This chapter describes the use of pseudoterminals, showing how they are
employed in applications such as terminal emulators, the script(1) program, and
programs such as ssh.

## 64.1 Overview
A pseudoterminal provides the missing link for creating a network connection to a
terminal-oriented program.

A pseudoterminal is a pair of connected virtual devices:
a `pseudoterminal master` and a `pseudoterminal slave`, sometimes jointly referred to as a
`pseudoterminal pair`. 

The key point about a pseudoterminal is that the slave device appears just like a
standard terminal. All of the operations that can be applied to a terminal device
can also be applied to a pseudoterminal slave device.

An application that uses a pseudoterminal typically does so as follows:
> 对于为何设计成为slave 和 master 的模式，也不是很理解.

1. The driver program opens the pseudoterminal master device.
2. The driver program calls fork() to create a child process. The child performs
the following steps:
    * Call setsid() to start a new session, of which the child is the session leader. This step also causes the child to lose its controlling terminal.
    * Open the pseudoterminal slave device that corresponds to the master
  device. Since the child process is a session leader, and it doesn’t have a controlling terminal, the pseudoterminal slave becomes the controlling terminal
  for the child process.
    * Use `dup()` (or similar) to duplicate the file descriptor for the slave device on
  standard input, output, and error.
    * Call `exec()` to start the terminal-oriented program that is to be connected to the pseudoterminal slave
> 上面的东西东西暴露出来对于 进程　基础知识理解很有问题啊!

At this point, the two programs can now communicate via the pseudoterminal.
Anything that the driver program writes to the master appears as input to the terminaloriented program on the slave, and anything that the terminal-oriented program
writes to the slave can be read by the driver program on the master. 

## 64.2 UNIX 98 Pseudoterminals
> 后面的没看，感觉和设备驱动关系不大。

