# bmc
## 尝试分析一下 bmc 中使用键盘鼠标的整个 io 路径
i8042 在硬中断里面:

```txt
kbd_event+5
input_handle_events_default+66
input_pass_values+307
input_event_dispose+322
input_event+78
atkbd_receive_byte+1350
ps2_interrupt+158
serio_interrupt+71
i8042_handle_data+247
i8042_interrupt+17
__handle_irq_event_percpu+74
handle_irq_event+59
handle_edge_irq+151
__common_interrupt+62
common_interrupt+128
asm_common_interrupt+38
pv_native_safe_halt+15
default_idle+19
default_idle_call+48
do_idle+437
cpu_startup_entry+41
start_secondary+247
common_startup_64+318
```

但是 usb 在软中断里面在软中断里面:
```txt
@[
        show_state_filter+0
        k_spec+112
        kbd_keycode.constprop.0+856
        kbd_event+308
        input_handle_events_default+96
        input_pass_values+288
        input_event_dispose+312
        input_handle_event+88
        input_event+104
        hidinput_report_event+72
        hid_report_raw_event+164
        __hid_input_report.constprop.0+284
        hid_input_report+24
        hid_irq_in+584
        __usb_hcd_giveback_urb+160
        usb_giveback_urb_bh+240
        process_one_work+376
        bh_worker+552
        workqueue_softirq_action+128
        tasklet_hi_action+28
        handle_softirqs+300
        __do_softirq+28
        ____do_softirq+24
        call_on_irq_stack+36
        do_softirq_own_stack+36
        __irq_exit_rcu+316
        irq_exit_rcu+24
        el1_interrupt+72
        el1h_64_irq_handler+24
        el1h_64_irq+128
        default_idle_call+56
        cpuidle_idle_call+380
        do_idle+244
        cpu_startup_entry+64
        secondary_start_kernel+224
        __secondary_switched+192
]: 1
```

code: 4, type: 4, value: 458889
code: 124, type: 1, value: 1
code: 0, type: 0, value: 0
code: 4, type: 4, value: 458889
code: 124, type: 1, value: 0
code: 0, type: 0, value: 0

```txt
  // 进入到 kspec 中
	(*k_handler[type])(vc, keysym & 0xff, !down);

  这里  type = 2 ，keysym & 0xff = 4
```

解释这个问题:
```txt
sudo bpftrace -e 'fentry:kbd_event { printf("code: %d, type: %d, value: %d\n", args->event_code, args->event_type, args->value); }'
```

正常的机器上按 f
```txt
code: 4, type: 4, value: 33
code: 33, type: 1, value: 1
code: 0, type: 0, value: 0
code: 4, type: 4, value: 33
code: 33, type: 1, value: 0
code: 0, type: 0, value: 0
```

按下 F 键：
type: 4, code: 4, value: 33：硬件报告扫描码 33（F 键）。
type: 1, code: 33, value: 1：输入子系统记录按下 F 键。
释放 F 键：
type: 4, code: 4, value: 33：硬件报告扫描码 33（可能表示释放）。
type: 1, code: 33, value: 0：输入子系统记录释放 F 键。

code 解码: include/uapi/linux/input-event-codes.h


```txt
code: 4, type: 4, value: 458889
code: 124, type: 1, value: 1
code: 0, type: 0, value: 0

code: 4, type: 4, value: 458889
code: 124, type: 1, value: 0
code: 0, type: 0, value: 0

code: 4, type: 4, value: 458976
code: 29, type: 1, value: 1
code: 0, type: 0, value: 0

code: 4, type: 4, value: 458774
code: 31, type: 1, value: 1
code: 0, type: 0, value: 0
code: 2, type: 17, value: 1
(Scroll Lock LED 打开，可能由系统或键盘驱动触发。)

code: 4, type: 4, value: 458774
code: 31, type: 1, value: 0
code: 0, type: 0, value: 0
(S)

code: 4, type: 4, value: 458976
code: 29, type: 1, value: 0
code: 0, type: 0, value: 0

code: 4, type: 4, value: 458976
code: 29, type: 1, value: 1

code: 4, type: 4, value: 458823
code: 70, type: 1, value: 1
(释放 Scroll Lock)
code: 0, type: 0, value: 0

code: 4, type: 4, value: 458823
code: 70, type: 1, value: 0
code: 0, type: 0, value: 0

code: 4, type: 4, value: 458976
code: 29, type: 1, value: 0
code: 0, type: 0, value: 0

code: 4, type: 4, value: 458774
code: 31, type: 1, value: 1
code: 0, type: 0, value: 0

code: 4, type: 4, value: 458774
code: 31, type: 1, value: 0
code: 0, type: 0, value: 0

code: 4, type: 4, value: 458823
code: 70, type: 1, value: 1
code: 0, type: 0, value: 0

code: 2, type: 17, value: 0
code: 4, type: 4, value: 458823
code: 70, type: 1, value: 0
code: 0, type: 0, value: 0

code: 4, type: 4, value: 458976
code: 29, type: 1, value: 1
code: 0, type: 0, value: 0

code: 4, type: 4, value: 458983
code: 126, type: 1, value: 1
code: 0, type: 0, value: 0

code: 4, type: 4, value: 458976
code: 29, type: 1, value: 0
code: 4, type: 4, value: 458983
code: 126, type: 1, value: 0
code: 0, type: 0, value: 0
```

## qemu 中运行 bmc
https://jia.je/system/2023/08/11/openbmc-qemu/#ast2600

## bmc 和 ipmi 是什么关系

他们的用户态程序都是什么?

相当于物理机上还有一个 ipmi 的设备

当然，这个日志现在是虚拟机中观察到的:
```txt
[    2.748021][  T792] ipmi_si: IPMI System Interface driver
[    2.748798][  T792] ipmi_si dmi-ipmi-si.0: ipmi_platform: probing via SMBIOS
[    2.749385][  T792] ipmi_platform: ipmi_si: SMBIOS: io 0x0 regsize 1 spacing 1 irq 0
[    2.750255][  T792] ipmi_si: Adding SMBIOS-specified kcs state machine
[    2.750915][  T792] ipmi_si 0000:00:0a.0: probing via PCI
[    2.752672][  T792] ipmi_si 0000:00:0a.0: [io  0xc390-0xc397] regsize 1 spacing 1 irq 10
[    2.753529][  T792] ipmi_si: Adding PCI-specified kcs state machine
[    2.754177][  T792] ipmi_si: Trying PCI-specified kcs state machine at i/o address 0xc390, slave address 0x0, irq 10
[    2.755492][  T792] ipmi_si 0000:00:0a.0: Using irq 10
[    2.766942][  T792] ipmi_si 0000:00:0a.0: IPMI message handler: Found new BMC (man_id: 0x000000, prod_id: 0x0000, dev_id: 0x20)
[    2.771969][  T792] ipmi_si 0000:00:0a.0: IPMI kcs interface initialized
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
