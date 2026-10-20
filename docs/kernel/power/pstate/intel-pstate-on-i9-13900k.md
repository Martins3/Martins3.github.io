# intel pstate 有无 hwp 的差别

利用 Intel P-state 在 i9-13900K 与 Xeon Gold 5218 的对比

## 13900K

| 项目                | 实际状态                        |
| ------------------- | ------------------------------- |
| CPU                 | i9-13900K，8 P-core + 16 E-core |
| 逻辑 CPU            | 32，P-core 开启 SMT             |
| 内核                | Fedora `7.1.3-201.fc44.x86_64`  |
| 驱动                | `intel_pstate`                  |
| 模式                | `active`                        |
| HWP                 | 已启用                          |
| governor            | 全部 `performance`              |
| EPP                 | 全部 `performance`              |
| Turbo               | 开启                            |
| `hwp_dynamic_boost` | 关闭                            |
| TuneD               | `throughput-performance`        |

这台机器上，`intel_pstate` 的核心工作可以概括为：

```text
TuneD / sysfs / governor
          |
          v
intel_pstate 计算每个逻辑 CPU 的 min / max / EPP
          |
          v
写入 Intel HWP 请求寄存器
          |
          v
CPU 根据负载、温度、功耗预算自行选择实时 P-state
          |
          v
APERF/MPERF 反馈实际运行频率
```

它不是持续由内核逐档设置具体 MHz，而是给硬件划定范围和偏好，
由 CPU 的 HWP 控制器做快速决策。


启动日志明确显示：

```text
intel_pstate: Intel P-state driver initializing
intel_pstate: HWP enabled
```

内核启动参数没有指定 `intel_pstate=passive` 等选项，因此这颗支持 HWP 的 CPU
默认进入 active 模式。

### 混合 CPU 的性能范围

| 逻辑 CPU   | 类型        |    最低 | Base，即非 Turbo 上限 | 最大 Turbo |
| ---------- | ----------- | ------: | --------------------: | ---------: |
| 0-7、12-15 | P-core      | 0.8 GHz |               3.0 GHz |    5.5 GHz |
| 8-11       | 优选 P-core | 0.8 GHz |               3.0 GHz |    5.8 GHz |
| 16-31      | E-core      | 0.8 GHz |               2.2 GHz |    4.3 GHz |

每个逻辑 CPU 都有独立的 `policyN`。这是因为 Intel 的 HWP 请求接口位于逻辑 CPU
层级，即使两个线程属于同一个 P-core，也分别有 CPUFreq policy。

### Active + HWP 下实际是谁决定频率

`intel_pstate` 主要在 `drivers/cpufreq/intel_pstate.c` 中完成以下工作：

- `intel_pstate_set_policy()`：处理 `performance` 或 `powersave` 选择。
- `intel_pstate_update_perf_limits()`：合并全局百分比限制和每 CPU 频率限制。
- `intel_pstate_hwp_set()`：将 min、max、EPP 等写入 HWP 请求。
- `intel_pstate_set_epp()`：写入 Energy Performance Preference。
- `intel_pstate_update_util_hwp()`：处理调度器回调、频率统计以及可选的动态 boost。

在这台机器的 active + HWP 模式下，调度器回调通常不负责逐次挑选 P-state，CPU
硬件负责挑选；内核主要更新限制、偏好和频率统计。

这里的两个 governor 名字容易误导：

- `performance`：EPP 设为 0，并把硬件有效最低请求提高到允许的最高性能。CPU 一旦执行指令，就倾向于运行在最高性能级别。
- `powersave`：不是锁在最低频率，而是开放一个性能范围，让 HWP 按 EPP、负载和功耗选择，行为更接近 `schedutil`。
- passive 模式才会显示 `scaling_driver=intel_cpufreq`，并使用真正的通用 `schedutil` 等 governor。

### no_turbo 的影响

测试将负载分别固定在 CPU 8 和 CPU 16，每阶段运行 5 秒，并使用 `turbostat` 的
APERF/MPERF 数据测量。

| 配置                            | CPU 8 P-core 忙时频率 | CPU 16 E-core 忙时频率 |     `stress-ng` 吞吐 |
| ------------------------------- | --------------------: | ---------------------: | -------------------: |
| performance + Turbo             |          约 5.485 GHz |              4.300 GHz |    3013 / 1964 ops/s |
| performance + 禁用 Turbo        |          约 2.992 GHz |              2.200 GHz |    1659 / 1009 ops/s |
| powersave + EPP=power，部分负载 |         1.52-2.66 GHz |          1.90-2.65 GHz | 非满负载，不直接比较 |

禁用 Turbo 后，P-core 和 E-core 恰好分别落在 3.0 GHz 和 2.2 GHz 的 base
frequency。这说明 `no_turbo` 控制的是整个 Turbo P-state
范围，而不是简单减掉某个固定频率。

## Xeon Gold

| 项目                | 本机 i9-13900K                      | 远端 Xeon Gold 5218        |
| ------------------- | ----------------------------------- | -------------------------- |
| HWP                 | 支持并启用                          | CPUID 明确显示 `No-HWP`    |
| `intel_pstate` 模式 | active + HWP                        | active + non-HWP           |
| 主要硬件接口        | `MSR_HWP_REQUEST`                   | `MSR_IA32_PERF_CTL`        |
| 选频主体            | CPU 硬件                            | Linux `intel_pstate`       |
| active governor     | `performance`、`powersave`          | `performance`、`powersave` |
| EPP sysfs           | 每 policy 暴露                      | 不暴露                     |
| TuneD               | `throughput-performance`            | `throughput-performance`   |

两台机器的 sysfs 都显示：

```text
status=active
scaling_driver=intel_pstate
scaling_governor=performance
min_perf_pct=100
max_perf_pct=100
no_turbo=0
```

虽然表面配置相同，内部控制路径完全不同。

### 调度器回调和 tracepoint 对比

在两个 SMT 兄弟 policy 都设置为 `powersave`、`min_perf_pct=25`，
并对其中一个逻辑 CPU 施加约 25% duty-cycle 负载后：

| 机器                      |      忙时频率 | `power:pstate_sample` | `power:cpu_frequency` | 解释                                   |
| ------------------------- | ------------: | --------------------: | --------------------: | -------------------------------------- |
| i9-13900K，active HWP     |   约 1.39 GHz |                     0 |                     0 | HWP 硬件自主选频，Linux 不逐次写频率   |
| Xeon 5218，active non-HWP | 1.32-1.42 GHz |           160 次/5 秒 |           160 次/5 秒 | 驱动通过调度器回调按利用率更新 P-state |

Xeon 在连续满载的 `powersave` 测试中，6 秒产生了 572 次 `pstate_sample`，平均约
10.5 ms 一次，与 `intel_pstate` non-HWP active 算法“不超过每 10 ms
更新一次”的设计一致。

在 Xeon 的 `performance` governor 下，即使是 non-HWP，tracepoint 仍然为
0。这是因为 `performance` 算法只需要在 policy 或限制变化时，通过
`intel_pstate_set_policy()` 和 `intel_pstate_set_pstate()` 设置允许的最大
P-state；稳定负载期间不需要按利用率重复更新。

因此，`power:pstate_sample=0` 只能说明采样窗口内 Linux 没有发出新的 P-state
请求，不能单独用来判断驱动是否工作，也不能证明 CPU 没有改变实际频率。

### 临时启用 C6 后的结果

禁用 C6 前:

`turbostat --debug` 读取到的 `MSR_TURBO_RATIO_LIMIT` 为：

```text
28 active cores: 2.8 GHz
24 active cores: 2.8 GHz
20 active cores: 2.8 GHz
16 active cores: 2.8 GHz
12 active cores: 3.1 GHz
 8 active cores: 3.6 GHz
 4 active cores: 3.7 GHz
 2 active cores: 3.9 GHz
```

进一步检查发现，远端机器所有 64 个逻辑 CPU 的 C6 都被禁用：

```text
/sys/devices/system/cpu/cpu*/cpuidle/state3/disable = 1
```

空闲 CPU 只能停在 C1/C1E，无法进入 C6。即使把所有 governor 改成
`powersave`，单线程仍然只能达到 2.8 GHz。

实验临时为所有 CPU 启用 C6，同时设置所有 policy 为
`powersave`、`min_perf_pct=25`，然后只满载 CPU 0：

| 指标                  |  原始配置 | 启用 C6 + powersave |
| --------------------- | --------: | ------------------: |
| CPU 0 单核忙时频率    | 2.800 GHz |     3.859-3.871 GHz |
| 其他 CPU 的 C6 驻留   |        0% |           约 94-97% |
| Socket 0 空闲功耗     | 约 39.7 W |           约 34.0 W |
| Socket 0 单核负载功耗 | 约 50.4 W |           约 43.7 W |
| CPU 0 温度            |   约 46 C |          约 54-55 C |

CPU 0 最终接近 3.9 GHz 的两活跃核 Turbo 档。这证明原先的 2.8 GHz 限制来自
C-state、活跃核心数量和 Turbo ratio 的联动，而不是 `intel_pstate` 不支持 3.9
GHz。

这个结果还说明 P-state 和 C-state 不能割裂分析：

```text
P-state 决定核心醒着时运行多快
C-state 决定核心空闲时睡多深
深度 C-state 又会影响 CPU 对 active core 数量和 Turbo budget 的判断
```

### 两台机器的关键结论

1. i9-13900K 的 active HWP 路径中，Linux 主要设置 min、max 和 EPP，CPU
   硬件自主选择实际频率，所以稳定负载期间可以没有软件频率 tracepoint。

2. Xeon 5218 的 active non-HWP 路径中，`powersave` 算法通过调度器利用率回调约每
   10 ms 计算并写入新的 P-state，因此可以观察到 `pstate_sample` 和
   `cpu_frequency`。

3. 两台机器的 `performance` 都不表示 CPU 永远处于非空闲状态。空闲时仍然可以进入
   C-state，但远端 Xeon 的 C6 被额外禁用，只能进入 C1/C1E。

4. 远端 Xeon 当前的整机配置没有提供最高单线程性能。TuneD 的 `min_perf_pct=100`
   会让 P-state 请求保持激进，但实验证明限制单核 Turbo 的决定性条件是 C6
   被禁用：仅把所有 policy 改成 `powersave` 时仍然只有 2.8 GHz，启用 C6
   后才达到约 3.87 GHz。后续通过 Xeon 的 Dell iDRAC Redfish BIOS 当前配置确认，
   根因是固件设置：

   ```text
   ProcCStates=Disabled
   ProcC1E=Disabled
   SysProfile=PerfOptimized
   ProcPwrPerf=MaxPerf
   ```

   同一时刻 `turbostat --debug` 读取到
   `MSR_PKG_CST_CONFIG_CONTROL=0x00008400`，即 package C-state limit 为
   `PC0`，且 automatic C-state conversion 关闭。这与 BIOS 禁用处理器 C-states
   相互印证。启动参数没有 `intel_idle.max_cstate` 或 `processor.max_cstate`，
   `intel_idle` 的 `max_cstate=9` 也没有限制 C6；TuneD 的
   `throughput-performance` 只设置 performance governor、`min_perf_pct=100`，
   没有通过 `force_latency` 禁用 C-state。因此这里的“默认”是 Dell BIOS 性能
   配置在每次启动时造成的默认状态，而非内核编译选项或 `intel_pstate` 的默认行为。

5. `performance`、禁用深度 C-state 通常是在用功耗和单线程 Turbo
   换取更稳定的唤醒延迟。它可能适合对尾延迟敏感的全核业务，但不应被笼统理解为所有工作负载下性能最高。

### 实验后的恢复状态

所有实验均通过 shell `trap`
保存和恢复状态，没有修改引导参数，也没有重启远端机器。

远端 Xeon 最终状态：

```text
status=active
no_turbo=0
min_perf_pct=100
max_perf_pct=100
performance policies=64
C6 disabled=64
test yes processes=0
```

本机 i9 最终状态：

```text
status=active
no_turbo=0
min_perf_pct=100
max_perf_pct=100
performance policies=32
test yes processes=0
```

### 内核对比

| 测试                             |              4.19 active |         7.2 passive |
| -------------------------------- | -----------------------: | ------------------: |
| performance + Turbo，满载        |                2.800 GHz |           2.800 GHz |
| performance + no_turbo，满载     |                2.300 GHz |           2.300 GHz |
| 25% duty-cycle                   | 1.32-1.42 GHz，powersave | 1.00 GHz，schedutil |
| 25% 测试的软件频率事件           |              160 次/5 秒 |          20 次/4 秒 |
| 临时启用 C6 后单核 Turbo         |            3.86-3.87 GHz |       3.88-3.89 GHz |
| 临时启用 C6 后 Socket 0 空闲功耗 |                约 34.0 W |           约 34.0 W |


因此在这台无 HWP 的 Xeon 上，实际关系是
`schedutil -> intel_cpufreq -> intel_pstate 被动执行 -> MSR_IA32_PERF_CTL`，而不是
旧版 active 模式下由 `intel_pstate` 自己承担 governor 计算。

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
