# panic 的参数配置

```txt
        panic_print=    Bitmask for printing system info when panic happens.
                        User can chose combination of the following bits:
                        bit 0: print all tasks info
                        bit 1: print system memory info
                        bit 2: print timer info
                        bit 3: print locks info if CONFIG_LOCKDEP is on
                        bit 4: print ftrace buffer
                        bit 5: print all printk messages in buffer
                        bit 6: print all CPUs backtrace (if available in the arch)
                        bit 7: print only tasks in uninterruptible (blocked) state
                        *Be aware* that this option may print a _lot_ of lines,
                        so there are risks of losing older messages in the log.
                        Use this option carefully, maybe worth to setup a
                        bigger log buffer with "log_buf_len" along with this.
```
## kdump.service 被 disable 之后，还是可以有 kdump 可以正确触发

kdump.service 只是用于生成 initramfs 的。


## CONFIG_PANIC_ON_OOPS
<!-- 9a090b99-fb4a-4b23-a6f9-939fe3d485e5 -->

fedora 内核默认没有配置 oops on panic
```txt
🧀  cat /boot/config-6.18.8-100.fc42.x86_64| grep CONFIG_PANIC
# CONFIG_PANIC_ON_OOPS is not set
CONFIG_PANIC_TIMEOUT=0

🧀  cat /boot/config-6.19.0-00001-g3c1afd4a5b7c | grep CONFIG_PANIC // 我们的配置
# CONFIG_PANIC_ON_OOPS is not set
CONFIG_PANIC_TIMEOUT=0
```

在 hyper-v 虚拟机中，也就是我们自己构建的内核，如果 panic 了，
可以发现只是哪一个进程卡主了，但是系统可以继续运行，
echo c | sudo tee /proc/sysrq-trigger 后可以收集到 vmcore 文件。

```txt
cat /boot/config-6.19.0-00001-g3c1afd4a5b7c | grep CONFIG_PANIC
# CONFIG_PANIC_ON_OOPS is not set
```

```txt
[33515.987667] BUG: unable to handle page fault for address: ffff888005520bc0
[33515.987753] #PF: supervisor write access in kernel mode
[33515.987813] #PF: error_code(0x0003) - permissions violation
[33515.987876] PGD 7a01067 P4D 7a01067 PUD 7a02067 PMD 80000000054001a1
[33515.987954] Oops: Oops: 0003 [#1] SMP NOPTI
[33515.987999] CPU: 6 UID: 1000 PID: 87084 Comm: gix_status::dir Kdump: loaded Not tainted 6.19.0-00001-g3c1afd4a5b7c #1 PREEMPT(none)
[33515.988153] Hardware name: Microsoft Corporation Virtual Machine/Virtual Machine, BIOS Hyper-V UEFI Release v4.1 09/04/2024
[33515.988294] RIP: 0010:sched_mm_cid_exit+0xb1/0x1b0
[33515.988349] Code: 4d 65 48 8b 05 38 05 da 01 4c 3b b0 70 06 00 00 75 70 48 8b 8d c0 00 00 00 65 48 03 0d 30 05 da 01 8b 01 25 ff ff ff bf 89 01 <f0> 48 0f b3 85 c0 0b 00 00 48 81 fa ff ef ff ff 77 08 4c 89 ef e8
[33515.988599] RSP: 0018:ffffc900190e3e78 EFLAGS: 00010002
[33515.988658] RAX: 000000002000000e RBX: ffff88811bb9cd80 RCX: ffffe8fffda18c00
[33515.988743] RDX: ffff88800152013f RSI: 0000000000000018 RDI: 0000000000000002
[33515.988828] RBP: ffff888001520000 R08: 0000000000000000 R09: 0000000000000001
[33515.988912] R10: ffffc900190e3e78 R11: 0000000000000000 R12: ffff888001520188
[33515.988997] R13: ffff888001520140 R14: ffff888001520000 R15: 0000000000000000
[33515.989081] FS:  00007fb02f7dc6c0(0000) GS:ffff88826eb45000(0000) knlGS:0000000000000000
[33515.989180] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[33515.989246] CR2: ffff888005520bc0 CR3: 000000000cd50000 CR4: 0000000000350ef0
[33515.989331] Call Trace:
[33515.989351]  <TASK>
[33515.989367]  do_exit+0xb0/0xa20
[33515.989398]  __x64_sys_exit+0x1b/0x20
[33515.989435]  x64_sys_call+0x1502/0x1510
[33515.989476]  do_syscall_64+0x75/0xf80
[33515.989514]  entry_SYSCALL_64_after_hwframe+0x76/0x7e
[33515.989571] RIP: 0033:0x7fb0554f7916
[33515.989606] Code: ff d0 e9 c3 fe ff ff 48 8b 44 24 08 31 c9 48 89 88 20 06 00 00 31 c0 87 03 83 e8 01 7f 0e ba 3c 00 00 00 90 31 ff 89 d0 0f 05 <eb> f8 48 89 df e8 e0 ce ff ff 83 ed 01 0f 85 aa fd ff ff eb df 48
[33515.989853] RSP: 002b:00007fb02f7dbca0 EFLAGS: 00000246 ORIG_RAX: 000000000000003c
[33515.989944] RAX: ffffffffffffffda RBX: 00007fb02f7dccdc RCX: 00007fb0554f7916
[33515.990029] RDX: 000000000000003c RSI: 0000000000000000 RDI: 0000000000000000
[33515.990113] RBP: 00007fb02f5dc000 R08: 0000000000000000 R09: 0000000000000000
[33515.990198] R10: 0000000000000000 R11: 0000000000000246 R12: 0000000000201000
[33515.990283] R13: 0000000000000002 R14: 00007fb02fff4800 R15: 0000000000201000
[33515.990371]  </TASK>
[33515.990386] Modules linked in: sunrpc af_packet nls_iso8859_1 nls_cp437 vfat fat hv_netvsc hv_balloon fuse configfs nfnetlink zram vsock_loopback zsmalloc vmw_vsock_virtio_transport_common zstd_compress vsock xfs hyperv_drm drm_client_lib drm_shmem_helper drm_kms_helper drm i2c_core hv_storvsc drm_panel_orientation_quirks scsi_transport_fc hid_hyperv hyperv_fb hyperv_keyboard hv_vmbus dm_mod efivarfs autofs4
[33515.990893] CR2: ffff888005520bc0
[33515.990924] ---[ end trace 0000000000000000 ]---
[33515.990974] RIP: 0010:sched_mm_cid_exit+0xb1/0x1b0
[33515.991027] Code: 4d 65 48 8b 05 38 05 da 01 4c 3b b0 70 06 00 00 75 70 48 8b 8d c0 00 00 00 65 48 03 0d 30 05 da 01 8b 01 25 ff ff ff bf 89 01 <f0> 48 0f b3 85 c0 0b 00 00 48 81 fa ff ef ff ff 77 08 4c 89 ef e8
[33515.991276] RSP: 0018:ffffc900190e3e78 EFLAGS: 00010002
[33515.991334] RAX: 000000002000000e RBX: ffff88811bb9cd80 RCX: ffffe8fffda18c00
[33515.991419] RDX: ffff88800152013f RSI: 0000000000000018 RDI: 0000000000000002
[33515.991504] RBP: ffff888001520000 R08: 0000000000000000 R09: 0000000000000001
[33515.991588] R10: ffffc900190e3e78 R11: 0000000000000000 R12: ffff888001520188
[33515.991672] R13: ffff888001520140 R14: ffff888001520000 R15: 0000000000000000
[33515.991757] FS:  00007fb02f7dc6c0(0000) GS:ffff88826eb45000(0000) knlGS:0000000000000000
[33515.991855] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[33515.991920] CR2: ffff888005520bc0 CR3: 000000000cd50000 CR4: 0000000000350ef0
[33515.992005] note: gix_status::dir[87084] exited with irqs disabled
[33515.992109] note: gix_status::dir[87084] exited with preempt_count 1
[33582.311654] sysrq: Trigger a crash
[33582.311693] Kernel panic - not syncing: sysrq triggered crash
[33582.313170] Kernel Offset: disabled
```

那么为什么最后 echo c > /proc/sysrq-trigger 可以导致 panic 呢，是由于:

4.19 内核是这样写的，目前的内核应该类似，但是代码不是这么写的:
```c
static void sysrq_handle_crash(int key)
{
	char *killer = NULL;

	/* we need to release the RCU read lock here,
	 * otherwise we get an annoying
	 * 'BUG: sleeping function called from invalid context'
	 * complaint from the kernel before the panic.
	 */
	rcu_read_unlock();
	panic_on_oops = 1;	/* force panic */
	wmb();
	*killer = 1;
}
```

但是实际上，一般这个是需要打开的

## panic 的默认行为是什么
<!-- ac853dd3-32d7-46bb-a9e5-3c5d211df02b -->

各个发行版都是 CONFIG_PANIC_TIMEOUT=0 关联的参数为 /sys/module/kernel/parameters/panic ，此时:
- 如果 kdump 配置异常，系统无限等待
- 如果 kdump 正常，系统进入到 kdump 生成的机制中。


特殊情况，如果配置内核参数 panic 不为 0,  无论 kdump 配置是否正常，系统都是会重启的，默认环境都不会这么用，
就不用考虑了。
```txt
      oops=panic      [KNL,EARLY]
                        Always panic on oopses. Default is to just kill the
                        process, but there is a small probability of
                        deadlocking the machine.
                        This will also cause panics on machine check exceptions.
                        Useful together with panic=30 to trigger a reboot.

        panic=          [KNL] Kernel behaviour on panic: delay <timeout>
                        timeout > 0: seconds before rebooting
                        timeout = 0: wait forever
                        timeout < 0: reboot immediately
                        Format: <timeout>
```

当系统无限等待的时候:
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - ksys_write
        - vfs_write
          - proc_reg_write
            - pde_write
              - write_sysrq_trigger
                - __handle_sysrq
                  - sysrq_handle_crash
                    - panic
                      - delay_halt
                        - delay_halt
                          - delay_halt_tpause


## TODO
其中包括 panic ，可以控制 hardlock 之后多长时间自动重启:
https://cedwards.xyz/use-this-kernel-parameter-in-your-kiosk/

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
