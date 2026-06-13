## qemu 中 yank 的含义
<!-- 3537feea-fb32-499c-8fe9-3630c55e0a95 -->

yank = 用力拉、猛地拽走、迅速取走
这也是 Vim 中使用该词的原因：
不是“优雅复制”
而是“快速把一段内容拽走放到寄存器里”

也就是我一直被 vim 误导，导致以为 yank 的含义是复制。

在 qemu 的语境中，他的意思是强制断开。

关联的函数:
- `migration/yank_functions.c`
- `util/yank.c`

- `yank_register_instance`
    - 调用位置只有三个，`yank_register_instance`
    - 正好是那几种类型的
        - `YANK_INSTANCE_TYPE_CHARDEV`
        - `YANK_INSTANCE_TYPE_MIGRATION`
        - `YANK_INSTANCE_TYPE_BLOCK_NODE`

- `yank_register_function`
    - 注册的 yank function 只有 socket io 和 nbd 使用，而且都是 shutdown 的作用


从 https://www.mail-archive.com/qemu-devel@nongnu.org/msg772357.html 中找到:

```txt
The yank feature allows to recover from hanging qemu by "yanking"
at various parts. Other qemu systems can register themselves and
multiple yank functions. Then all yank functions for selected
instances can be called by the 'yank' out-of-band qmp command.
Available instances can be queried by a 'query-yank' oob command.
```

yank.json 中有
```json
##
# @YankInstance:
#
# A yank instance can be yanked with the @yank qmp command to recover from a
# hanging QEMU.
#
# Currently implemented yank instances:
#  - nbd block device:
#    Yanking it will shut down the connection to the nbd server without
#    attempting to reconnect.
#  - socket chardev:
#    Yanking it will shut down the connected socket.
#  - migration:
#    Yanking it will shut down all migration connections. Unlike
#    @migrate_cancel, it will not notify the migration process, so migration
#    will go into @failed state, instead of @cancelled state. @yank should be
#    used to recover from hangs.
#
# Since: 6.0
##
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

在 QEMU 里，yank 可以理解成“强制拔线”或“硬断开”。

它是一个 QMP 恢复机制，不是普通的正常停止流程。用途是当 QEMU 因某个外部连接卡住时，直接把这个连接关掉，让 QEMU 从 hang 里恢复。QAPI 的原文定义就在
qapi/yank.json:49 和 qapi/yank.json:76。

当前支持的对象有：

- block-node
- chardev
- migration

语义分别是：

- NBD block device：直接断开到 NBD server 的连接，不尝试重连，见 qapi/yank.json:56
- socket chardev：直接 shutdown socket，见 chardev/char-socket.c:908
- migration：直接 shutdown migration channel；和 migrate_cancel 不同，它不是“通知迁移正常取消”，而是把连接硬切掉，所以迁移通常进 failed，不是
  cancelled，见 qapi/yank.json:61 和 migration/yank_functions.c:16

如果从实现上看，yank 本质是：

- 先注册一个 YankInstance
- 再给它挂一个 YankFn
- 触发时调用这个函数去关闭底层资源

接口定义在 include/qemu/yank.h:17。

一句话总结：yank 不是优雅关闭，而是为了从卡死 I/O 中恢复的“强制断连”。如果你愿意，我可以继续给你画一下它和 migrate_cancel、block-job-cancel 这类“正常取
消”接口的区别。


## 和 migration 的原因

```c
void migration_ioc_register_yank(QIOChannel *ioc)
{
    if (migration_ioc_yank_supported(ioc)) {
        yank_register_function(MIGRATION_YANK_INSTANCE,
                               migration_yank_iochannel,
                               QIO_CHANNEL(ioc));
    }
}
```

- `migration_ioc_register_yank`
    - `yank_register_function`
    - `migration_yank_iochannel`
        - `qio_channel_shutdown`
            - 调用 hook



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
