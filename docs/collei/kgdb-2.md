# kgdb 为什么最好是用 ttyS0 来
virtserialport 不能直接用于标准 kgdboc；virtconsole 则有接入路径，但存在限制。 两者需要区分：

 QEMU 设备         Guest 中的接口                           KGDB
━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 virtserialport    /dev/vportXpY、/dev/virtio-ports/名称    不是 TTY，不能直接配置给 kgdboc
────────────────  ───────────────────────────────────────  ───────────────────────────────────────
 virtconsole       /dev/hvc0 等                             HVC 实现了 polling 接口，可以尝试接入
────────────────  ───────────────────────────────────────  ───────────────────────────────────────
 模拟 UART         /dev/ttyS0 等                            常规的 kgdboc 使用方式

kgdboc 需要 TTY 驱动提供 poll_get_char / poll_put_char，以便内核停在调试器中时仍能收发数据；不是有可读写的字符设备就能使用。内核文档
(https://docs.kernel.org/process/debugging/kgdb.html)

我检查了你当前源码：

- drivers/char/virtio_console.c:1088：普通 port 提供的是 file_operations，无法直接接入 kgdboc。
- drivers/tty/hvc/hvc_console.c:855：有 CONFIG_CONSOLE_POLL 下的 polling 回调，所以 virtconsole → hvc0 → kgdboc 这条路径存在。

不过，当前 HVC polling 实现直接访问 driver->ttys[0]->driver_data，依赖 TTY 已初始化；virtio 发送路径还涉及分配内存、持锁和等待 host 消费队列。因此，不
能保证换成 virtconsole、加上 kgdboc=hvc0 就能可靠调试早期启动或任意死锁现场，尤其断点落在相关锁持有期间时。

如果目的是调试 QEMU guest 内核，优先用模拟 UART 配合 KGDB；已有 virtserialport 要继续利用，就需要专门实现 KGDB 的 virtio 传输后端，单改参数不够。


## 是否可以共用
可以，KGDB 和系统 console 可以共用 ttyS0，这正是 kgdboc 支持的用途。 内核文档
(https://docs.kernel.org/process/debugging/kgdb.html#kernel-parameter-kgdboc)

内核启动参数这样配：

console=ttyS0,115200 kgdboc=ttyS0,115200 nokaslr

如果希望启动时停下来等 GDB，在后面加 kgdbwait，并将 CONFIG_KGDB、CONFIG_KGDB_SERIAL_CONSOLE 编译进内核。

共用时的操作流程是：

1. 正常运行时，用串口终端看日志、操作 console。
2. 通过 SysRq-G 进入调试器。
3. 断开串口终端，让宿主机上的 GDB 连接同一个串口后端。
4. 调试结束后，在 GDB 中 detach，再接回串口终端。

宿主机上的终端程序和 GDB 要轮流使用这个连接；想免去手动切换，需要能够分流 console 和 GDB 协议的代理。另外，普通 kgdboc 不支持直接用 GDB 的 Ctrl-C
打断运行中的内核，需要 SysRq-G。连接说明 (https://docs.kernel.org/process/debugging/kgdb.html#connecting-with-gdb-to-a-serial-port)

还有一个容易混淆的参数：这里不要加 kgdbcon。它用于把 printk 封装成 GDB 协议消息，不能与同一 TTY 上的系统 console 一起使用。
kgdbcon 限制 (https://docs.kernel.org/process/debugging/kgdb.html#kernel-parameter-kgdbcon)

## 特殊命令

p linux_banner

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
