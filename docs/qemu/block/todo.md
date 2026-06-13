## block

也许了解一下这个经典设备可以帮助我来理解 aio 之类的蛇皮到底在干什么

- [ ] 将 glib.md 中的测试分析首先放一下吧, 先分析 block/null.c

- [ ] QEMU 的 blocking layer 如此复杂
- [ ] 为什么 QEMU 中的 block 感觉比网络复杂很多的。

## qemu 的 ./job.c 的做什么的
<!-- 89063995-2c82-4418-bb6c-b62df9f8f066 -->
✦ 好的，我来总结一下QEMU的作业系统（job system）。

  QEMU作业系统概述

  QEMU的作业系统是一个用于管理长时间运行操作的复杂框架。以下是其核心组件和功能：

  核心组件

   1. Job结构体：主要的数据结构，包含：
      - 作业ID和驱动程序
      - 执行协程
      - 状态管理（状态、暂停、取消标志）
      - 进度跟踪
      - AIO上下文管理
      - 事务成员关系

   2. Job驱动程序：定义不同作业生命周期阶段的回调函数：
      - run：主执行协程
      - pause/resume：控制暂停行为
      - prepare：预最终化准备
      - commit/abort/clean：最终化回调
      - cancel：自定义取消逻辑
      - free：清理回调

   3. 作业状态：作业在各种状态之间转换：
      - UNDEFINED → CREATED → RUNNING → READY/WAITING → PENDING → CONCLUDED → NULL
      - 还包括PAUSED、STANDBY、ABORTING等状态

   4. 作业事务：允许将作业分组，使它们一起完成或失败

  主要特性

   - 线程安全性：通过job_mutex保护，具有特定的锁定模式
   - 进度跟踪：内置进度表用于监控作业进展
   - 速率限制：支持对作业操作进行节流
   - 暂停/恢复：对作业执行的细粒度控制
   - 取消：软取消和硬取消模式
   - 最终化：自动或手动最终化工作流程

  实现示例

  备份作业实现展示了如何：
   - 定义具有特定回调的自定义作业驱动程序
   - 处理错误和重试
   - 集成块复制操作
   - 管理脏位图和同步

  该系统设计用于处理各种类型的长时间运行操作，如备份、镜像、流式传输、提交和其他块操作，同时为监控、控制和错误处理提供一致的接口。

  关键文件

   - job.c：作业系统的主实现
   - job.h：作业相关的数据结构和函数声明
   - job-qmp.c：作业的QMP（QEMU机器协议）接口
   - blockjob.c：块作业的实现
   - blockjob.h：块作业的公共接口

  这个系统是QEMU虚拟机中管理后台任务的核心机制，确保了长时间运行的磁盘操作能够被正确地监控、控制和管理。

## 这个目录就是给 qsd 使用的吧
block/export/

这个东西测试下:
```txt
static SaveVMHandlers savevm_dirty_bitmap_handlers = {
    .save_setup = dirty_bitmap_save_setup,
    .save_live_complete_postcopy = dirty_bitmap_save_complete,
    .save_live_complete_precopy = dirty_bitmap_save_complete,
    .has_postcopy = dirty_bitmap_has_postcopy,
    .state_pending_exact = dirty_bitmap_state_pending,
    .state_pending_estimate = dirty_bitmap_state_pending,
    .save_live_iterate = dirty_bitmap_save_iterate,
    .is_active_iterate = dirty_bitmap_is_active_iterate,
    .load_state = dirty_bitmap_load,
    .save_cleanup = dirty_bitmap_save_cleanup,
    .is_active = dirty_bitmap_is_active,
};
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
