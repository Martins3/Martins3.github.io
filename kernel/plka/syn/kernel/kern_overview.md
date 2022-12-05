| Name                   | Blank | Comment | code | Explanation                                                                                                                             |
|------------------------|-------|---------|------|-----------------------------------------------------------------------------------------------------------------------------------------|
| module.c               | 652   | 526     | 3220 | module 相关的操作，显然module 比 driver 是一个更加范围广的东西                                                                          |
| workqueue.c            | 869   | 1909    | 3038 | generic async execution with shared worker pool @todo workqueue 为什么不放置irq 中间，还是workqueue 实现根本不是基于此的                |
| sysctl.c               | 202   | 310     | 2735 | proc fs                                                                                                                                 |
| signal.c               | 518   | 954     | 2541 | 实现 signal 机制，需要结合 process/thread/thread group/daemon/session/job control 相关的东西配合才可以真正的理解。                      |
| sys.c                  | 341   | 312     | 1988 | 不是sys的内容，而是提供了大量的简单的syscall 的支持，比如gethostname getuid 之类的                                                      |
| auditsc.c              | 253   | 392     | 1884 | Gateway between the kernel (e.g., selinux) and the user-space audit daemon.  System-call specific features have moved to auditsc.c      |
| fork.c                 | 362   | 393     | 1850 | process-A                                                                                                                               |
| kprobes.c              | 369   | 419     | 1759 | https://www.ibm.com/developerworks/library/l-kprobes/index.html                                                                         |
| futex.c                | 439   | 1478    | 1723 | futex is tricky !                                                                                                                       |
| audit.c                | 300   | 544     | 1546 |                                                                                                                                         |
| cpu.c                  | 322   | 443     | 1503 | cpu hotplug                                                                                                                             |
| auditfilter.c          | 153   | 136     | 1151 |                                                                                                                                         |
| sysctl_binary.c        | 201   | 151     | 1123 | 定义了一个禁止使用的sysctl 系统调用 @todo 其中还定义了大量的table 但是我不知道具体的作用是什么                                          |
| exit.c                 | 214   | 435     | 1111 | 处理 exit 和 wait 系列的syscall                                                                                                         |
| resource.c             | 215   | 329     | 1079 | 管理资源，应该就是特指io映射的                                                                                                          |
| seccomp.c              | 192   | 262     | 890  | https://en.wikipedia.org/wiki/Seccomp Man seccomp(2) @todo 有点意思                                                                     |
| relay.c                | 180   | 290     | 861  | See Documentation/filesystems/relay.txt for an overview.                                                                                |
| ptrace.c               | 187   | 259     | 853  | @todo ptrace 和 trace/ 中间的内容有什么关系                                                                                             |
| user_namespace.c       | 199   | 290     | 838  | https://en.wikipedia.org/wiki/Linux_namespaces　@todo 所以user namespace 实现什么东西的虚拟化 ?                                         |
| audit_tree.c           | 138   | 84      | 812  |                                                                                                                                         |
| kexec_file.c           | 195   | 226     | 808  |                                                                                                                                         |
| kexec_core.c           | 167   | 295     | 746  |                                                                                                                                         |
| padata.c               | 207   | 186     | 719  | 应该是并行计算的                                                                                                                        |
| params.c               | 147   | 96      | 717  | 内核参数解析之类的操作                                                                                                                  |
| kthread.c              | 152   | 409     | 671  |                                                                                                                                         |
| jump_label.c           | 127   | 116     | 557  | https://lwn.net/Articles/412072/                                                                                                        |
| torture.c              | 76    | 177     | 541  | @todo                                                                                                                                   |
| cred.c                 | 101   | 190     | 527  | Task credentials management                                                                                                             |
| taskstats.c            | 109   | 81      | 511  |                                                                                                                                         |
| softirq.c              | 114   | 175     | 474  |                                                                                                                                         |
| kallsyms.c             | 102   | 154     | 454  | 处理syms的东西，@todo 检查一下其中的使用位置                                                                                            |
| watchdog.c             | 109   | 218     | 453  | 大名鼎鼎的watchdog机制，其实                                                                                                      |
| profile.c              | 75    | 70      | 420  | http://www.pixelbeat.org/programming/profiling/　具体内容不知道，很有可能是sched 的debug工具                                            |
| umh.c                  | 85    | 184     | 417  | https://kernelnewbies.org/KernelProjects/usermode-helper-enhancements @todo 尚且没有阅读                                                |
| smp.c                  | 116   | 269     | 414  | 提供一些辅助函数来实现 cpu                                                                                                              |
| audit_watch.c          | 92    | 75      | 402  |                                                                                                                                         |
| acct.c                 | 64    | 146     | 396  | acct 系统调用                                                                                                                           |
| stop_machine.c         | 84    | 205     | 393  | @todo 关机 ?                                                                                                                            |
| panic.c                | 86    | 187     | 392  |                                                                                                                                         |
| tracepoint.c           | 73    | 145     | 387  | trace 机制，诡异!                                                                                                                       |
| compat.c               | 61    | 30      | 355  | Kernel compatibililty routines for e.g. 32 bit syscall support on 64 bit kernels.                                                       |
| crash_core.c           | 67    | 55      | 353  | crashkernel 内核参数                                                                                                                    |
| reboot.c               | 76    | 175     | 333  |                                                                                                                                         |
| kcov.c                 | 45    | 76      | 333  | https://www.kernel.org/doc/html/latest/dev-tools/kcov.html                                                                              |
| pid_namespace.c        | 73    | 73      | 322  |                                                                                                                                         |
| pid.c                  | 70    | 77      | 321  |                                                                                                                                         |
| smpboot.c              | 63    | 113     | 305  |                                                                                                                                         |
| fail_function.c        | 55    | 18      | 283  | 应该和debug有关的，但是具体作用不知道                                                                                                   |
| notifier.c             | 53    | 237     | 274  | 似乎linux-inside讲过                                                                                                                    |
| test_kprobes.c         | 55    | 17      | 250  |                                                                                                                                         |
| audit.h                | 38    | 59      | 248  |                                                                                                                                         |
| memremap.c             | 56    | 72      | 247  | io映射的重新映射                                                                                                                        |
| capability.c           | 64    | 208     | 237  | getting and setting thread capabilities                                                                                                 |
| sys_ni.c               | 100   | 102     | 235  | ni是什么意思，提供统一的syscall 接口 ?                                                                                                  |
| ksysfs.c               | 34    | 20      | 217  | sysfs attributes in /sys/kernel                                                                                                         |
| ucount.c               | 28    | 13      | 205  | 应该是处理namespace 的相关的，不清楚                                                                                                    |
| nsproxy.c              | 39    | 34      | 204  | task_struct 持有 nsproxy 成员，应该是处理所有的namespace的根源                                                                          |
| rseq.c                 | 37    | 142     | 188  | Restartable sequences system call : https://www.phoronix.com/scan.php?page=news_item&px=Restartable-Sequences-Speed                     |
| kcmp.c                 | 40    | 29      | 187  | compare two processes to determine if they share a kernel resource                                                                      |
| kexec.c                | 52    | 76      | 183  |                                                                                                                                         |
| latencytop.c           | 46    | 78      | 182  | @todo 有注释                                                                                                                            |
| hung_task.c            | 45    | 63      | 174  | kernel thread for detecting tasks stuck in D state https://stackoverflow.com/questions/20423521/process-permanently-stuck-on-d-state    |
| uid16.c                | 42    | 5       | 174  | 一些兼容性的代码                                                                                                                        |
| groups.c               | 41    | 27      | 173  | Supplementary group IDs                                                                                                                 |
| watchdog_hld.c         | 43    | 87      | 165  |                                                                                                                                         |
| async.c                | 53    | 117     | 163  | Asynchronous function calls for boot performance                                                                                        |
| audit_fsnotify.c       | 31    | 23      | 162  |                                                                                                                                         |
| user.c                 | 30    | 45      | 160  | 管理user的，和 groups.c 中间对应                                                                                                        |
| utsname.c              | 31    | 21      | 131  | 处理uts namespace                                                                                                                       |
| futex_compat.c         | 32    | 39      | 131  |                                                                                                                                         |
| range.c                | 28    | 12      | 124  | range相关算法库                                                                                                                         |
| tsacct.c               | 21    | 45      | 119  | System accounting over taskstats interface　进程的统计                                                                                  |
| delayacct.c            | 26    | 27      | 118  | 进程统计                                                                                                                                |
| context_tracking.c     | 29    | 73      | 116  | 感觉是一个debug机制                                                                                                                     |
| irq_work.c             | 34    | 49      | 112  | Provides a framework for enqueueing and running callbacks from hardirq context. The enqueueing is NMI-safe.                             |
| kmod.c                 | 22    | 50      | 106  | the kernel module loader                                                                                                                |
| Makefile               | 13    | 11      | 105  |                                                                                                                                         |
| utsname_sysctl.c       | 17    | 30      | 101  |                                                                                                                                         |
| iomem.c                | 22    | 44      | 101  | iomem                                                                                                                                   |
| extable.c              | 22    | 61      | 93   | exception table 处理的辅助函数                                                                                                          |
| freezer.c              | 30    | 61      | 90   | Function to freeze a process                                                                                                            |
| cpu_pm.c               | 21    | 106     | 82   | cpu 相关的电池管理                                                                                                                      |
| dma.c                  | 29    | 43      | 77   | A DMA channel allocator https://www.kernel.org/doc/html/v4.19/driver-api/dmaengine/provider.html @todo 难道dma 全部的interface 在此处 ? |
| up.c                   | 17    | 13      | 72   | Uniprocessor-only support functions                                                                                                     |
| backtracetest.c        | 17    | 11      | 63   |                                                                                                                                         |
| task_work.c            | 11    | 49      | 58   | 实现的非常简单的sched 机制，@todo 不知道谁来使用                                                                                        |
| stacktrace.c           | 12    | 13      | 54   |                                                                                                                                         |
| module_signing.c       | 11    | 23      | 54   | Module signature checker                                                                                                                |
| configs.c              | 16    | 37      | 46   | 一个kernel module 来实现 Echo the kernel .config file used to build the kernel @todo怎么使用                                            |
| workqueue_internal.h   | 12    | 30      | 33   |                                                                                                                                         |
| exec_domain.c          | 6     | 9       | 31   |                                                                                                                                         |
| user-return-notifier.c | 6     | 10      | 28   |                                                                                                                                         |
| crash_dump.c           | 4     | 20      | 22   |                                                                                                                                         |
| module-internal.h      | 3     | 11      | 21   |                                                                                                                                         |
| kexec_internal.h       | 4     | 1       | 21   |                                                                                                                                         |
| elfcore.c              | 4     | 1       | 20   | 一堆week 的空函数                                                                                                                       |
| smpboot.h              | 5     | 1       | 17   |                                                                                                                                         |
| bounds.c               | 2     | 9       | 15   |                                                                                                                                         |
| uid16.h                | 2     | 1       | 11   |

1. audit
2. reboot panic crash boot
3. 进程生老病死的管理，fork exit exec 等，sched/ 持有内容都是进程调度而已
4. workqueu tasklet softirq 其实也是放到进程管理中间了!
5. module
6. io 映射
7. kexec
8. 内核参数解析
9. smp 现在才注意到smp 和 percpu 定义的微妙区别
10. user group
11. 各种namespace

module.c 对应chapter 7 中间的内容:

1. mod_tree_insert 各种mod_tree 的操作
2. mod_sysfs_init sysfs 相关操作
3. lookup_module_symbol_attrs symbol 相关的操作
> 总体来说更像是对于二进制文件的操作

> 本来以为module 和 device 放在一起，其实整个关注的内容完全的不同，放在文件夹的位置也是完全不同的。
