# qemu 错误处理
<!-- 9b40dc74-ab8b-47fa-b21e-5f1f68627c97 -->

## 两个错误 api 区别是什么?
- error_setg
- error_report
- error_report_err

- devel/style.rst
- devel/qapi-code-gen.rst

1. 不要用 printf ，而是去使用 error_report 和 warn_report ，qemu 需要控制 stdout 的正确使用。

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

## 内部实现

一共两个文件
#include "qapi/error.h"
#include "qemu/error-report.h"

```c
/*
 * Propagate the Error* object to migration core.  The caller mustn't
 * reference the error pointer after the function returned, because the
 * Error* object might be freed.
 */
void migrate_error_propagate(MigrationState *s, Error *error)
{
    QEMU_LOCK_GUARD(&s->error_mutex);
    trace_migrate_error(error_get_pretty(error));
    error_propagate(&s->error, error);
}
```


## migration 如何注入错误?

- migration_cancel
- migration_file_set_error

## 错误处理结果

QEMU 的错误处理核心不是 C++ 异常，而是“返回值 + Error **errp + 分层上报”。当前源码版本是 v11.1.0-454-g0d4709b8349a。

可以把它理解成下面这条路径：

```txt
底层发现错误
    │  error_setg()
    ▼
Error 对象沿 Error **errp 向上传递
    │  可补充上下文 error_prepend()
    ▼
边界层决定怎么处理
    ├─ CLI/HMP：打印错误
    ├─ QMP：转换成 JSON error 响应
    ├─ fatal：打印并 exit(1)
    └─ 编程错误：abort()
```

主要机制如下：

1. Error **errp 负责传递详细错误

函数一般同时通过返回值表示成败，通过 errp 携带可读的错误信息：

```txt
bool open_backend(const char *path, Error **errp)
{
    if (invalid(path)) {
        error_setg(errp, "invalid backend path '%s'", path);
        return false;
    }

    return true;
}
```

QEMU 推荐的返回值约定是：

- bool：true 成功，false 失败
- 指针：非 NULL 成功，NULL 失败
- 整数：非负成功，负数失败

参见 include/qapi/error.h:20。

2. 调用者决定错误策略

调用函数时，errp 可以有四种典型形式：

```txt
Error *err = NULL;
foo(&err);              /* 接收错误，之后自行处理 */

foo(NULL);              /* 忽略错误 */

foo(&error_fatal);      /* 出错时打印并 exit(1) */

foo(&error_abort);      /* 出错时打印位置并 abort() */
```

实现位于 util/error.c:23。

这里体现了 QEMU 的重要原则：底层负责发现错误并清理现场，上层负责决定恢复、报告还是退出。

3. 错误可以逐层补充上下文

推荐直接把 errp 传下去；需要增加上下文时使用：

if (!open_image(filename, errp)) {
    error_prepend(errp, "failed to initialize disk: ");
    return false;
}

较老代码常见：

Error *local_err = NULL;

foo(&local_err);
if (local_err) {
    error_propagate(errp, local_err);
    return false;
}

新代码倾向使用 ERRP_GUARD()，这样可以安全检查 *errp，参见 include/qapi/error.h:515。

4. 到接口边界后转换成不同形式

普通 CLI/HMP 路径用：

error_report_err(err);  /* 打印并释放 Error */
warn_report_err(err);

直接日志还包括：

error_report(...);
warn_report(...);
info_report(...);

它们输出到当前 HMP monitor，否则输出到 stderr，参见 util/error-report.c:317。

QMP 命令则把 Error 转成结构化 JSON：

{
  "error": {
    "class": "GenericError",
    "desc": "具体错误信息"
  }
}

转换代码在 qapi/qmp-dispatch.c:95，QMP 调度器集中接收命令处理函数返回的错误并生成响应。

5. Error 和日志不是一回事

Error 用来控制程序流程和向调用者报告失败；日志主要用于诊断，不应代替错误传播。

例如设备模型经常使用：

qemu_log_mask(LOG_GUEST_ERROR, ...);
qemu_log_mask(LOG_UNIMP, ...);

用于记录 guest 的非法行为或尚未实现的功能，通常不意味着 QEMU 自身必须退出。日志类别定义在 include/qemu/log.h:17。

6. 子系统仍可能使用传统错误码

性能敏感或接口历史较久的代码，特别是 block/AIO 层，大量使用负 errno：

ret = operation();

if (ret < 0) {
    error_setg_errno(errp, -ret, "operation failed");
    return false;
}

也就是说，底层用 -EIO、-EINVAL 等传递机器可处理的状态，到上层边界再转换成 Error。

一句话总结：QEMU 用返回值快速判断成功或失败，用 Error 保存可读上下文，逐层传递到 CLI、HMP 或 QMP 边界；只有真正不可恢复的启动错误或
内部逻辑错误才使用 exit()、abort() 或断言。


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
