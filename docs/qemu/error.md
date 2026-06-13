## 经典例子

如果直通了一个设备，然后尝试热迁移，在执行 qmp 命令的时候，
可以接受到这个错误:
```txt
(qemu) migrate -d file:/tmp/qemu_mig
Error: 0000:03:00.0: VFIO migration is not supported in kernel
```

add_blockers

注册错误的位置在:
```c
            error_setg(&err, "%s: VFIO migration is not supported in kernel",
                       vbasedev->name);
```

而最后打印错误的位置在:
```c
bool migration_is_blocked(Error **errp)
{
    GSList *blockers = migration_blockers[migrate_mode()];

    if (qemu_savevm_state_blocked(errp)) {
        return true;
    }

    if (blockers) {
        error_propagate(errp, error_copy(blockers->data));
        return true;
    }

    return false;
}
```


## 文档
devel/style.rst
devel/qapi-code-gen.rst

## report
不要用 printf ，而是去使用 error_report 和 warn_report ，qemu 需要控制 stdout 的正确
使用。
error_setg

## propagate

bdrv_snapshot_goto 中经典例子:

首先，定义一个错误，然后在 error_propagate 中检查错误类型并且处理:
```c
        Error *local_err = NULL;
        // ...
        open_ret = drv->bdrv_open(bs, options, bs->open_flags, &local_err);
        qobject_unref(options);
        if (open_ret < 0) {
            bdrv_unref(fallback_bs);
            bs->drv = NULL;
            /* A bdrv_snapshot_goto() error takes precedence */
            error_propagate(errp, local_err);
            return ret < 0 ? ret : open_ret;
        }
```

更加经典的例子是:

host_memory_backend_memory_complete
```c
        bc->alloc(backend, &local_err);
        if (local_err) {
            goto out;
        }
        // ...
out:
    error_propagate(errp, local_err);
```
bc::alloc 会会调用 file_backend_memory_alloc ，在 file_backend_memory_alloc 中

## 这个是做什么的?
ERRP_GUARD

这就是一个经典例子:
vhost_user_blk_device_realize

## 为什么调用 error_propagate ，还是直接返回

error_propagate -> error_handle ，调用之后，会将 errp 中内容替换为 errp

简单来说，如果有本地的 Error ，而且调用函数的时候传递的是本地的 Error，
那么调用 error_propagate ，将本地的 Error 赋值到 errp 中。

那么问题来了，为什么需要 error_propagate ?
```c
static void audio_validate_opts(Audiodev *dev, Error **errp)
{
    Error *err = NULL;

    audio_create_pdos(dev);

    audio_validate_per_direction_opts(audio_get_pdo_in(dev), &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }

    audio_validate_per_direction_opts(audio_get_pdo_out(dev), &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }

    if (!dev->has_timer_period) {
        dev->has_timer_period = true;
        dev->timer_period = 10000; /* 100Hz -> 10ms */
    }
}
```

## 一共两个文件
#include "qapi/error.h"
#include "qemu/error-report.h"

## 看看这个是如何使用的?
```c
void migrate_set_error(MigrationState *s, const Error *error)
{
    QEMU_LOCK_GUARD(&s->error_mutex);

    trace_migrate_error(error_get_pretty(error));

    if (!s->error) {
        s->error = error_copy(error);
    }
}
```

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
