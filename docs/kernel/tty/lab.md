# 实验
## 安装虚拟机的时候，ovmf stdio 也有安装界面

但是 seabios 是没有的，这是为什么?

是 grub 在 seabios 和 uefi 模式下的不同吗?

## 获取当前所在的 tty 上

ps -elf | awk 'NR==1 || $13 != "?"'


终端中的 bash 是 terminal emulater 创建的
```txt
 ghostty─┬─zsh───python3.13───ssh
         ├─zsh───tmux: client
         └─23*[{ghostty}]
```

```txt
|-alacritty-+-zsh---pstree
            `-9*[{alacritty}]
```

这些程序都是连上的用户态的，对应的 server 都是连接上的，

##

cat /sys/class/tty/console/active

## ls -la /proc/self/fd

也是一种方法:

如果 ssh 到虚拟机中:
```txt
🧀  ls -la /proc/self/fd
lrwx------ - martins3 18 Dec 08:24 0 -> /dev/pts/25
lrwx------ - martins3 18 Dec 08:24 1 -> /dev/pts/25
lrwx------ - martins3 18 Dec 08:24 2 -> /dev/pts/25
lr-x------ - martins3 18 Dec 08:24 3 -> /proc/1079992/fd
```

如果直接在 vnc 中:
```txt
🧀  ls -la /proc/self/fd
lrwx------ 1 root root 64 Dec 18 21:27 0 -> /dev/tty1
l-wx------ 1 root root 64 Dec 18 21:27 1 -> /root/tty1
lrwx------ 1 root root 64 Dec 18 21:27 2 -> /dev/tty1
lr-x------ 1 root root 64 Dec 18 21:27 3 -> /proc/1860/fd
```
因为 tty0 是

如果在 serial 中:
```txt
ls -la /proc/self/fd
lrwx------ - martins3 23 Sep 16:33 0 -> /dev/hvc0
lrwx------ - martins3 23 Sep 16:33 1 -> /dev/hvc0
lrwx------ - martins3 23 Sep 16:33 2 -> /dev/hvc0
lr-x------ - martins3 23 Sep 16:33 3 -> /proc/1512/fd
```

```txt
lrwx------ 1 root root 64 Dec 18 21:25 0 -> /dev/hvc0
lrwx------ 1 root root 64 Dec 18 21:25 1 -> /dev/hvc0
lrwx------ 1 root root 64 Dec 18 21:25 2 -> /dev/hvc0
lr-x------ 1 root root 64 Dec 18 21:25 3 -> /proc/1228/fd
```

## 如果 cat /dev/ttyS0
cat /proc/interrupts | grep ttyS1 才会显示出来



## loginctl

/run/systemd🔒 on ☁️
🧀  sudo ls sessions
[sudo] password for martins3:
1  460	462  500  504  c840
/run/systemd🔒 on ☁️
🧀  loginctl
SESSION  UID USER     SEAT  LEADER  CLASS         TTY    IDLE SINCE
      1 1000 martins3 -     2202    manager       -      no   -
    460 1000 martins3 -     217751  user          pts/69 no   -
    462 1000 martins3 seat0 1335798 user          tty2   no   -
    500 1000 martins3 -     521524  user          pts/9  yes  1h 16min ago
    504    0 root     -     742115  manager-early -      no   -




## who / w

没想到居然是解析 /run/utmp 获取当前那些用户登录的

## cat /proc/tty/drivers

也许是我当时的笔记本
```txt
➜  Vn git:(master) ✗ cat /proc/tty/drivers
/dev/tty             /dev/tty        5       0 system:/dev/tty
/dev/console         /dev/console    5       1 system:console
/dev/ptmx            /dev/ptmx       5       2 system
/dev/vc/0            /dev/vc/0       4       0 system:vtmaster
rfcomm               /dev/rfcomm   216 0-255 serial
usbserial            /dev/ttyUSB   188 0-511 serial
serial               /dev/ttyS       4 64-95 serial
pty_slave            /dev/pts      136 0-1048575 pty:slave
pty_master           /dev/ptm      128 0-1048575 pty:master
unknown              /dev/tty        4 1-63 console
```

13900k
```txt
🧀  cat /proc/tty/drivers
/dev/tty             /dev/tty        5       0 system:/dev/tty
/dev/console         /dev/console    5       1 system:console
/dev/ptmx            /dev/ptmx       5       2 system
/dev/vc/0            /dev/vc/0       4       0 system:vtmaster
usbserial            /dev/ttyUSB   188 0-511 serial
dbc_serial           /dev/ttyDBC   242 0-63 serial
serial               /dev/ttyS       4 64-95 serial
pty_slave            /dev/pts      136 0-1048575 pty:slave
pty_master           /dev/ptm      128 0-1048575 pty:master
unknown              /dev/tty        4 1-63 console
```

13900k 虚拟机:
```txt
🧀  cat /proc/tty/drivers
/dev/tty             /dev/tty        5       0 system:/dev/tty
/dev/console         /dev/console    5       1 system:console
/dev/ptmx            /dev/ptmx       5       2 system
/dev/vc/0            /dev/vc/0       4       0 system:vtmaster
hvc                  /dev/hvc      229 0-7 system
serial               /dev/ttyS       4 64-67 serial
pty_slave            /dev/pts      136 0-1048575 pty:slave
pty_master           /dev/ptm      128 0-1048575 pty:master
pty_slave            /dev/ttyp       3 0-255 pty:slave
pty_master           /dev/pty        2 0-255 pty:master
unknown              /dev/tty        4 1-63 console
```

kunpeng
```txt
 cat /proc/tty/drivers
/dev/tty             /dev/tty        5       0 system:/dev/tty
/dev/console         /dev/console    5       1 system:console
/dev/ptmx            /dev/ptmx       5       2 system
/dev/vc/0            /dev/vc/0       4       0 system:vtmaster
serial               /dev/ttyS       4 64-95 serial
pty_slave            /dev/pts      136 0-1048575 pty:slave
pty_master           /dev/ptm      128 0-1048575 pty:master
unknown              /dev/tty        4 1-63 console
```

kunpeng 虚拟机:
```txt
root@localhost:~# cat /proc/tty/drivers
/dev/tty             /dev/tty        5       0 system:/dev/tty
/dev/console         /dev/console    5       1 system:console
/dev/ptmx            /dev/ptmx       5       2 system
/dev/vc/0            /dev/vc/0       4       0 system:vtmaster
usbserial            /dev/ttyUSB   188 0-511 serial
dbc_serial           /dev/ttyDBC   510 0-63 serial
tegra_hsuart         /dev/ttyTHS   235 0-7 serial
qcom_geni_uart       /dev/ttyHS    236 0-2 serial
qcom_geni_console    /dev/ttyMSM   237       0 serial
msm_serial           /dev/ttyMSM   238 0-2 serial
IMX-uart             /dev/ttymxc   207 16-23 serial
fsl-lpuart           /dev/ttyLP    239 0-11 serial
fsl-linflexuart      /dev/ttyLF    240 0-3 serial
serial               /dev/ttyS       4 64-95 serial
pty_slave            /dev/pts      136 0-1048575 pty:slave
pty_master           /dev/ptm      128 0-1048575 pty:master
unknown              /dev/tty        4 1-63 console
ttyAMA               /dev/ttyAMA   204 64-77 serial
mvebu_serial         /dev/ttyMV    253 0-1 serial
```

## /proc/consoles

## cat /sys/class/tty/console/active

## 9. 观察和调试 TTY 的方法

## stty / tty
```bash
# 当前终端的 termios、窗口大小和行规程相关状态
stty -a
```

## top : 按 f ，展示 TTY

```txt
lr-x------ - martins3 27 Sep 14:58 0 -> pipe:[415219716]
l-wx------ - martins3 27 Sep 14:58 1 -> /home/martins3/.local/share/pueue/task_logs/1236.log
l-wx------ - martins3 27 Sep 14:58 2 -> /home/martins3/.local/share/pueue/task_logs/1236.log
```
## /sys/class/tty

## /sys/class/vcs

## /proc/sys/kernel/pty

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
