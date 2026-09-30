# 我所知道 systemd 的全部

## 推荐教程
- Introduction to systemd Basics ⭐
	- https://documentation.suse.com/en-us/sle-micro/6.0/html/Micro-systemd-basics/index.html#best-practices
- ruanyf 的教程
	- https://www.ruanyifeng.com/blog/2016/03/systemd-tutorial-commands.html
- https://blog.k8s.li/systemd.html

## 官方文档
- https://www.freedesktop.org/wiki/Software/systemd/
- https://systemd.io/

## 常用命令
- 查看依赖
  - sudo systemctl list-dependencies
  - https://serverfault.com/questions/617398/is-there-a-way-to-see-the-execution-tree-of-systemd
- 检查日志
  - https://unix.stackexchange.com/questions/20399/view-stdout-stderr-of-systemd-service
    - sudo journalctl -u [unitfile]

## 各种 init
- https:/github.com/troglobit/finit : Finit is a simple alternative to SysV init and systemd.
- https://github.com/krallin/tini : A tiny but valid init for containers

## 原理上的疑惑
- [ ] D-bus 的工作原理
	- https://news.ycombinator.com/item?id=46278857
- [ ] 如何和 cgroup 交互
- [ ] /var 下的 journal 到底是谁生成的，是 systemd 管理的吗?

## 不要在参数上引入多余的双引号

```sh
cat /etc/systemd/system/hugepage.service

[Unit]
Description=A simple echo

[Service]
Type=oneshot
ExecStart="/bin/echo 1000"
TimeoutStopSec=10
KillMode=process

[Install]
WantedBy=multi-user.target
```

如果使用 pipeline 等复杂的 shell 操作，应该使用上 /bin/sh -c "cmd"

## rc.local

不要使用 rc.local [^1]

如果非要使用，记得
```sh
chmod +x /etc/rc.d/rc.local
```




- [ ] 为什么内核参数可以管理 systemd

systemctl list-units --type=service

- [ ] TimeoutStopSec=10 似乎没用

## 问题
启动这个服务实际上会等待 10s 的:
```sh
[Unit]
Description=MountSmokeScreen

[Service]
Type=oneshot
ExecStart=/bin/sleep 10
TimeoutStopSec=1

[Install]
WantedBy=multi-user.target
```

- [ ] 为什么 systemd 挂掉之后，reboot 也不能正常使用了。

## [ ] 没有理解 target 和 wanted by 是什么意思

## [ ] 到底是守护进程还是一个 oneshot 的似乎是存在区别的

## [ ] WantedBy 和 RequiredBy 的区别是什么？

## [ ] service 的 Type 是 dbus 该如何理解

### 如果是检测另一个 sytemd 进程

reboot.sh 中增加
```sh
if [[ $(/bin/systemctl is-active caixukun) == "active" ]];then
reboot
fi
```

在 reboot.service 中增加:
```sh
After=cauxukun.service
```

## oneshot vs simple
https://trstringer.com/simple-vs-oneshot-systemd-service/

## systemd 的 before after 应该是不能实现只有 exit = 0 才可以继续的操作
但是，为什么曾经见过磁盘服务没有启动，然后后面都没有启动的情况

## systemd unit file 的位置
- sys: /etc/systemd/system
- user: /etc/systemd/user or $HOME/.config/systemd/user

## journalctl
- https://www.loggly.com/ultimate-guide/using-journalctl/
- https://unix.stackexchange.com/questions/139513/how-to-clear-journalctl

### 显示所有的 dmesg 信息
- journalctl -t kernel : 所有的 kernel 日志
- journalctl -k : 这一次
- journalctl --boot=-1 -k : 上一次的 kernel 日志

### 打开持久化的 journal
有的机器默认是没有持久化的
```txt
mkdir -p /var/log/journal
systemctl restart systemd-journald.service
```
接下来，检查 /var/log/journal 中是否存在对应的数据。

## https://systemd-by-example.com/

## [ ] 到底是直接执行脚本还是需要借助 bash

不知道为什么，遇到了这个错误:
Failed at step EXEC spawning /root/IPTV/xteve: Permission denied

```txt
ExecStart=bash /root/share/pkg/main.sh
```

## 等待网络的方法
- network-online.target

## 常见疑问
- https://unix.stackexchange.com/questions/506347/why-do-most-systemd-examples-contain-wantedby-multi-user-target

## TODO
https://unix.stackexchange.com/questions/503679/systemd-unit-file-wantedby-and-after

[^1]: https://unix.stackexchange.com/questions/471824/what-is-the-correct-substitute-for-rc-local-in-systemd-instead-of-re-creating-rc
[^2]: https://support.huaweicloud.com/intl/en-us/trouble-ecs/ecs_trouble_0349.html


## 调查下 systemd 的 log

ubuntu 和 centos 上都是用的 rsyslog

sudo systemctl status rsyslog.service


- rsyslog 和 journald 关系:
  - https://serverfault.com/questions/959982/is-rsyslog-redundant-on-when-using-journald

> rsyslog is redundant if journald persistent storage is enabled and there are no applications that depend on the specific files and format produced by rsyslog, the content is the same.

也就是说，rsyslog 只是

- journald 和 journalctl 的关系:
  - https://www.loggly.com/ultimate-guide/using-journalctl

> Journalctl is a utility for querying and displaying logs from journald, systemd’s logging service. Since journald stores log data in a binary format instead of a plaintext format, journalctl is the standard way of reading log messages processed by journald.


## 常用命令
- systemctl --user list-timers --all
- systemctl list-timers --all
- systemctl --failed

## systmed 的 disable 不会将将程序杀掉
```txt
[root@localhost 16:34:12 ~]$ systemctl disable chronyd.service
Removed /etc/systemd/system/multi-user.target.wants/chronyd.service.
[root@localhost 16:34:18 ~]$ systemctl status chronyd.service
● chronyd.service - NTP client/server
   Loaded: loaded (/usr/lib/systemd/system/chronyd.service; disabled; vendor preset: enabled)
   Active: active (running) since Wed 2024-12-11 16:28:23 CST; 6min ago
     Docs: man:chronyd(8)
           man:chrony.conf(5)
 Main PID: 998 (chronyd)
    Tasks: 1
   Memory: 952.0K
   CGroup: /system.slice/chronyd.service
           └─998 /usr/sbin/chronyd

Dec 11 16:28:23 localhost.localdomain systemd[1]: Starting NTP client/server...
Dec 11 16:28:23 localhost.localdomain chronyd[998]: chronyd version 3.5 starting (+CMDMON +NTP +REFCLOCK +RTC +PRIVDROP >
Dec 11 16:28:23 localhost.localdomain systemd[1]: Started NTP client/server.
Dec 11 16:28:57 localhost.localdomain chronyd[998]: Selected source 202.112.29.82
Dec 11 16:28:59 localhost.localdomain chronyd[998]: Source 162.159.200.123 replaced with 103.147.22.149
```
- systemctl list-unit-files

## 看看 systemd 和 udev 的关系

## 有趣的报告
https://mp.weixin.qq.com/s/DFWE7b2AOhw1IBZaqTTkJQ

systemd 是一套大约 150 个独立的二进制文件

他还讨论了 systemd 在代码行数和依赖项方面的占用空间。他说，该项目包含 690,000 行代码，而 wpa_supplicant 大约有 460,000 行代码，GNU C 库 (GNU C library, glibc) 超过 140 万行。

Poettering 说，在 Fedora 上完全安装 systemd 大约需要 36MB，而 GNU Bash 大约需要 8MB。他说，如果仅 shell 就需要 8MB，“那么” systemd 需要 36MB “也没那么糟糕”。

目标二是重新思考 systemd 的进程间通信 (interprocess communication, IPC)，特别是从 D-Bus 转向 varlink。

Poettering 有时间回答几个问题。第一个问题是 systemd 最终是否会取代 GRUB。Poettering 说，正如你可能猜到的那样，他“不相信 GRUB”，并且替换它的所有部件都已就绪。剩下的问题是政治性的。他说，GRUB 试图做太多事情，而这些事情中的大多数都是错误的。如果仅关注 EFI，则大多数发行版都可以切换到 systemd-boot。

## systemd-boot
https://wiki.archlinux.org/title/Systemd-boot#Choosing_next_boot

## 直接进入到 emergency mode 中
systemd.unit=desired.target

## systemd 提供的常用工具
<!-- 2e795870-837d-46ef-908d-a41fa109f9fa -->

参考: https://www.zhihu.com/question/525825165/answer/2422924492

不得不说，仅仅会使用这些东西就是对于 Linux 的使用很熟练了:

在 fedora 系统中，这些基本上都是可以用的:

| ctl            | 介绍                   | 额外说明                                                     |
|----------------|------------------------|--------------------------------------------------------------|
| homectl        | Home 目录管理          |                                                              |
| hostnamectl    | hostname 配置          |                                                              |
| journalctl     | 日志                   |                                                              |
| localectl      | locale 和键盘布局配置  |                                                              |
| machinectl     | 虚拟机、容器管理       | 需要安装 systemd-container 才会有                            |
| systemctl      | 服务管理               |                                                              |
| timedatectl    | 时间管理               |                                                              |
| udevadm        | udev 管理              |                                                              |
| userdbctl      | 用户管理               |                                                              |
| portablectl    | 可移植服务镜像配置     | 配合  systemd-portabled.service 使用，似乎用于 coreos 之类的 |
| coredumpctl    | coredump 处理          |                                                              |
| busctl         | D-Bus 监控             | D-BUS 到底是什么东西?                                        |
| kernel-install | 内核和 initramfs 管理  |                                                              |
| networkctl     | 网络管理               | 和 network-manager 什么关系                                  |
| oomctl         | OOM 配置               | 用于配合 systemd-oomd.service 的工作                         |
| bootctl        | 管理 EFI 和 bootloader |                                                              |
| loginctl       | login manager 配置     |                                                              |

后面的那几个真的很有用:

https://man.archlinux.org/man/portablectl.1.en

sudo systemd-repart : 磁盘管理的

## 这个东西
https://news.ycombinator.com/item?id=43899236

## 为什么有人需要使用 systemd-run 这个命令?
systemd-run --user -qt -p PrivateUsers=yes ls

systemd-run --user -qt -p PrivateUsers=yes env

## rsyslog 和 systemd-journald 的关系
<!-- b99f6f88-c118-4f58-b102-a714a026ba0f -->

一般系统中都是可以同时打开:
systemctl status rsyslog.service
systemctl status systemd-journald

https://serverfault.com/questions/959982/is-rsyslog-redundant-on-when-using-journald

> Based on this I would say that rsyslog is redundant if journald persistent storage
> is enabled and there are no applications that depend on the specific files and
> format produced by rsyslog, the content is the same.

简而言之就是，journald 可以替代 rsyslog ，但是没有

rsyslog 的配置文件在 : /etc/rsyslog.conf

> [!NOTE]
> 参考 Deepseeek ，有待验证

二进制 journal（journalctl 查看）
文本日志文件（如 /var/log/messages，由 rsyslog 写入）

在大多数现代 Linux 发行版（包括 Fedora）中：

systemd-journald 始终运行，作为第一层日志收集器。
rsyslog 作为第二层日志处理器，通过监听 journald 提供的 syslog socket（通常是 /run/systemd/journal/syslog）来获取日志副本。

(最后一个问题， 把 rsyslog disable 掉也完全不影响生活吗?)

## systemd 会自动记录 fork 的 process

```txt
 sudo systemctl status crond
● crond.service - Command Scheduler
     Loaded: loaded (/usr/lib/systemd/system/crond.service; enabled; preset: enabled)
     Active: active (running) since Thu 2025-06-12 11:09:16 CST; 43min ago
   Main PID: 1469 (crond)
      Tasks: 4 (limit: 47365)
     Memory: 2.8M (peak: 3.3M)
        CPU: 20ms
     CGroup: /system.slice/crond.service
             ├─1469 /usr/sbin/crond -n
             ├─4484 /usr/sbin/CROND -n
             ├─4489 bash /home/martins3/test.sh
             └─4491 sleep 1000000

Jun 12 11:09:16 localhost.localdomain systemd[1]: Started Command Scheduler.
Jun 12 11:09:16 localhost.localdomain crond[1469]: (CRON) STARTUP (1.6.1)
Jun 12 11:09:16 localhost.localdomain crond[1469]: (CRON) INFO (Syslog will be used instead of sendmail.)
Jun 12 11:09:16 localhost.localdomain crond[1469]: (CRON) INFO (RANDOM_DELAY will be scaled with factor >
Jun 12 11:09:16 localhost.localdomain crond[1469]: (CRON) INFO (running with inotify support)
Jun 12 11:51:01 localhost.localdomain CROND[4112]: (root) CMD (/home/martins3/test.sh)
Jun 12 11:51:01 localhost.localdomain CROND[4107]: (root) CMDEND (/home/martins3/test.sh)
Jun 12 11:52:01 localhost.localdomain CROND[4489]: (root) CMD (/home/martins3/test.sh)
```

## systemd-analyze blame

13900k fedora 物理机中的结果是这样的，这合理吗?
```txt
1min 57ms NetworkManager-wait-online.service
  18.831s plymouth-read-write.service
   9.985s fstrim.service
   9.959s sys-module-configfs.device
   7.959s dev-disk-by\x2did-nvme\x2dFanxiang_S790_4TB_FXS790233391302_1\x2dpart3.device
   7.959s dev-disk-by\x2dpath-pci\x2d0000:06:00.0\x2dnvme\x2d1\x2dpart3.device
   7.959s dev-disk-by\x2dpath-pci\x2d0000:06:00.0\x2dnvme\x2d1\x2dpart-by\x2dpartuuid-4c0dd748\x2d2a01\x2d4>
   7.959s dev-nvme0n1p3.device
   7.959s sys-devices-pci0000:00-0000:00:1d.0-0000:06:00.0-nvme-nvme0-nvme0n1-nvme0n1p3.device
   7.959s dev-disk-by\x2dpath-pci\x2d0000:06:00.0\x2dnvme\x2d1\x2dpart-by\x2dpartnum-3.device
   7.959s dev-disk-by\x2did-nvme\x2dnvme.1e4b\x2d465853373930323333333931333032\x2d46616e7869616e6720533739>
   7.959s dev-disk-by\x2dpartuuid-4c0dd748\x2d2a01\x2d4ec7\x2d8ea3\x2def4846b3891a.device
   7.959s dev-disk-by\x2did-nvme\x2dFanxiang_S790_4TB_FXS790233391302\x2dpart3.device
   7.959s dev-disk-by\x2ddiskseq-3\x2dpart3.device
```
- sys-module-configfs.device 是什么鬼?
- plymouth-read-write.service 是什么鬼?


## 文档
Highlights from systemd v258: part one
https://mp.weixin.qq.com/s/_bRapMml6V6tezb94a45pg

https://documentation.suse.com/zh-cn/sles/15-SP7/html/SLES-all/cha-systemd.html
https://www.ruanyifeng.com/blog/2016/03/systemd-tutorial-commands.html

```txt
重启系统
sudo systemctl reboot

关闭系统，切断电源
sudo systemctl poweroff

CPU停止工作
sudo systemctl halt

暂停系统
sudo systemctl suspend

让系统进入冬眠状态
sudo systemctl hibernate

让系统进入交互式休眠状态
sudo systemctl hybrid-sleep

启动进入救援状态（单用户状态）
sudo systemctl rescue
```

## systemd user session 缺失问题

虽然 `systemd-logind` 服务在运行：

```bash
$ systemctl status systemd-logind
Active: active (running)
```

```bash
# 检查环境变量为空，正常输出
$ echo $XDG_RUNTIME_DIR
/run/user/1000

# user 服务异常
$ systemctl --user status
```

### 原理

`/sbin/init` 是 全局服务、系统资源 提供 system dbus
`systemd --user`（每个用户一个）  用户自己的会话、应用 提供 user session dbus |

`systemd-run` 是 systemd 提供的工具，用来在临时 unit 中运行命令。

- `systemd-run`（不带 `--user`）→ 直接跟 PID 1 对话，不需要 dbus 会话变量，但需要 root 权限
- `systemd-run --user` → 跟用户自己的 systemd 实例对话，必须有 dbus 连接，但不需要 root

SSH 正常登录流程
```
SSH 登录
  → sshd
    → PAM (pam_systemd.so)
      → systemd-logind 创建 session
        → 启动 systemd --user
          → 设置 XDG_RUNTIME_DIR=/run/user/<uid>
            → 设置 DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/<uid>/bus
```

`pam_systemd.so` 是关键一环。如果 PAM 配置里缺了它，后面整条链都断了。

### 解决办法
1. 安装 systemd-pam
```bash
sudo dnf install systemd-pam
```

2. 修改
在 `/etc/pam.d/sshd` 的 `session` 区段添加：

```bash
session    optional     pam_systemd.so
```

修改后的 `/etc/pam.d/sshd` 示例：

```
#%PAM-1.0
auth       substack     password-auth
auth       include      postlogin
account    required     pam_sepermit.so
account    required     pam_nologin.so
account    include      password-auth
password   include      password-auth
session    required     pam_selinux.so close
session    required     pam_loginuid.so
session    required     pam_selinux.so open env_params
session    required     pam_namespace.so
session    optional     pam_keyinit.so force revoke
session    optional     pam_motd.so
session    include      password-auth
session    include      postlogin
session    optional     pam_systemd.so
```

然后退出 ssh

## systemctl 来控制 cgroup 内存使用量
<!-- 4271eee5-dc29-4983-8617-55330f6ccb4d -->

```sh
sudo systemctl set-property abc.service MemoryLimit=8G
systemctl show abc.service -p MemoryLimit
cat /sys/fs/cgroup/memory/abc.service/memory.limit_in_bytes
```

## 利用 systemd-run 来控制 CPU 的使用量
本机是 cgroup v2 + systemd（32 核），所以最精确的做法不是调 -j 参数，而
是给整个 make 进程树加一个 CPU 配额上限：80% × 32 = 25.6 个核，即每秒钟
最多消耗 25.6 CPU·秒。

```bash
systemd-run --scope -p "CPUQuota=$(( $(nproc) * 80 ))%" make -j32
```

## 常用命令
systemctl cat getty@.service

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
