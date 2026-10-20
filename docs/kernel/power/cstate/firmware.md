# firmware 对于 cstate 的影响

我发现了一个很奇怪的事情，intel 的 Xeon
1. cstate 可以在固件层次被关闭
2. 可以在运行时通过 sysfs 重新打开

### 原理
https://www.kernel.org/doc/html/latest/admin-guide/pm/intel_idle.html#enumeration-of-idle-states

枚举 cstate 状态是可以通过 ACPI 和 intel_idle 的，如果这个 CPU 被 intel 识别到了，
可以不用 acpi

Dell BIOS ProcCStates=Disabled
    -> 固件不通过 ACPI _CST 提供 C6
    -> intel_idle 将 C6 标记为 CPUIDLE_FLAG_OFF
    -> Linux 的 C6 sysfs 状态默认 disable=1

具体代码为:
1. `drivers/idle/intel_idle.c` 的 `idle_cpu_skx` 设置 `.use_acpi = true`，`skx_cstates` 仍包含 C6（`MWAIT 0x20`）。
2. `intel_idle_off_by_default()` 在有效 ACPI 状态列表中找不到 `0x20`，返回 true。
3. `intel_idle_init_cstates_icpu()` 给 C6 加上 `CPUIDLE_FLAG_OFF`。C1E 带 `CPUIDLE_FLAG_ALWAYS_ENABLE`，所以即使 ACPI 只提供 C1，C1E 仍默认开启。
4. `drivers/cpuidle/cpuidle.c` 的 `__cpuidle_register_device()` 将 `CPUIDLE_FLAG_OFF` 转换成 `CPUIDLE_STATE_DISABLED_BY_USER`；因此这个默认禁用可通过 sysfs 清除，标记名字里的 `BY_USER` 不代表一定有人手动关闭过。
5. `drivers/cpuidle/sysfs.c` 的 `show_state_default_status()` 则继续报告驱动的默认策略。


### 手动开启 C6 的验证

随后按用户要求，在 `172.21.129.52` 上按 `state*/name=C6` 找到全部 64 个逻辑 CPU 的 C6 状态，将对应 `disable` 写为 `0`。原值备份在远端 `/tmp/6788-c6-before-x6dvrw27.json`。

开启前后分别用 turbostat 采样 5 次、每次 2 秒；开启后先等待 2 秒。机器处于近乎空闲状态，采样时 `Busy%` 为 0.01–0.03%。

| 指标 | 开启前 | 开启后 |
| --- | --- | --- |
| C6 `disable` | 全部 64 个为 1 | 全部 64 个为 0 |
| 硬件核心 C6 驻留率 `CPU%c6` | 0% | 97.86–98.14% |
| 双路 CPU 封装总功耗 `PkgWatt`，5 次平均 | 78.68 W | 70.17 W |
| C6 `default_status` | disabled | disabled |

观察窗口中，全部 64 个逻辑 CPU 的 C6 `usage` 均增长，合计增加 7311 次；
硬件驻留计数也证明确实进入了核心 C6。
短时空闲采样的封装总功耗下降约 8.51 W（10.8%），不是整机墙上功耗，
也不代表所有负载下的节能幅度。本次未做单核 Turbo 或负载性能测试。

结束时重新连接确认全部 C6 `disable=0`，最近 3 分钟内核 journal 没有 warning 及更高优先级记录。当前保留 C6 开启状态，没有修改 BIOS 或启动参数；`default_status` 保持 disabled 是正常的，它记录驱动初始化时的默认策略。在现有配置下重启后仍会恢复默认禁用。


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
