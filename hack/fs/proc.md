# proc
- [ ] clear this document and pick useful info from plka/syn

- [ ] 内核中间只有 /proc/sys 的文档 https://www.kernel.org/doc/html/latest/admin-guide/sysctl/index.html

https://www.kernel.org/doc/html/latest/filesystems/proc.html

## /proc/devices
- fs/proc/devices.c
  - `chrdev_show`
  - `blkdev_show`

## /proc/kcore
2. dynamic kernel image : 128T
```
➜  .SpaceVim.d git:(master) ✗ l /proc/kcore
.r-------- root root 128 TB Tue Mar 17 18:47:48 2020   kcore
```

## sysrq-trigger

### b : 直接 reboot

这个不会清理 cache 的，最后调用到这里:
```c
struct machine_ops machine_ops __ro_after_init = {
	.power_off = native_machine_power_off,
	.shutdown = native_machine_shutdown,
	.emergency_restart = native_machine_emergency_restart,
	.restart = native_machine_restart,
	.halt = native_machine_halt,
#ifdef CONFIG_KEXEC_CORE
	.crash_shutdown = native_machine_crash_shutdown,
#endif
};
```
如果是 `shutdown now` 会存在这个调用
```txt
#0  native_machine_power_off () at arch/x86/kernel/reboot.c:737
#1  0xffffffff8117884e in __do_sys_reboot (magic1=-18751827, magic2=<optimized out>, cmd=1126301404, arg=0x7fcbe9d289e0) at kernel/reboot.c:753
#2  0xffffffff8227db9f in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000017f58) at arch/x86/entry/common.c:50
#3  do_syscall_64 (regs=0xffffc90000017f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#4  0xffffffff824000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

# proc

## /proc/stat

https://man7.org/linux/man-pages/man5/proc.5.html

```txt
/proc/stat
              kernel/system statistics.  Varies with architecture.
              Common entries include:

              cpu 10132153 290696 3084719 46828483 16683 0 25195 0
              175628 0
              cpu0 1393280 32966 572056 13343292 6130 0 17875 0 23933 0
                     The amount of time, measured in units of USER_HZ
                     (1/100ths of a second on most architectures, use
                     sysconf(_SC_CLK_TCK) to obtain the right value),
                     that the system ("cpu" line) or the specific CPU
                     ("cpuN" line) spent in various states:

                     user   (1) Time spent in user mode.

                     nice   (2) Time spent in user mode with low
                            priority (nice).

                     system (3) Time spent in system mode.

                     idle   (4) Time spent in the idle task.  This value
                            should be USER_HZ times the second entry in
                            the /proc/uptime pseudo-file.

                     iowait (since Linux 2.5.41)
                            (5) Time waiting for I/O to complete.  This
                            value is not reliable, for the following
                            reasons:

                            1. The CPU will not wait for I/O to
                               complete; iowait is the time that a task
                               is waiting for I/O to complete.  When a
                               CPU goes into idle state for outstanding
                               task I/O, another task will be scheduled
                               on this CPU.

                            2. On a multi-core CPU, the task waiting for
                               I/O to complete is not running on any
                               CPU, so the iowait of each CPU is
                               difficult to calculate.

                            3. The value in this field may decrease in
                               certain conditions.

                     irq (since Linux 2.6.0)
                            (6) Time servicing interrupts.

                     softirq (since Linux 2.6.0)
                            (7) Time servicing softirqs.

                     steal (since Linux 2.6.11)
                            (8) Stolen time, which is the time spent in
                            other operating systems when running in a
                            virtualized environment

                     guest (since Linux 2.6.24)
                            (9) Time spent running a virtual CPU for
                            guest operating systems under the control of
                            the Linux kernel.

                     guest_nice (since Linux 2.6.33)
                            (10) Time spent running a niced guest
                            (virtual CPU for guest operating systems
                            under the control of the Linux kernel).

              page 5741 1808
                     The number of pages the system paged in and the
                     number that were paged out (from disk).

              swap 1 0
                     The number of swap pages that have been brought in
                     and out.

              intr 1462898
                     This line shows counts of interrupts serviced since
                     boot time, for each of the possible system
                     interrupts.  The first column is the total of all
                     interrupts serviced including unnumbered
                     architecture specific interrupts; each subsequent
                     column is the total for that particular numbered
                     interrupt.  Unnumbered interrupts are not shown,
                     only summed into the total.

              disk_io: (2,0):(31,30,5764,1,2) (3,0):...
                     (major,disk_idx):(noinfo, read_io_ops, blks_read,
                     write_io_ops, blks_written)
                     (Linux 2.4 only)

              ctxt 115315
                     The number of context switches that the system
                     underwent.

              btime 769041601
                     boot time, in seconds since the Epoch, 1970-01-01
                     00:00:00 +0000 (UTC).

              processes 86031
                     Number of forks since boot.

              procs_running 6
                     Number of processes in runnable state.  (Linux
                     2.5.45 onward.)

              procs_blocked 2
                     Number of processes blocked waiting for I/O to
                     complete.  (Linux 2.5.45 onward.)

              softirq 229245889 94 60001584 13619 5175704 2471304 28
              51212741 59130143 0 51240672
                     This line shows the number of softirq for all CPUs.
                     The first column is the total of all softirqs and
                     each subsequent column is the total for particular
                     softirq.  (Linux 2.6.31 onward.)
```

## /proc/diskstats
Documentation/iostats.txt

```txt
🧀  cat /proc/diskstats
 259       0 nvme1n1 1141423 165624 55597064 204574 5461706 6557411 242187142 967652 0 1014962 1345517 0 0 0 0 872323 173291
 259       1 nvme1n1p1 1140939 164658 55574251 204415 5461483 6541994 242062036 967375 0 1014812 1171790 0 0 0 0 0 0
 259       2 nvme1n1p2 114 15 7096 48 221 15417 125104 276 0 200 325 0 0 0 0 0 0
 259       3 nvme1n1p3 277 951 12045 102 2 0 2 0 0 92 102 0 0 0 0 0 0
 259       4 nvme0n1 154 0 8144 10 0 0 0 0 0 15 10 0 0 0 0 0 0
 259       5 nvme0n1p1 59 0 4432 4 0 0 0 0 0 6 4 0 0 0 0 0 0
   8      16 sdb 94 0 4504 94 0 0 0 0 0 94 94 0 0 0 0 0 0
   8       0 sda 94 0 4504 32 0 0 0 0 0 29 32 0 0 0 0 0 0
   7       0 loop0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       1 loop1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       2 loop2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       3 loop3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       4 loop4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       5 loop5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       6 loop6 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       7 loop7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
```
