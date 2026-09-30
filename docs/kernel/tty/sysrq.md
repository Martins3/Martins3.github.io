# sysrq

默认是不打开的，防止误触:
echo 1 | sudo tee /proc/sys/kernel/sysrq

## 如何触发 sysrq

 路径                   如何触发                                               条件和特点
━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 /proc/sysrq-trigger    以 root 权限向文件写入命令字符，如 t                   适合本地程序、SSH、脚本；需要用户态还能运行；不受
                                                                               kernel.sysrq 开关限制
─────────────────────  ─────────────────────────────────────────────────────  ─────────────────────────────────────────────────────
 键盘                   x86 上按 Alt + SysRq/PrintScreen + 命令键              经过内核 input 子系统；受 kernel.sysrq 控制
─────────────────────  ─────────────────────────────────────────────────────  ─────────────────────────────────────────────────────
 串口控制台             发送 BREAK，再在 5 秒内发送命令字符                    当前源码需要 CONFIG_MAGIC_SYSRQ_SERIAL，串口驱动支
                                                                               持，且该端口是内核 console
─────────────────────  ─────────────────────────────────────────────────────  ─────────────────────────────────────────────────────
 HVC 虚拟控制台         对采用通用 HVC 接收路径的控制台，按 Ctrl+O，再输入     仅作用于选中的 HVC console；具体取决于控制台实现
                        命令字符
─────────────────────  ─────────────────────────────────────────────────────  ─────────────────────────────────────────────────────
 Xen 管理通道           向目标 guest 的 Xenstore control/sysrq 写命令字符      guest 中的 Xen 驱动监听后调用 handle_sysrq()
─────────────────────  ─────────────────────────────────────────────────────  ─────────────────────────────────────────────────────
 UML 管理控制台         mconsole 的 sysrq <字符> 命令                          仅适用于 User Mode Linux
─────────────────────  ─────────────────────────────────────────────────────  ─────────────────────────────────────────────────────
 KDB 调试器             在 KDB 中执行 sr t 等命令                              已进入 KDB 时可用


  前三种标准入口见 Linux 官方 SysRq 文档 (https://docs.kernel.org/admin-guide/sysrq.html)。

  两个容易混淆的地方：

  - /proc/sys/kernel/sysrq 是开关和功能掩码，/proc/sysrq-trigger 才是触发入口。前者为 0 时，管理员仍能通过后者触发。虽然文档把开关描
    述为影响键盘，但源码里的串口、HVC、Xen 等经 handle_sysrq() 的路径也会检查它。

  - SysRq 不保证在所有死锁下都有效。proc 路径依赖进程得到调度；键盘、串口路径依赖对应输入处理仍能执行，因此用户态卡住时可能有效，但相
    关 CPU 关中断死循环时未必有效。

  常用排障命令字符：w 查看不可中断睡眠任务，t 查看所有任务，l 查看所有活跃 CPU 的栈，m 查看内存信息。c 会主动触发崩溃，b 会立即重启。

  源码入口可看：

  - SysRq 分发与开关检查 (drivers/tty/sysrq.c:583)
  - proc 写入入口 (drivers/tty/sysrq.c:1206)
  - 串口 BREAK 处理 (include/linux/serial_core.h:1312)
  - HVC 的 Ctrl+O 处理 (drivers/tty/hvc/hvc_console.c:704)

### Alt + SysRq/PrintScreen + 命令键 ，我理解只有 vt 才有这个机制吧

和键盘完全去掉耦合的，其实关系不大

## 首先执行 ctrl alt F1 进入到 vt 中
qemu 可以进入到这种 vt 吗? 是可以的

但是，到底什么是

## 测试 sysrq 也是有相同的问题
dmesg 中可以看到的:
```txt
[160673.642166] sysrq: Show Regs

[160673.642701] CPU#11: ctrl:       0001000f000000ff
[160673.642702] CPU#11: status:     0000000000000000
[160673.642702] CPU#11: overflow:   0000000000000000
[160673.642702] CPU#11: fixed:      00000000000000b0
[160673.642703] CPU#11: pebs:       0000000000000000
[160673.642703] CPU#11: debugctl:   0000000000004000
[160673.642704] CPU#11: active:     0000000200000000
[160673.642704] CPU#11:   gen-PMC0 ctrl:  0000000000000000
[160673.642705] CPU#11:   gen-PMC0 count: 0000000000000000
[160673.642705] CPU#11:   gen-PMC0 left:  0000000000000000
[160673.642706] CPU#11:   gen-PMC1 ctrl:  0000000000000000
[160673.642706] CPU#11:   gen-PMC1 count: 0000000000000000
[160673.642706] CPU#11:   gen-PMC1 left:  0000000000000000
[160673.642707] CPU#11:   gen-PMC2 ctrl:  0000000000000000
[160673.642707] CPU#11:   gen-PMC2 count: 0000000000000000
[160673.642708] CPU#11:   gen-PMC2 left:  0000000000000000
[160673.642708] CPU#11:   gen-PMC3 ctrl:  0000000000000000
[160673.642708] CPU#11:   gen-PMC3 count: 0000000000000000
[160673.642709] CPU#11:   gen-PMC3 left:  0000000000000000
[160673.642709] CPU#11:   gen-PMC4 ctrl:  0000000000000000
[160673.642709] CPU#11:   gen-PMC4 count: 0000000000000000
[160673.642710] CPU#11:   gen-PMC4 left:  0000000000000000
[160673.642710] CPU#11:   gen-PMC5 ctrl:  0000000000000000
[160673.642711] CPU#11:   gen-PMC5 count: 0000000000000000
[160673.642711] CPU#11:   gen-PMC5 left:  0000000000000000
[160673.642711] CPU#11:   gen-PMC6 ctrl:  0000000000000000
[160673.642712] CPU#11:   gen-PMC6 count: 0000000000000000
[160673.642712] CPU#11:   gen-PMC6 left:  0000000000000000
[160673.642713] CPU#11:   gen-PMC7 ctrl:  0000000000000000
[160673.642713] CPU#11:   gen-PMC7 count: 0000000000000000
[160673.642713] CPU#11:   gen-PMC7 left:  0000000000000000
[160673.642714] CPU#11: fixed-PMC0 count: 00008004bc8d6009
[160673.642714] CPU#11: fixed-PMC1 count: 0000fffb3f914e2a
[160673.642715] CPU#11: fixed-PMC2 count: 0000000000000000
[160673.642715] CPU#11: fixed-PMC3 count: 0000000000000000
```
但是在 console 中仅仅可以看到
sysrq: Show Regs

## 在 vt 中真的可以执行
fgconsole ，继续调查下

## 当使用 vt 的时候，触发的 backtrace 为这个效果的
```txt
@[
    sysrq_handle_showregs+5
    __handle_sysrq+188
    sysrq_filter+736
    input_handle_events_filter+66
    input_pass_values+297
    input_event_dispose+348
    input_event+93
    hidinput_report_event+55
    hid_report_raw_event+202
    __hid_input_report+328
    hid_input_report+21
    hid_irq_in+496
    __usb_hcd_giveback_urb+145
    usb_giveback_urb_bh+188
    process_one_work+399
    bh_worker+371
    tasklet_hi_action+19
    handle_softirqs+272
    __irq_exit_rcu+245
    irq_exit_rcu+14
    common_interrupt+135
    asm_common_interrupt+38
    cpuidle_enter_state+220
    cpuidle_enter+45
    do_idle+459
    cpu_startup_entry+41
    start_secondary+291
    common_startup_64+318
]: 4
```

## l : 展示所有 CPU 的 backtrace

通过 nmi 实现的
```txt
[161363.082362] sysrq: Show backtrace of all active CPUs
[161363.082912] NMI backtrace for cpu 11
[161363.082913] CPU: 11 UID: 0 PID: 0 Comm: swapper/11 Not tainted 6.13.2 #1-NixOS
[161363.082914] Hardware name: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
[161363.082915] Call Trace:
[161363.082916]  <IRQ>
[161363.082917]  dump_stack_lvl+0x77/0xb0
[161363.082920]  nmi_cpu_backtrace+0xf9/0x140
[161363.082924]  ? __pfx_nmi_raise_cpu_backtrace+0x10/0x10
[161363.082926]  nmi_trigger_cpumask_backtrace+0x11e/0x160
[161363.082928]  __handle_sysrq+0xbc/0x180
[161363.082930]  sysrq_filter+0x2e0/0x560
[161363.082931]  input_handle_events_filter+0x42/0xa0
[161363.082933]  input_pass_values+0x129/0x160
[161363.082933]  input_event_dispose+0x15c/0x170
[161363.082934]  input_event+0x5d/0x90
[161363.082937]  hidinput_report_event+0x37/0x60 [hid]
[161363.082941]  hid_report_raw_event+0xca/0x4c0 [hid]
[161363.082945]  __hid_input_report+0x148/0x210 [hid]
[161363.082948]  hid_input_report+0x15/0x30 [hid]
[161363.082950]  hid_irq_in+0x1f0/0x220 [usbhid]
[161363.082952]  __usb_hcd_giveback_urb+0x91/0x110
[161363.082954]  usb_giveback_urb_bh+0xbc/0x150
[161363.082955]  process_one_work+0x18f/0x3b0
[161363.082956]  bh_worker+0x173/0x1d0
[161363.082957]  tasklet_hi_action+0x13/0x30
[161363.082958]  handle_softirqs+0x110/0x3e0
[161363.082959]  __irq_exit_rcu+0xf5/0x120
[161363.082960]  irq_exit_rcu+0xe/0x20
[161363.082961]  common_interrupt+0x87/0xa0
[161363.082962]  </IRQ>
[161363.082963]  <TASK>
[161363.082963]  asm_common_interrupt+0x26/0x40
[161363.082964] RIP: 0010:cpuidle_enter_state+0xdc/0x450
[161363.082966] Code: 3a ec ff ff 8b 53 04 49 89 c5 0f 1f 44 00 00 31 ff e8 18 c8 3b ff 45 84 ff 0f 85 53 02 00 00 e8
 9a 15 4e ff fb 0f 1f 44 00 00 <45> 85 f6 0f 88 90 01 00 00 49 63 d6 48 8d 04 52 48 8d 04 82 49 8d
[161363.082967] RSP: 0018:ffffb6a1401e7e90 EFLAGS: 00000246
[161363.082967] RAX: 0000000000000000 RBX: ffffd6a13f3a3ed8 RCX: 0000000000000000
[161363.082968] RDX: 0000000000000000 RSI: 0000000000000000 RDI: 0000000000000000
[161363.082968] RBP: 0000000000000002 R08: 0000000000000000 R09: 0000000000000000
[161363.082969] R10: 0000000000000000 R11: 0000000000000000 R12: ffffffffaf9cfae0
[161363.082969] R13: 000092c245340114 R14: 0000000000000002 R15: 0000000000000000
[161363.082970]  cpuidle_enter+0x2d/0x50
[161363.082972]  do_idle+0x1cb/0x220
[161363.082974]  cpu_startup_entry+0x29/0x30
[161363.082975]  start_secondary+0x123/0x140
[161363.082976]  common_startup_64+0x13e/0x141
[161363.082978]  </TASK>
[161363.082978] Sending NMI from CPU 11 to CPUs 0-10,12-31:
[161363.082985] NMI backtrace for cpu 10 skipped: idling at intel_idle+0x59/0xa0
[161363.083025] NMI backtrace for cpu 1 skipped: idling at intel_idle+0x59/0xa0
[161363.083028] NMI backtrace for cpu 0 skipped: idling at intel_idle+0x59/0xa0
[161363.083031] NMI backtrace for cpu 6 skipped: idling at intel_idle+0x59/0xa0
[161363.083034] NMI backtrace for cpu 7 skipped: idling at intel_idle+0x59/0xa0
[161363.083036] NMI backtrace for cpu 2 skipped: idling at intel_idle+0x59/0xa0
[161363.083039] NMI backtrace for cpu 4 skipped: idling at intel_idle+0x59/0xa0
[161363.083042] NMI backtrace for cpu 5 skipped: idling at intel_idle+0x59/0xa0
[161363.083044] NMI backtrace for cpu 3 skipped: idling at intel_idle+0x59/0xa0
[161363.083096] NMI backtrace for cpu 14 skipped: idling at intel_idle+0x59/0xa0
[161363.083099] NMI backtrace for cpu 15 skipped: idling at intel_idle+0x59/0xa0
[161363.083101] NMI backtrace for cpu 13 skipped: idling at intel_idle+0x59/0xa0
[161363.083104] NMI backtrace for cpu 12 skipped: idling at intel_idle+0x59/0xa0
[161363.083106] NMI backtrace for cpu 24 skipped: idling at intel_idle+0x59/0xa0
[161363.083110] NMI backtrace for cpu 26 skipped: idling at intel_idle+0x59/0xa0
[161363.083114] NMI backtrace for cpu 25 skipped: idling at intel_idle+0x59/0xa0
[161363.083120] NMI backtrace for cpu 23 skipped: idling at intel_idle+0x59/0xa0
[161363.083124] NMI backtrace for cpu 9 skipped: idling at intel_idle+0x59/0xa0
[161363.083128] NMI backtrace for cpu 16 skipped: idling at intel_idle+0x59/0xa0
[161363.083132] NMI backtrace for cpu 19 skipped: idling at intel_idle+0x59/0xa0
[161363.083135] NMI backtrace for cpu 8 skipped: idling at intel_idle+0x59/0xa0
[161363.083139] NMI backtrace for cpu 29 skipped: idling at intel_idle+0x59/0xa0
[161363.083143] NMI backtrace for cpu 27 skipped: idling at intel_idle+0x59/0xa0
[161363.083146] NMI backtrace for cpu 17 skipped: idling at intel_idle+0x59/0xa0
[161363.083150] NMI backtrace for cpu 18 skipped: idling at intel_idle+0x59/0xa0
[161363.083153] NMI backtrace for cpu 28 skipped: idling at intel_idle+0x59/0xa0
[161363.083157] NMI backtrace for cpu 31 skipped: idling at intel_idle+0x59/0xa0
[161363.083160] NMI backtrace for cpu 30 skipped: idling at intel_idle+0x59/0xa0
[161363.083164] NMI backtrace for cpu 21 skipped: idling at intel_idle+0x59/0xa0
[161363.083167] NMI backtrace for cpu 20 skipped: idling at intel_idle+0x59/0xa0
[161363.083171] NMI backtrace for cpu 22 skipped: idling at intel_idle+0x59/0xa0
```
## 奇怪的方法
```txt
curl --unix-socket $API_SOCKET -i \
	-X PUT "http://localhost/actions" \
	-H "Accept: application/json" \
	-H "Content-Type: application/json" \
	-d '{
		"action_type": "SendCtrlAltDel"
		}' || pkill firecracker
```

## 当前的 sysrq 的结果

```txt
bash-5.3# stackcount __handle_sysrq
Tracing 1 functions for "__handle_sysrq"... Hit Ctrl-C to end.
^C
  # qmp 发送的 keyboard
  __handle_sysrq
  sysrq_filter
  input_handle_events_filter
  input_pass_values
  input_handle_event
  input_event
  virtinput_recv_events
  vring_interrupt
  vp_vring_interrupt
  __handle_irq_event_percpu
  handle_irq_event
  handle_edge_irq
  __common_interrupt
  common_interrupt
  asm_common_interrupt
  pv_native_safe_halt
  default_idle
  default_idle_call
  do_idle
  cpu_startup_entry
  __pfx_kernel_init
  start_kernel
  x86_64_start_reservations
  x86_64_start_kernel
  common_startup_64
    1

	# serial
  __handle_sysrq
  serial8250_handle_irq.part.0
  serial8250_default_handle_irq
  serial8250_interrupt
  __handle_irq_event_percpu
  handle_irq_event
  handle_edge_irq
  __common_interrupt
  common_interrupt
  asm_common_interrupt
  pv_native_safe_halt
  default_idle
  default_idle_call
  do_idle
  cpu_startup_entry
  __pfx_kernel_init
  start_kernel
  x86_64_start_reservations
  x86_64_start_kernel
  common_startup_64
    1
```

在物理机中测试的结果，的确非常有趣，不过注意，不是所有的键盘都有这个 prtsc 键:
```
@[
        __handle_sysrq+5
        sysrq_handle_keypress+519
        input_handle_events_filter+96
        input_pass_values+338
        input_event_dispose+391
        input_handle_event+65
        input_event+89
        hidinput_report_event+55
        hid_report_raw_event+227
        __hid_input_report+376
        hid_safe_input_report+20
        hid_irq_in+426
        __usb_hcd_giveback_urb+160
        usb_giveback_urb_bh+179
        process_one_work+414
        bh_worker+459
        tasklet_hi_action+19
        handle_softirqs+238
        __irq_exit_rcu+176
        common_interrupt+133
        asm_common_interrupt+38
        cpuidle_enter_state+205
        cpuidle_enter+49
        cpuidle_idle_call+271
        do_idle+156
        cpu_startup_entry+41
        start_secondary+294
        common_startup_64+318
]: 4
```

### 原来 console 的作用体会出现了

• 原因找到了：virtme-03 的 hvc0 没有被配置为内核 console，所以 Ctrl+O,w 被
  当作普通输入。

  我实际检查到：

  - kernel.sysrq=1，开关已打开。
  - /proc/cmdline 只有 console=ttyS0,115200n8。
  - /proc/consoles 只有 ttyS0，没有 hvc0。

  随后在 guest 中读取 /dev/hvc0，再执行 sysrq_hvc，读到了：

  NEW_BYTES=0f77

  这正是 Ctrl+O,w，但内核日志没有新增 SysRq 输出。说明字符确实送到了，内核
  没有把它识别成 SysRq。

  内核 /home/martins3/data/kernel/linux-drm/drivers/tty/hvc/hvc_console.c
  的 __hvc_poll() 只有满足下面条件才解析组合键：

  if (hp->index == hvc_console.index)

  之前实验 VM 特意加了 console=hvc0，virtme-03 没有。要启用，在它的 /home/
  martins3/data/hack/vm/virtme-03/config.ini 的 [vm] 中添加：

  cmdline = console=hvc0

  然后停止并重新启动 QEMU，使新启动参数生效；仅 guest 内重启不会更新 QEMU
  的 -append。

  我写的 action 目前只确认字符已发送，没有校验 guest 的 console 配置，因此
  这里的 Sent 容易让人误以为已经触发。





## 看看键盘中，是如何路由的，为什么有时候是 vt 接手，有时候就可以到 wayland 上了?
实际上没有路由，在任何环境中都是可以继续用的。



## 参考
- https://docs.kernel.org/admin-guide/sysrq.html
- https://docs.kernel.org/admin-guide/sysrq.html
- https://superuser.com/questions/786545/how-to-send-a-break-on-a-serial-port-from-command-line-in-raspbian-linux

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
