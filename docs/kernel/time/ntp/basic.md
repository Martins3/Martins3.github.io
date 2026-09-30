## ntp
https://developer.aliyun.com/mirror/NTP

通过 /dev/ptp 调整需要在内核完成吗?

## 几个基本的问题
## 如果时间差别过大，是如何处理
## ntp 的校准是存在范围的吗?
似乎没有，时间变化可以直接是一年的

1. 多个 ntp server 应该相信谁 ?

## guest 的时间是通过什么分析的

看上去，fedora 中默认使用的安装的，但是 ntpdate 需要
yum install ntpdate


```txt
systemctl status chronyd

chronyc tracking           # To get information about the main time reference
chronyc sources -v         # equivalent information to the ntpq
ntpdate pool.ntp.org       # To quickly synchronize a server
```


```txt
Reference ID    : 11FD74FD (twtpe2-ntp-002.aaplimg.com)
Stratum         : 2
Ref time (UTC)  : Sun Jun 02 09:41:50 2024
System time     : 199.635406494 seconds fast of NTP time
Last offset     : +0.000412030 seconds
RMS offset      : 23.699993134 seconds
Frequency       : 500000.000 ppm fast
Residual freq   : +1.278 ppm
Skew            : 40.203 ppm
Root delay      : 0.057455495 seconds
Root dispersion : 0.002696760 seconds
Update interval : 65.5 seconds
Leap status     : Normal
```

## 符合想象，ntp 校准时间是缓慢进行的
https://stackoverflow.com/questions/7497242/get-notified-of-ntp-adjustments


## rust ntp 实现
https://letsencrypt.org/2024/06/24/ntpd-rs-deployment.html#

## 一个简单的实践
https://reintech.io/blog/configuring-time-server-ntp-fedora-38

使用 sudo systemctl status chronyd 来分析

使用 chronyc sources 检查

chronyd 日志
```txt
Jul 25 05:57:41 localhost.localdomain systemd[1]: Starting chronyd.service - NTP client/server...
Jul 25 05:57:41 localhost.localdomain chronyd[7088]: chronyd version 4.5 starting (+CMDMON +NTP +REFCLOCK +RTC +PRIVDROP +SCFILTER>
Jul 25 05:57:41 localhost.localdomain chronyd[7088]: Frequency 0.000 +/- 1000000.000 ppm read from /var/lib/chrony/drift
Jul 25 05:57:41 localhost.localdomain chronyd[7088]: Using right/UTC timezone to obtain leap second data
Jul 25 05:57:41 localhost.localdomain chronyd[7088]: Loaded seccomp filter (level 2)
Jul 25 05:57:41 localhost.localdomain systemd[1]: Started chronyd.service - NTP client/server.
Jul 25 05:57:45 localhost.localdomain chronyd[7088]: Selected source 10.0.101.0
Jul 25 05:57:45 localhost.localdomain chronyd[7088]: System clock wrong by 18221038.001330 seconds
Feb 21 02:21:43 localhost.localdomain chronyd[7088]: System clock was stepped by 18221038.001330 seconds
Feb 21 02:21:43 localhost.localdomain chronyd[7088]: System clock TAI offset set to 37 seconds
```
配置文件 /etc/chrony.conf

在 10.0.0.5 中:
```txt
server 10.0.101.0 iburst
```
在 10.0.101.0 中:
```txt
allow 10.0.0.5/16
```

## ntp 的基本分工
<!-- 5f601d0f-8fc7-411c-beee-9be86ab2f64f -->

(一字不动从 codex 中粘贴过来的，我认为这一段说的很好)

NTP 解决的是“系统时间怎么跟外部标准时间对齐”这个问题，但它一般不直接频繁去写硬件 RTC。
Linux 里实际上有两套时钟：

- system clock：内核当前维护的系统时间，CLOCK_REALTIME 的基础，日常所有 gettimeofday/ clock_gettime 主要看它。
- hardware clock / RTC：主板上的实时时钟，掉电靠电池继续走，用来在开机早期给系统一个初始时间。

NTP 的主要工作对象是第一套，也就是 system clock。典型流程是：

1. 用户态 NTP 守护进程从网络上的 NTP 服务器拿到时 间。
2. 它估计本机和标准时间之间的：
    - offset：现在差多少
    - frequency drift：本机时钟跑快还是跑慢
    - delay/jitter：网络测量误差
3. 它通过内核接口，比如 adjtimex() / clock_adjtime()，告诉内核如何“慢慢调”系统时间。
4. 内核里的 timekeeping/NTP PLL-FLL 逻辑负责把这些参数落实成：
    - 立刻 step 时间，或者
    - 更常见地 slewing，轻微改变 tick/frequency，让系统时间逐渐靠拢标准时间。

所以，内核在 NTP 中的核心作用不是“联网对时”，而是“作为被校准的本地时钟控制器”。

Linux kernel 在 NTP 里的角色

可以把职责分开看：

用户态负责：

- 跟远端 NTP 服务器通信
- 做滤波、选源、异常点剔除
- 决定当前 offset/frequency 校正值
- 调用 adjtimex() 把控制量交给内核

内核负责：

- 维护高精度系统时间
- 在每个 tick / hrtimer / clocksource 累积中推进时间
- 根据 NTP 参数修正时间和频率
- 处理 leap second、11-minute mode 等和“时间 discipline”有关的内核机制
- 在需要时把系统时间同步回 RTC

## chronyd / systemd-timesyncd 的关系

nptd 不考虑
chronyd : 完备
systemd-timesyncd :
单源，无多源投票/时钟滤波
容器、最小系统、边缘设备

具体实现在 src/timesync/ 中，一共就 1000 行代码而已

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
