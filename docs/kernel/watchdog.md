## soft lockup
- https://www.kernel.org/doc/Documentation/lockup-watchdogs.txt
- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/managing_monitoring_and_updating_the_kernel/keeping-kernel-panic-parameters-disabled-in-virtualized-environments_managing-monitoring-and-updating-the-kernel#doc-wrapper

- soft lockup : 如果一个进程 20s 独占 CPU，让其他的 task 不能使用
- hard lockup : 如果一个进程 20s 独占 CPU，连中断都不可以进入

If any CPU in the system does not receive any hrtimer interrupt during that time the
'hardlockup detector' (the handler for the NMI perf event) will generate a kernel warning or call panic, depending on the configuration.


## 分布的文件

kernel/watchdog.c
kernel/watchdog_hld.c
driver/watchdog/
