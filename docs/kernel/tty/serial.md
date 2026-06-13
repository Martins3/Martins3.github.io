必须将 CONFIG_SERIAL_8250 配置为 yes ，
否则启动时候 console=ttyS0 没有效果，但是 console=tty0 不受影响。

也是由于 SERIAL_8250_CONSOLE 对于 `CONFIG_SERIAL_8250=y` 有依赖
```txt
config SERIAL_8250_CONSOLE
	bool "Console on 8250/16550 and compatible serial port"
	depends on SERIAL_8250=y
	select SERIAL_CORE_CONSOLE
	select SERIAL_EARLYCON
```

如果把 SERIAL_8250=m ，那么将会有如下的 module 消失了。
```txt
kernel/drivers/tty/serial/serial_base.ko
kernel/drivers/tty/serial/8250/8250.ko
kernel/drivers/tty/serial/8250/8250_base.ko
kernel/drivers/tty/serial/8250/8250_exar.ko
kernel/drivers/tty/serial/8250/8250_lpss.ko
kernel/drivers/tty/serial/8250/8250_mid.ko
kernel/drivers/tty/serial/8250/8250_pci.ko
kernel/drivers/tty/serial/8250/8250_pericom.ko

kernel/drivers/misc/eeprom/eeprom_93cx6.ko
kernel/lib/fonts/font.ko
kernel/lib/math/rational.ko
```

总体符合预期，就是两个路径。

## drivers/input/serio/i8042.c 这个是做什么的?

## 这个是做什么的?
drivers/usb/serial/bus.c

## 那么 serial over lan 是什么?

## 在 qemu 的 stdio 中 dmesg ，最后可以触发这个错误

irq4 ，如何分配的
```txt
[  419.999920] link port-storage as upper device of eth2
[  448.267378] serial8250: too much work for irq4
[  448.403917] serial8250: too much work for irq4
[  448.541998] serial8250: too much work for irq4
[  448.696771] serial8250: too much work for irq4
[  448.860427] serial8250: too much work for irq4
[  449.020484] serial8250: too much work for irq4
[  449.155814] serial8250: too much work for irq4
[  449.295326] serial8250: too much work for irq4
[  449.428446] serial8250: too much work for irq4
[  449.582178] serial8250: too much work for irq4
[  455.103230] serial8250_interrupt: 1 callbacks suppressed
[  455.103235] serial8250: too much work for irq4
[  455.259182] serial8250: too much work for irq4
[  455.398221] serial8250: too much work for irq4
[  455.538640] serial8250: too much work for irq4
[  455.693459] serial8250: too much work for irq4
[  455.846877] serial8250: too much work for irq4
[  456.000410] serial8250: too much work for irq4
[  456.153424] serial8250: too much work for irq4
[  456.306691] serial8250: too much work for irq4
[  456.457983] serial8250: too much work for irq4
```

想不到注册中断是后面进行的:
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_openat
        - __se_sys_openat
          - __do_sys_openat
            - do_sys_open
              - do_sys_openat2
                - do_filp_open
                  - path_openat
                    - do_open
                      - vfs_open
                        - do_dentry_open
                          - chrdev_open
                            - tty_open
                              - uart_open
                                - tty_port_open
                                  - uart_port_activate
                                    - uart_startup
                                      - uart_startup
                                        - uart_port_startup
                                          - serial8250_do_startup
                                            - univ8250_setup_irq
                                              - serial_link_irq_chain
                                                - request_irq
                                                  - request_threaded_irq
                                                    - __setup_irq
                                                      - irq_setup_forced_threading

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
