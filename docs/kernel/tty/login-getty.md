# login 和 getty

## 基本流程是什么?
getty 是 Linux 中管理终端登录入口的程序。你看到这样的提示时，通常就是它在等待用户名：

Ubuntu 24.04 tty1

hostname login:

名字来自 get teletype，其中 teletype 指早期的电传打字终端。

它主要做三件事：
1. 打开并初始化一个终端，例如本地虚拟控制台 /dev/tty1 或串口 /dev/ttyS0。
2. 显示 login: 提示并读取用户名。
3. 启动 login 程序，由 login 负责密码认证，成功后启动用户的 shell。

所以，基本上可以想到，这是很简单的程序了，
对于一个 serial 设备，Linux 内核会保留出来一个 /dev/tty1 ，
systemd 会 fork 出来一个程序，这个程序会来 open /dev/tty1 ，并且持续的
做成相应，这个程序可以是 bash ，但是最开始的时候可以是 getty

## 源代码的位置

常见 Linux 发行版中，它们的源码归属是：

 程序/组件                                所属项目                                     作用
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 agetty                                   util-linux，term-utils/agetty.c              初始化终端、读取用户名、执行 login
───────────────────────────────────────  ───────────────────────────────────────────  ────────────────────────────────────
 login                                    通常是 util-linux，login-utils/login.c；     认证用户、建立会话、启动 shell 部分发行版使用 shadow 的实现
───────────────────────────────────────  ───────────────────────────────────────────  ────────────────────────────────────
 getty@.service、serial-getty@.service    systemd                                      定义如何启动和管理 agetty

因此，前面的调用链表示的是运行时关系，不是源码归属：

```txt
systemd                  agetty                 login
启动并监督服务     →      准备终端、读用户名  →   认证并启动 shell
[systemd 项目]           [util-linux 项目]      [util-linux 或 shadow]
```

login 通常还会借助 PAM 完成认证，PAM 又是独立的项目。


## 这个错误意味着什么?
```txt
Mar 26 19:51:27 ensemble systemd: getty@tty1.service has no holdoff time, scheduling restart.
Mar 26 19:51:27 ensemble systemd: Stop job pending for unit, delaying automatic restart.
```
## systemd 的管理

```txt
🧀  systemctl status getty@tty1.service
● getty@tty1.service - Getty on tty1
     Loaded: loaded (/etc/systemd/system/getty@.service; disabled; preset: ignored)
    Drop-In: /nix/store/fbfgn7937plaicb7lpkirnk2jvpyk7w4-system-units/getty@.service.d
             └─overrides.conf
     Active: active (running) since Mon 2025-03-24 14:42:33 CST; 1 week 0 days ago
 Invocation: 067022400a4845c7836d599345bb56a7
       Docs: man:agetty(8)
             man:systemd-getty-generator(8)
             https://0pointer.de/blog/projects/serial-console.html
   Main PID: 2021 (agetty)
         IP: 0B in, 0B out
         IO: 268K read, 0B written
      Tasks: 1 (limit: 76716)
     Memory: 524K (peak: 1.9M)
        CPU: 12ms
     CGroup: /system.slice/system-getty.slice/getty@tty1.service
             └─2021 /nix/store/62camdd58i8lfsv26wagljp1nqvvs4la-util-linux-2.39.4-bin/bin/agetty --login-program /nix/store/yr>
```

systemd 会自动为每一个 tty 都创建出来，也就是:
```txt
ps -elf | grep getty
4 S root        1350       1  0  80   0 -  1333 -      Sep15 tty1     00:00:00 /sbin/agetty -o -- \u --noreset --noclear - linux
4 S root        1351       1  0  80   0 -  1975 -      Sep15 hvc0     00:00:00 /sbin/agetty -o -- \u --noreset --noclear --keep-baud 115200,57600,38400,9600 - vt220
4 S root        1352       1  0  80   0 -  1975 -      Sep15 ttyS0    00:00:00 /sbin/agetty -o -- \u --noreset --noclear --keep-baud 115200,57600,38400,9600 - vt220
```

## 案例


### collei virtme 模式


启动 shell 并不必须经过 getty。
Collei 的 virtme init 自己完成了终端和会话设置，再启动指定用户的 shell，因此可以省掉 getty。

普通 Linux 控制台的登录过程大致是：

init / systemd
    └─ getty：打开终端、设置终端参数、显示 login:、读取用户名
         └─ login：验证密码、设置用户身份和环境
              └─ shell：读取并执行命令

以常见的 agetty 为例，它负责打开 tty、配置串口速率或回显等终端属性、询问用户名，然后调用 /bin/login。密码认证通常由
login/PAM 完成，getty 自己不负责执行用户命令。 agetty 手册 (https://man7.org/linux/man-pages/man8/agetty.8.html)

你当前 Collei 的 virtme 启动路径是：

内核执行 initramfs 中的 /init
    → 挂载 virtiofs ROOTFS
    → switch_root，运行 Rust init，仍为 PID 1
        └─ 设置终端和会话
             └─ su 指定用户
                  └─ shell

collei 建立终端的核心操作是:

```txt
setsid();                     // 创建新 session
dup2(tty_fd, STDIN_FILENO);    // 标准输入接到终端
ioctl(0, TIOCSCTTY, 1);        // 将终端设为当前 session 的控制终端
dup2(tty_fd, STDOUT_FILENO);   // 标准输出接到终端
dup2(tty_fd, STDERR_FILENO);   // 标准错误接到终端
```

“标准输入输出连到 tty”和“拥有控制终端”是两件事。
前者让 shell 能读写字符；
后者配合前台进程组和终端设置，让 shell 的作业控制、Ctrl-C、Ctrl-Z 等正常工作。
TIOCSCTTY 就是建立控制终端关系的接口。TIOCSCTTY 手册(https://man7.org/linux/man-pages/man2/TIOCSCTTY.2const.html)

### 将 bash 重新替换为 agetty

如果 ssh 到虚拟机中:
```txt
exec sudo agetty --noclear --noreset - "$TERM"

[sudo] password for martins3:

Fedora Linux 42 (Server Edition)
Kernel 6.14.0-63.fc42.x86_64 on x86_64 (pts/1)

Web console: https://nix-init:9090/ or https://10.0.2.15:9090/

nix-init login:
```
不过这个模式，一旦输入密码，ssh connection 就断开了。

如果启动 qmeu 后，直接在 stdio 中模拟的串口中测试，就是完全回到

```txt
Fedora Linux 42 (Server Edition)
Kernel 6.14.0-63.fc42.x86_64 on x86_64 (hvc0)

Web console: https://nix-init:9090/ or https://10.0.2.15:9090/

nix-init login: martins3
Password:
Last login: Wed Sep 23 15:53:28 on hvc0

~ 🦇
🧀  exec sudo agetty --noclear --noreset - "$TERM"

[sudo] password for martins3:

Fedora Linux 42 (Server Edition)
Kernel 6.14.0-63.fc42.x86_64 on x86_64 (pts/0)

Web console: https://nix-init:9090/ or https://10.0.2.15:9090/

nix-init login: martins3 <- 不知道为什么，第一次总是失败

Fedora Linux 42 (Server Edition)
Kernel 6.14.0-63.fc42.x86_64 on x86_64 (hvc0)

Web console: https://nix-init:9090/ or https://10.0.2.15:9090/

nix-init login: martins3
Password:
Last login: Wed Sep 23 15:53:33 on hvc0 <--- 重新登录回来
~ 🦇
🧀
```

## login

           ├─login(1152)───zsh(2205)
           ├─sshd(1022)───sshd-session(2006)───sshd-session(2037)───zsh(2044)───ps+

分别从两个地方登录

```txt
           ├─login(1184)───zsh(1411)
           ├─login(1185)───zsh(1707)
           ├─sshd(1061)───sshd-session(1195)───sshd-session(1225)───zsh(1232)───pstree(1903)
```

## 在 asahi linux 中启动虚拟机，虚拟机发最后有这个日志

```txt
[   14.912919] fbcon: Taking over console
[   14.913646] Console: switching to colour frame buffer device 160x50

Fedora Linux 42 (Server Edition)
Kernel 6.17.9-200.fc42.aarch64 on aarch64 (ttyAMA0)

Web console: https://localhost:9090/

localhost login:
```

## 那么这个服务是做什么的?
```txt
🧀  sudo systemctl status systemd-vconsole-setup
[sudo] password for martins3:
Sorry, try again.
[sudo] password for martins3:
● systemd-vconsole-setup.service - Virtual Console Setup
     Loaded: loaded (/etc/systemd/system/systemd-vconsole-setup.service; linked; preset: ignored)
     Active: active (exited) since Thu 2025-02-27 14:53:59 CST; 2 days ago
 Invocation: 1b89ee8a1ea74877bb86b6d7f5cb55c9
       Docs: man:systemd-vconsole-setup.service(8)
             man:vconsole.conf(5)
   Main PID: 1061 (code=exited, status=0/SUCCESS)
         IO: 3.1M read, 0B written
   Mem peak: 6.5M
        CPU: 15ms

Feb 27 14:53:59 nixos systemd[1]: Starting Virtual Console Setup...
Feb 27 14:53:59 nixos systemd[1]: Finished Virtual Console Setup.
```

显然 systemd 配置了 启动之后，如果 scsi 没有安装之类的，会发现系统日志会卡到这里，

```txt
[  OK  ] Finished dracut pre-udev hook.
         Starting Rule-based Manager for Device Events and Files...
[  OK  ] Started Rule-based Manager for Device Events and Files.
         Starting Coldplug All udev Devices...
[  OK  ] Finished Coldplug All udev Devices.
[  OK  ] Reached target System Initialization.
[  OK  ] Reached target Basic System.
         Starting dracut initqueue hook...
[  OK  ] Stopped Virtual Console Setup.
         Stopping Virtual Console Setup...
         Starting Virtual Console Setup...
[  OK  ] Finished Virtual Console Setup.
```

  Docs: man:systemd-vconsole-setup.service(8)
        man:vconsole.conf(5)

内核启动的日志中有这个，这个是那个服务来搞的:

```txt
[  OK  ] Started Getty on tty1.
[  OK  ] Started Serial Getty on hvc0.
[  OK  ] Started Serial Getty on ttyS0.
```

## 正确设置 getty

### 问题背景
我希望用，echo 1 > /dev/ttyS1 来判断，当前到底连的是什么串口，
但是我发现只有这次没用

### 解决办法 : codex
最可能的原因是：echo 不会设置串口参数。

你的成功测试顺序是：

1. 把 ttyS1 设置为 115200。
2. 写入 SOL TEST...，SOL 成功收到。
3. 执行恢复旧配置。虽然 stty 报错，但它很可能已经把波特率部分恢复成原来的 9600。

4. 随后的 echo 1 便按照当前波特率发送，与 BMC 的 115200 不匹配。

先确认：

stty -F /dev/ttyS1 speed

然后重新设置并测试短消息：

stty -F /dev/ttyS1 \
    115200 cs8 -cstopb -parenb -crtscts -ixon -ixoff

printf '1\r\n' > /dev/ttyS1

printf '1\r\n' 比 echo 1 更适合串口，因为：

- echo 1 发送的是 1\n。
- 串口终端通常期望 \r\n。
- 只有 \n 时，字符可能出现但光标位置异常。

设置到 115200 后再执行：

echo 1 > /dev/ttyS1

理论上也应看到字符 1，只是换行可能不正常。

为了让 ttyS1 始终保持正确配置，启用 getty：

systemctl enable --now serial-getty@ttyS1.service

它会以 115200,38400,9600 的顺序启动，初始使用 115200。之后 SOL 中
按回车应出现登录提示。下次重启前再把内核参数改成
console=ttyS1,115200n8，这样 kernel panic 也会直接输出到 SOL。

这里的，真的没明白啊:
```txt
systemctl enable --now serial-getty@ttyS1.service
```

## fedora 启动有这个，这是做什么的
```txt
Finished systemd-vconsole-setup.service - Virtual Console Setup.
```


systemd-vconsole-setup 负责配置控制台，agetty 负责在控制台上提供登录入口。 两者使用同一个终端，但职责不同。

             systemd-vconsole-setup.service                 getty@tty1.service 等运行的 agetty
━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 做什么      设置虚拟控制台的键盘映射、字体等               设置终端参数，显示 login:，读取用户名并调用 login
──────────  ─────────────────────────────────────────────  ───────────────────────────────────────────────────
 作用范围    Linux 虚拟控制台，例如 /dev/tty1、/dev/tty2    每个实例对应一个终端，也支持串口
──────────  ─────────────────────────────────────────────  ───────────────────────────────────────────────────
 配置来源    /etc/vconsole.conf 等                          getty 服务定义、agetty 参数等
──────────  ─────────────────────────────────────────────  ───────────────────────────────────────────────────
 生命周期    配置完成后退出                                 等待登录；登录会话结束后，systemd 再启动一个实例

其中，密码认证和登录后的 shell 启动由 login 处理。agetty 官方手册
(https://kernel.googlesource.com/pub/scm/utils/util-linux/util-linux/+/refs/tags/v2.41.1/term-utils/agetty.8.adoc)

本机服务定义中的启动顺序是：

systemd-vconsole-setup.service：设置字体、键盘映射
    ↓ Before=sysinit.target
sysinit.target
    ↓ getty 服务默认 After=sysinit.target
getty@tty1.service：启动 agetty，提供登录入口

vconsole-setup 不会启动 agetty，getty 也没有直接 Requires=systemd-vconsole-setup.service。 前者先配置环境，后者随后使用控制台提供登录。

你看到的：

Active: active (exited)
Main PID: 1137 (code=exited, status=0/SUCCESS)

是正常状态，因为该服务配置了：

Type=oneshot
RemainAfterExit=yes

表示设置已经成功完成，进程已经退出，systemd 仍记住它处于“已完成”的状态。

如果是 serial-getty@ttyS0.service，它同样运行 agetty，但串口终端的字体和键盘通常由另一端负责，因此不依赖这里的虚拟控制台字体、键盘配置。

## 当使用 fedora 的图形界面的时候，进行 logout 之后，原来这个时候，

所有的图形程序都会被 kill 掉的，但是 tmux 和 qemu 之类的东西却不会被 kill 掉，这个时候，重新登录之后，tmux 和 qemu 之类的东西继续在

所以，这么想，logout 之后，相当于当前的 session 中的程序就是那些图形程序?


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
