## 首先按照这个操作
- https://hackmd.io/@sysprog/user-mode-linux-env

```sh
make menuconfig ARCH=um
make ARCH=um
```

# user mode linux

- [ ] 官方文档 : https://www.kernel.org/doc/html/latest/virt/uml/user_mode_linux.html
- [ ] https://stackoverflow.com/questions/32303095/how-does-the-user-mode-kernel-in-uml-interface-with-the-underlying-kernel-on-the


## 首先让我猜测一下是如何工作的

- 对于设备存在的模拟
- 内存的模拟。
  - 模拟出来多个地址空间出来。
- CPU 的模拟
  - 使用线程进行模拟的吧!
  - context swtich 也是特殊的才对。
