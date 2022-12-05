### ./util/yank

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
