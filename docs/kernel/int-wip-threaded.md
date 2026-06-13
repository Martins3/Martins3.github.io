# 简单分析下 中断线程化

## request irq
devm_request_threaded_irq ==> request_threaded_irq

![loading](https://img2020.cnblogs.com/blog/1771657/202006/1771657-20200605223042609-247616444.png)
- [ ] 上面讲解了 thread ，shared 的处理，问题是，参数 irq 必须找到对应的 irq_desc, 新分配的只是 irq_action
    - [ ] 通过 irq_domain_alloc_descs 可以分配 irq_desc, 但是现在找不到这些函数的调用位置

https://lwn.net/Articles/302043/

顺便问下 ，一个 open ，为什么最后会触发这个路径:
```txt
#0  irq_setup_forced_threading (new=0xffff888008c75600) at kernel/irq/manage.c:1366
#1  __setup_irq (irq=irq@entry=4, desc=desc@entry=0xffff888004175000, new=new@entry=0xffff888008c75600) at kernel/irq/manage.c:1544
#2  0xffffffff811d2bcd in request_threaded_irq (irq=4, handler=<optimized out>, handler@entry=0xffffffff81913e40 <serial8250_interrupt>,
    thread_fn=thread_fn@entry=0x0 <fixed_percpu_data>, irqflags=0, devname=0xffff888005576d68 "ttyS0", dev_id=dev_id@entry=0xffff88800f769640) at kernel/irq/manage.c:2204
#3  0xffffffff81914f92 in request_irq (dev=<optimized out>, name=<optimized out>, flags=<optimized out>, handler=0xffffffff81913e40 <serial8250_interrupt>,
    irq=<optimized out>) at ./include/linux/interrupt.h:168
#4  serial_link_irq_chain (up=0xffffffff83a2df80 <serial8250_ports>) at drivers/tty/serial/8250/8250_core.c:210
#5  univ8250_setup_irq (up=0xffffffff83a2df80 <serial8250_ports>) at drivers/tty/serial/8250/8250_core.c:332
#6  0xffffffff81919a95 in serial8250_do_startup (port=0xffffffff83a2df80 <serial8250_ports>) at drivers/tty/serial/8250/8250_port.c:2331
#7  0xffffffff81911b93 in uart_port_startup (tty=tty@entry=0xffff88800f746400, state=state@entry=0xffff888005744000, init_hw=init_hw@entry=false)
    at drivers/tty/serial/serial_core.c:265
#8  0xffffffff81912c07 in uart_startup (init_hw=false, state=0xffff888005744000, tty=0xffff88800f746400) at drivers/tty/serial/serial_core.c:308
#9  uart_startup (init_hw=false, state=0xffff888005744000, tty=0xffff88800f746400) at drivers/tty/serial/serial_core.c:299
#10 uart_port_activate (port=0xffff888005744000, tty=0xffff88800f746400) at drivers/tty/serial/serial_core.c:1942
#11 0xffffffff818f8742 in tty_port_open (port=0xffff888005744000, tty=0xffff88800f746400, filp=0xffff88800b80b800) at drivers/tty/tty_port.c:784
#12 0xffffffff8190ee9e in uart_open (tty=<optimized out>, filp=<optimized out>) at drivers/tty/serial/serial_core.c:1922
#13 0xffffffff818efccf in tty_open (inode=0xffff8880093ea130, filp=0xffff88800b80b800) at drivers/tty/tty_io.c:2145
#14 0xffffffff8143d726 in chrdev_open (inode=0xffff8880093ea130, filp=0xffff88800b80b800) at fs/char_dev.c:414
#15 0xffffffff81431885 in do_dentry_open (f=f@entry=0xffff88800b80b800, inode=0xffff8880093ea130, open=<optimized out>, open@entry=0x0 <fixed_percpu_data>) at fs/open.c:920
#16 0xffffffff81433822 in vfs_open (path=path@entry=0xffffc90000017db0, file=file@entry=0xffff88800b80b800) at fs/open.c:1051
#17 0xffffffff8144c95c in do_open (op=0xffffc90000017db0, file=0xffff88800b80b800, nd=0xffffc90000017db0) at fs/namei.c:3636
#18 path_openat (nd=nd@entry=0xffffc90000017db0, op=op@entry=0xffffc90000017edc, flags=flags@entry=65) at fs/namei.c:3791
#19 0xffffffff8144d823 in do_filp_open (dfd=dfd@entry=-100, pathname=pathname@entry=0xffff888008c29000, op=op@entry=0xffffc90000017edc) at fs/namei.c:3818
#20 0xffffffff81433b3f in do_sys_openat2 (dfd=-100, filename=<optimized out>, how=how@entry=0xffffc90000017f18) at fs/open.c:1356
#21 0xffffffff8143413e in do_sys_open (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1372
#22 __do_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1388
#23 __se_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1383
#24 __x64_sys_openat (regs=<optimized out>) at fs/open.c:1383
#25 0xffffffff820cd04e in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000017f58) at arch/x86/entry/common.c:50
#26 do_syscall_64 (regs=0xffffc90000017f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#27 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

启动虚拟机，似乎没有人用。
