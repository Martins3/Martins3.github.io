# pm_qos

PM QoS（Power Management Quality of Service）用于提交电源管理约束，例如允许的最大延迟、频率上下限。框架汇总请求，由 cpuidle、CPUFreq、设备驱动或电源域代码落实限制，从而间接影响功耗；它不直接设置功耗，也不保证业务的端到端响应时间。

源码入口：

- `kernel/power/qos.c`：通用请求聚合、CPU latency QoS、frequency QoS。
- `drivers/base/power/qos.c`：per-device PM QoS。
- `include/linux/pm_qos.h`：数据结构与接口定义。
- [内核文档：PM Quality Of Service Interface](https://www.kernel.org/doc/html/latest/power/pm_qos_interface.html)

## 接口分类

要区分“约束作用于谁”和“从内核态还是用户态提交请求”。CPU QoS 和 per-device QoS 都可以提供内核态、用户态入口。

| 作用对象      | 入口                                          | 用途                                                |
| ------------- | --------------------------------------------- | --------------------------------------------------- |
| CPU，全局约束 | `cpu_latency_qos_add/update/remove_request()` | 内核中的驱动或子系统管理 CPU latency 请求           |
| CPU，全局约束 | `/dev/cpu_dma_latency`                        | 用户态管理同一套 CPU latency 请求                   |
| CPU，全局约束 | `/dev/cpu_wakeup_latency`                     | 用户态管理单独的 CPU system wakeup latency 请求集合 |
| 指定设备      | `dev_pm_qos_add/update/remove_request()`      | 内核管理该设备的指定类型约束                        |
| 指定设备      | 设备的 `power/pm_qos_*` sysfs 属性            | 用户态管理已暴露的设备约束                          |

## CPU QoS

内核接口：

- `cpu_latency_qos_add_request(struct pm_qos_request *req, s32 value)`：添加请求，延迟单位为微秒。
- `cpu_latency_qos_update_request()`：更新请求值。
- `cpu_latency_qos_remove_request()`：移除请求。

用户态接口:
- /dev/cpu_dma_latency
- /dev/cpu_wakeup_latency

### 实现原理

对于 CPU latency，无论请求来自 `/dev/cpu_dma_latency` 还是内核的 `cpu_latency_qos_add_request()`，最终都更新同一个约束集合。CPU 进入 idle、由 governor 选择空闲状态时才读取限制：

```text
cpu_latency_qos_add/update/remove_request()
  -> cpu_latency_qos_apply()
     -> pm_qos_update_target()：更新请求和汇总值
     -> 汇总值改变时 wake_up_all_idle_cpus()

cpuidle governor（例如 menu_select()）
  -> cpuidle_governor_latency_req()
     -> cpu_latency_qos_limit() 等：读取约束
  -> 结合退出延迟、预计空闲时间等选择 idle state
  -> cpuidle driver 执行硬件 idle 操作
```

例如 `drivers/cpuidle/governors/menu.c` 的 `menu_select()` 遍历候选状态时，会检查 `s->exit_latency_ns > latency_req`。约束通过限制空闲状态选择生效，不会把硬件本身需要 100 μs 的退出过程加速到 10 μs。

下面的采样栈展示了 menu governor 读取 CPU latency 限制的路径：

```txt
@[
        cpu_latency_qos_limit+1
        cpuidle_governor_latency_req+44
        menu_select+70
        cpuidle_idle_call+218
        do_idle+127
        cpu_startup_entry+38
        rest_init+179
        arch_call_rest_init+10
        start_kernel+925
        x86_64_start_reservations+20
        x86_64_start_kernel+146
        secondary_startup_64_no_verify+404
]: 461
@[
        cpu_latency_qos_limit+1
        cpuidle_governor_latency_req+44
        menu_select+70
        cpuidle_idle_call+218
        do_idle+127
        cpu_startup_entry+38
        start_secondary+277
        secondary_startup_64_no_verify+404
]: 13012
```

## Per-device QoS

每个设备的约束保存在 `dev->power.qos`，同一设备、同一类型的请求一起聚合。主要类型包括：

| 类型                           | 含义                                              | 聚合方式 |
| ------------------------------ | ------------------------------------------------- | -------- |
| `DEV_PM_QOS_RESUME_LATENCY`    | 设备从挂起状态恢复的延迟限制                      | 取最小值 |
| `DEV_PM_QOS_LATENCY_TOLERANCE` | 工作期间可容忍的硬件省电延迟                      | 取最小值 |
| `DEV_PM_QOS_MIN_FREQUENCY`     | 设备频率下限                                      | 取最大值 |
| `DEV_PM_QOS_MAX_FREQUENCY`     | 设备频率上限                                      | 取最小值 |
| `DEV_PM_QOS_FLAGS`             | 例如 `PM_QOS_FLAG_NO_POWER_OFF`，要求设备不被断电 | 按位 OR  |

`dev_pm_qos_expose_latency_limit(dev, value)` 和 `dev_pm_qos_hide_latency_limit(dev)` 是一对用户态入口管理接口

例如在 i9-13900K 这个机器上:

设备                                     数量    属性                           当前值
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 NVMe 控制器 nvme0～2                        3    pm_qos_latency_tolerance_us    100000 μs，即 100 ms
───────────────────────────────────────  ──────  ─────────────────────────────  ────────────────────────────
 Intel Serial IO I2C 控制器 00:15.0～2       3    pm_qos_latency_tolerance_us    auto
───────────────────────────────────────  ──────  ─────────────────────────────  ────────────────────────────
 USB Hub 端口                                8    pm_qos_no_power_off            1，要求不关闭端口电源
───────────────────────────────────────  ──────  ─────────────────────────────  ────────────────────────────
 CPU cpu0～31                               32    pm_qos_resume_latency_us       0，此 sysfs 接口表示无约束

例如

/sys/class/nvme/nvme0/power/pm_qos_latency_tolerance_us

对应源码路径是：

用户修改 latency tolerance
    → PM QoS 汇总设备请求
    → nvme_set_latency_tolerance()
    → nvme_configure_apst()
    → 调整 NVMe 自动低功耗状态配置

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
