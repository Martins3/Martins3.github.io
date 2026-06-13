# trace 传统工具

# procs

| 命令    | 解释                                                         | 特殊说明                             |
|---------|--------------------------------------------------------------|--------------------------------------|
| free    | Report the amount of free and used memory in the system      |
| kill    | Send a signal to a process based on PID                      |
| pgrep   | List processes based on name or other attributes             |
| pkill   | Send a signal to a process based on name or other attributes |
| pmap    | Report memory map of a process                               | /proc/pid/maps 和 smaps 重新解析而已 |
| ps      | Report information of processes                              |
| pwdx    | Report current directory of a process                        |                                  |
| slabtop | Display kernel slab cache information in real time           |
| sysctl  | Read or Write kernel parameters at run-time                  |
| top     | Dynamic real-time view of running processes                  |
| uptime  | Display how long the system has been running                 |
| vmstat  | Report virtual memory statistics                             |
| w       | Report logged in users and what they are doing               |
| watch   | Execute a program periodically, showing output fullscreen    |
| cifsiostat|
| tapestat|

在 fedora 中安装，移除掉翻译后，是这些文件:
```txt
🤒  sudo rpm -ql sysstat
/etc/sysconfig/sysstat
/etc/sysconfig/sysstat.ioconf
/usr/bin/cifsiostat
/usr/bin/iostat
/usr/bin/mpstat
/usr/bin/pidstat
/usr/bin/sadf
/usr/bin/sar
/usr/bin/tapestat
/usr/lib/systemd/system-sleep/sysstat.sleep
/usr/lib/systemd/system/sysstat-collect.service
/usr/lib/systemd/system/sysstat-collect.timer
/usr/lib/systemd/system/sysstat-rotate.service
/usr/lib/systemd/system/sysstat-rotate.timer
/usr/lib/systemd/system/sysstat-summary.service
/usr/lib/systemd/system/sysstat-summary.timer
/usr/lib/systemd/system/sysstat.service
/usr/lib64/sa
/usr/lib64/sa/sa1
/usr/lib64/sa/sa2
/usr/lib64/sa/sadc
/usr/share/doc/sysstat
/usr/share/doc/sysstat/CHANGES
/usr/share/doc/sysstat/CREDITS
/usr/share/doc/sysstat/FAQ.md
/usr/share/doc/sysstat/README.md
/usr/share/man/man1/iostat.1.gz
/usr/share/man/man1/mpstat.1.gz
/usr/share/man/man1/pidstat.1.gz
/usr/share/man/man1/sadf.1.gz
/usr/share/man/man1/sar.1.gz
/usr/share/man/man1/cifsiostat.1.gz
/usr/share/man/man1/tapestat.1.gz
/usr/share/man/man5/sysstat.5.gz
/usr/share/man/man8/sa1.8.gz
/usr/share/man/man8/sa2.8.gz
/usr/share/man/man8/sadc.8.gz
/var/log/sa
```

## vmstat

vmstat 1

```txt
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
 r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
 2  0      0 19883668 271528 4860896    0    0   153    53  283  344  1  1 98  0  0
 0  0      0 19882456 271528 4860972    0    0     0     4 6806 10110  0  0 99  0  0
 1  0      0 19884664 271528 4861036    0    0     0     4 9503 12598  2  0 97  0  0
 1  0      0 19900404 271528 4861164    0    0     0   728 12225 16093  4  0 96  0  0
```

## top
<!-- 202b1e15-b30a-4ffd-ba8c-4174004b9674 -->
- 打开 top, 按数值 1 的时候，可以检查 CPU 的 softirq (si) hardware irq (hi)

- https://unix.stackexchange.com/questions/128953/how-to-display-top-results-sorted-by-memory-usage-in-real-time
  - top 的排序为 `top -o %MEM`

和 htop 相同，通过按 H 可以详细的展示每一个 thread 的内容，这个更好用，可以展示 thread 的名称。

### 展示 Last Used CPU
1. 在 top 中临时开启（交互式）

启动 top 后，按 f 进入字段管理界面：

• 找到 P (Last Used Cpu) 或 Last Used CPU (SMP)
• 按 Space 选中（前面会出现 *）
• 按 q 返回主界面

此时 P 列就会显示每个进程最后一次在哪个 CPU 核心上运行。

2. 保存为默认配置
在 top 中按 Shift + W（即大写 W），可以将当前字段设置保存到 ~/.toprc，下次启动 top 自动生效。

## ps

- ps -elf

- ps aux : 和 ps -elf 等价
> a = show processes for all users
> u = display the process's user/owner
> x = also show processes not attached to a terminal

[What does aux mean in `ps aux`?](https://unix.stackexchange.com/questions/106847/what-does-aux-mean-in-ps-aux)

[ps 输出排序](https://superuser.com/questions/481423/reverse-order-using-command)
- ps -elf --sort -pid : 递减排序
- ps -elf --sort +pid : 递增排序


# sysstat

> iostat reports CPU statistics and input/output statistics for block devices and partitions.
> mpstat reports individual or combined processor related statistics.
> pidstat reports statistics for Linux tasks (processes) : I/O, CPU, memory, etc.

### sar
<!-- 2dfab771-7f1d-4456-8559-8a827dc81253 -->

最核心就记住这个就可以了:
sar -n DEV 1

- https://medium.com/@malith.jayasinghe/network-monitoring-using-sar-37bab6ce9f68
- sar -n DEV 1 : 监控 nic 的流量
- sar -n EDEV : 监控 nic 的错误
- sar -n TCP,ETCP 1
- sar -d -p 1 : disk
  - -p 显示的更加科学点
- sar -B 1 : 内存管理之类的

在这里 https://www.brendangregg.com/linuxperf.html
找到了这个图总结，真不错啊
https://www.brendangregg.com/Perf/linux_static_tools.png

sar 的配套工具，找到系统中长时间的记录:
1. sadf - Display data collected by sar in multiple formats.
2. sadc - System activity data collector.

例如是 8 号这天的日志:
```txt
sar -P 1,2 -f  /var/log/sa/sa08 | rg "10:.*:.*"
```

注意，sar 配置了一个服务就可以了:
```txt
systemctl status sysstat-collect
○ sysstat-collect.service - system activity accounting tool
     Loaded: loaded (/usr/lib/systemd/system/sysstat-collect.service; static)
    Drop-In: /usr/lib/systemd/system/service.d
             └─10-timeout-abort.conf
     Active: inactive (dead) since Tue 2026-04-07 13:30:02 CST; 6min ago
 Invocation: fb53c5b0c2f24719af7d3629842ef372
TriggeredBy: ● sysstat-collect.timer
       Docs: man:sa1(8)
    Process: 2580619 ExecStart=/usr/lib64/sa/sa1 1 1 (code=exited, status=0/SUCCE>
   Main PID: 2580619 (code=exited, status=0/SUCCESS)
   Mem peak: 1.2M
        CPU: 9ms
```


### iostat
<!-- 9358447c-fc9a-4505-a80b-8193035738b5 -->

iostat 从 /sys/block/nvme0n1/stat 中获取的:
 Documentation/admin-guide/iostats.rst

执行一次 iostat 的结果似乎不对，使用 iostat -xz 1
```txt
🤒  cat /sys/block/nvme0n1/stat
  250304    54867 17676331    44341   420980   730997 14007220  1718258        0   733693  1910688        0        0        0        0    88034   148089
```

/sys/block/loop4/stat


docs/kernel/sysfs-blk.md

#### 通过 iostat -xz 1 可以看到 D2C 的 io 的延迟

这个数据是从 /proc/diskstats 中导出的吗？
或者说，D2C 这个字段也是在  /proc/diskstats 中的?

#### 看看 util 的含义

```txt
              %util  Percentage of elapsed time during which I/O requests were issued  to  the  device
                     (bandwidth  utilization for the device). Device saturation occurs when this value
                     is close to 100% for devices serving requests serially.  But for devices  serving
                     requests  in  parallel, such as RAID arrays and modern SSDs, this number does not
                     reflect their performance limits.
```

# pidstat

- pidstat 1 : 展示系统中正在运行的程序的状态
```txt
Average:      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
Average:        0         9    0.00    0.33    0.00    0.00    0.33     -  kworker/0:1-events
Average:        0       969    0.00    0.33    0.00    0.00    0.33     -  irq/111-nvidia
Average:        0      1560    0.00    0.33    0.00    0.00    0.33     -  ovs-vswitchd
Average:        0      2020    0.00    0.33    0.00    0.00    0.33     -  .tailscaled-wra
Average:     1000      2189    0.00    0.33    0.00    0.00    0.33     -  .fcitx5-wrapped
Average:     1000      2239    0.33    0.00    0.00    0.00    0.33     -  .gnome-shell-wr
Average:     1000      5447    0.33    0.00    0.00    0.00    0.33     -  nvim
Average:     1000      5772    1.67    0.67    0.00    0.00    2.33     -  wezterm-gui
Average:     1000      5891    0.33    0.33    0.00    0.00    0.67     -  chrome
Average:     1000      5936    0.33    0.33    0.00    0.00    0.67     -  chrome
Average:     1000      6548    1.00    1.00    0.00    0.00    2.00     -  chrome
Average:     1000      7331    1.33    0.00   28.00    0.00   29.33     -  qemu-system-x86
Average:     1000      8552    0.33    0.33    0.00    0.00    0.67     -  chrome
Average:     1000      8603    0.33    0.33    0.00    0.00    0.67     -  chrome
Average:     1000      9717    0.33    0.00    0.00    0.00    0.33     -  zellij
Average:     1000     12286    0.00    0.67    0.00    0.00    0.67     -  pidstat
```
- 可以只是监控一个程序

# mpstat
<!-- 5019a428-217a-4628-9ad9-918ea0555e62 -->

- 使用 -P 可以控制哪些 CPU 的展示，这个方法可以解决 top 1 无法展示所有的 CPU 的问题
  - mpstat -P ALL 1
  - mpstat -P 0-3 1
  - mpstat -P 0,1,3 1
- mpstat 1
  - 展示整个系统的

```txt
20时14分19秒  CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
20时14分20秒  all    0.00    0.00    0.13    0.00    0.00    0.00    0.00    0.00    0.00   99.87
20时14分21秒  all    0.37    0.00    0.34    0.06    0.00    0.03    0.00    0.00    0.00   99.19
20时14分22秒  all    0.25    0.00    0.16    0.00    0.00    0.00    0.00    0.00    0.00   99.59
20时14分23秒  all    0.19    0.00    0.09    0.03    0.00    0.00    0.00    0.00    0.00   99.69
20时14分24秒  all    0.38    0.00    0.19    0.00    0.00    0.00    0.00    0.03    0.00   99.41
20时14分25秒  all    0.06    0.00    0.13    0.03    0.00    0.00    0.00    0.00    0.00   99.78
20时14分26秒  all    0.19    0.00    0.16    0.00    0.00    0.00    0.00    0.00    0.00   99.66
20时14分27秒  all    0.37    0.00    0.19    0.03    0.00    0.00    0.00    0.00    0.00   99.41
20时14分28秒  all    0.19    0.00    0.19    0.03    0.00    0.00    0.00    0.09    0.00   99.50
20时14分29秒  all    1.41    0.00    0.50    0.19    0.00    0.00    0.00    0.31    0.00   97.53
```

# util-linux
- https://en.wikipedia.org/wiki/Util-linux

## ipcs
- ipcs : 展示 IPC 机制的信息

## irqtop


## htop
<!-- 81f24b00-b755-4ae3-8e73-ed320e5251ad -->
- [htop](https://peteris.rocks/blog/htop/)

- [ ] htop 中，如何控制 CPU 条的大小，在核心很多的位置上，这个 CPU 条被严重压缩了。

- checksheet : https://wangchujiang.com/reference/docs/htop.html

- [htop top 观测 cpu](https://unix.stackexchange.com/questions/349908/how-to-get-all-processes-running-on-each-cpu-core-in-ubuntu)
  - 用 top 观察更加方便，htop 观察需要设置半天

### 操作
- u : 按照 user 显示
- k : 给 process 发送信号
- l : 显示打开的文件
- s : 对于 process attach 上 strace
- Space : 把当前所在行的进程进行标记，U 则是取消标记
- K : 显示 kernel thread
- h : help

- H : 没有看懂 man

### 界面
- VIRT 是虚拟内存大小
- RES 是实际使用的物理内存大小
- SHR 是使用的共享页的大小


## pstack
- /proc/self/stack 检查一个进程在内核中的 stack

https://github.com/mpercy/pstack

## lshw
- lshw -c disk : 可以查看一个 disk 所在的 CPU
## lscpu
## numactl

## time
- time 的源码: https://savannah.gnu.org/git/?group=time
- 系统调用的: https://man7.org/linux/man-pages/man2/getrusage.2.html
通过 `vtime_guest_enter` 可以理解 qemu 在运行起来的时候，发现 user 是占据大多数的，因为统计将 non-root 中的运行统计到 user 中了。

## lsof

https://github.com/lsof-org/lsof

- 一个进程 cd bin
- 然后 lsof bin 可以看到这个内容:
```txt
~ lsof bin
COMMAND  PID USER   FD   TYPE DEVICE SIZE/OFF    NODE NAME
zsh     1582 root  cwd    DIR  253,3     4096 3016181 bin
sleep   1787 root  cwd    DIR  253,3     4096 3016181 bin
```

```sh
strace -t -e trace=file lsof bin
```
只是访问 /proc/pdi/fd 和 /proc/pdi/fdinfo 而已

## iotop
https://github.com/Tomas-M/iotop

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
