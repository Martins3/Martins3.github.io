# tty2 如何关联上 gnome 的

```txt
ps -lt tty2

F S   UID     PID    PPID  C PRI  NI ADDR SZ WCHAN  TTY          TIME CMD
4 S  1000   18011   17320  0  80   0 - 41767 poll_s tty2     00:00:00 gdm-wayland-ses
0 S  1000   18044   18011  0  80   0 - 44201 poll_s tty2     00:00:00 gnome-session-i
```

## codex
有，而且我在当前机器上查到了实际证据：打开的设备文件、键盘模式、设备所有权，以及 GNOME Shell 在 logind 中的控制关系。 这些都不用切换桌面就能观察到。

不过你的感觉也有依据：现代 GNOME 的文字、窗口、鼠标事件，大部分确实不经过 tty2 的字符读写接口。

1. GDM 的会话进程，确实打开着 /dev/tty2。

当前机器的 gdm-wayland-session PID 是 18011。查看它的文件描述符：

ls -l /proc/18011/fd

实际结果中有：

0 -> /dev/tty2
1 -> socket:[52826]
2 -> socket:[52827]

也就是说，tty2 是它真实持有的文件描述符，而且被放在标准输入的位置。

这能在 GDM 源码中对应起来：daemon/gdm-session-worker.c 的 gdm_session_worker_start_session() 把终端 fd 放到标准输入，再通过 setsid() 和 TIOCSCTTY
建立控制终端关系。保留这个 fd 的目的包括保持终端打开，并不意味着 GNOME 从标准输入读取你的键盘操作。GDM 源码
(https://github.com/GNOME/gdm/blob/main/daemon/gdm-session-worker.c)

2. 为了运行图形会话，tty2 的键盘处理模式被改变了。

我执行：

kbd_mode -C /dev/tty2

实际输出：

The keyboard is in Disabled mode, perhaps you are using a graphical environment?

这里的 Disabled 指 tty2 的内核控制台键盘输入路径被关闭了。物理键盘仍然工作，GNOME 通过输入设备接口获取事件。

这是一项实质性的协调：图形会话接管输入后，避免内核文本终端同时处理这些按键。

对应的实现是 systemd 的 src/login/logind-session.c 中 session_prepare_vt()：

ioctl(vt, KDSKBMODE, K_OFF);
ioctl(vt, KDSETMODE, KD_GRAPHICS);

它同时关闭控制台键盘输入，并将 VT 设为图形模式；退出控制关系时还有恢复终端状态的路径。systemd 源码
(https://github.com/systemd/systemd/blob/main/src/login/logind-session.c)

3. tty2 的设备所有权也发生了变化。

当前机器上：

/dev/tty2 owner=martins3 group=tty
/dev/tty3 owner=root    group=tty

tty2 已经属于你的用户。上面的 session_prepare_vt() 就有 fchown()，把会话所用 VT 的所有者设为该用户。这是文件系统层面可以直接观察的联系。

4. logind 中可以串出“tty2 → 会话 → GNOME Shell → 显卡和输入设备”的完整关系。

当前 /run/systemd/sessions/3 中的实际记录包含：

TYPE=wayland
TTY=tty2
VTNR=2
CONTROLLER=:1.128
DEVICES=13:70 226:1 ...

这里的 CONTROLLER 是控制这个会话的 D-Bus 连接。我继续查询连接 :1.128，得到 PID 18091，恰好就是当前的 gnome-shell。

设备号也能对应起来：

226:1 → /dev/dri/card1
13:70 → /dev/input/event6

于是，这台机器上实际存在的关系是：

logind 会话 3
    ├── 关联的虚拟终端：tty2
    ├── 会话类型：Wayland
    ├── 会话控制者：gnome-shell，PID 18091
    └── 管理的设备：显卡、键盘/鼠标等输入设备

Mutter 的 meta_launcher_take_control() 会向 logind 调用 TakeControl，对应的就是这项会话控制关系。Mutter 源码
(https://github.com/GNOME/mutter/blob/main/src/backends/meta-launcher.c)

还有一个特别能解释你困惑的事实：当前 gnome-shell 的 ps TTY 列确实是 ?。 它本身没有控制终端，但仍然通过 logind 控制着关联 tty2 的图形会话。因此，理解这
层联系时，要看会话、终端模式和设备管理；仅看 /dev/tty2 的字符输入输出，确实看不到桌面的主要工作。

### 2026-10-02

只能说，只能意会了，对于 gnome 完全不懂。

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
