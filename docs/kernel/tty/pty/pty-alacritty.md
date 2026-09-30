# alacritty 的 PTY 之路:pts 的 master 到底在谁手里

> 实验日期:2026-09-23。源码:`~/data/alacritty`(commit `2f03c30283a1`,0.18.0-dev),
> 构建产物 `target/release/alacritty`。内核源码树:`~/data/kernel/linux`(7.2.0),
> 引用的 `pty.c` / `tty_io.c` / `tty_jobctrl.c` / `n_tty.c` 均在 `drivers/tty/` 下。
> 配套阅读:`tty-architecture-overview.md`、`pty-driver.md`、`tmux.md`。
> 本文的脚本/程序在 `code/` 下:`find-ptmx.sh`、`tiocgptn.c`、`pty.c`、`pty-slave.c`。

## 0. 引子:pts/4 的 master 是谁

`ps -elf` 看到 nvim 跑在 pts/4 上,而 `TTY=?` 的进程里必然有一个持有它的 master fd。
直觉答案是"终端模拟器(alacritty)",但实测:

```sh
# code/find-ptmx.sh:扫描所有进程,找持有 ptmx fd 的家伙
for proc in /proc/[0-9]*; do
  pid=${proc##*/}
  exe=$(readlink /proc/"$pid"/exe 2>/dev/null) || continue
  [[ $exe == *alacritty* ]] || continue
  echo "PID $pid ($exe):"
  for fd in /proc/"$pid"/fd/*; do
    link=$(readlink "$fd" 2>/dev/null) || continue
    [[ $link == *pts* || $link == *ptmx* ]] || continue
    ls -l "$fd"
  done
done
```

结果(本机实测):

```txt
PID 655445 (/usr/bin/alacritty):
30 -> /dev/ptmx                      # alacritty 只有一个 master,对应它的子 shell

tmux server(2399319,TTY=?):
5  -> /dev/ptmx   ├── pts/121  zsh
7  -> /dev/ptmx   ├── pts/4    zsh → nvim    ← 我们要找的!
9  -> /dev/ptmx   ├── pts/26   zsh
10 -> /dev/ptmx   ├── pts/14   zsh
13 -> /dev/ptmx   └── pts/7    zsh
12 -> /dev/pts/22  # 这是 tmux client 所在的 pts,不是 master
```

**pts/4 的 master 是 tmux server,不是 alacritty。** 因为 tmux 在中间又叠了一层:
alacritty 只模拟 tmux client 所在的那个终端;tmux server 自己给每个 pane 开一对新 pty,
对 pane 里的 shell 来说,tmux server 才是"终端模拟器"。

完整分层(本机实测数据):


这一层套娃的细节见 §4,这里先解决通用问题:**给你一个 `/dev/pts/N`,怎么找到 master。**

## 1. 通用方法:找任意 pts/N 的 master

master 持有者的两个特征:

1. `TTY=?`(它自己没有控制终端);
2. fd 表里有指向 `/dev/ptmx` 的 fd。

### 1.1 为什么 procfs 看不到 N

`/proc/PID/fd` 里 master fd 的符号链接显示的是 `/dev/ptmx`,**不显示 N**。
原因不是 procfs 偷懒,而是这个链接本来就指向你 `open()` 的那个路径——
而 master 一律是通过同一个节点 `/dev/ptmx`(cdev `TTYAUX_MAJOR:2`)打开的,
每个 `open()` 各自由 `ptmx_open()` 动态分配一对 pty,**设备节点本身跟 N 无关**。
N 只是 devpts 实例(`devpts_new_index()`)分下来的一个索引,存在 `tty->index` 里,
要主动问才拿得到(§2.4)。

反过来 slave 就没这个问题:它的设备节点**就是**按 N 命名的(`/dev/pts/N`),
所以 `ttyname(0)` / `readlink /proc/self/fd/0` 直接给出答案。
不对称的根源在 §2:master 走的是共享节点,slave 走的是 devpts 下的独立节点。

### 1.2 三个映射办法

**(a) 子进程关联**(最简单,不需要额外权限):
master 持有者的直接子进程(PPID = 它)的 fd 0/1/2 就指向对应的 `/dev/pts/N`。
§0 的 tmux 那张图就是这么画出来的——`ps --ppid <tmux-server>` 一眼看穿。

**(b) `pidfd_getfd` + `TIOCGPTN`**(最准,直接问内核,不需要对方配合):
扫一遍目标进程的 `/proc/PID/fd`,把每个 `/dev/ptmx` fd "偷" 到自己进程里,然后
`ioctl(fd, TIOCGPTN, &n)`——slave 侧对这个 ioctl 返回 `ENOTTY`,所以它同时就是
master 的判定条件。拿到 N 之后再扫一遍全机 `/proc`,顺带把"这个 `/dev/pts/N` 是
谁在用"也回答了:fd 开在它上面的进程,和拿它当控制终端(`/proc/PID/stat` 的
`tty_nr`)的进程。`code/tiocgptn.c` 的核心几行:

```c
int pidfd = syscall(SYS_pidfd_open, pid, 0);
int fd = syscall(SYS_pidfd_getfd, pidfd, target_fd, 0);  // 把别人的 fd 偷过来
unsigned int n = 0;
ioctl(fd, TIOCGPTN, &n);                                  // n 就是 pts 编号
```

只给 pid 就行,不用事先知道 fd 号。单 pty 的 alacritty 一行就完:

```txt
$ make tiocgptn.out && ./tiocgptn.out 655445
pid 655445 fd 30 (/dev/ptmx) -> /dev/pts/40
```

(下面同样会缩进列出 pts/40 的使用者,也就是它的子 shell。)和 (a) 推出来的结论一致。
进程手里 pty 一多,这招的优势就出来了——实测 2026-09-24,对着 §0 那个 tmux server,
每个 pane 的 shell 和它里面跑的东西一目了然:

```txt
$ ./tiocgptn.out 2399319
pid 2399319 fd 7 (/dev/ptmx) -> /dev/pts/4
	pid 2715816  (zsh)             fds 0,1,2,10  ctty
	pid 2716428  (claude)          fds 0,1,2,7,8,9  ctty
pid 2399319 fd 13 (/dev/ptmx) -> /dev/pts/121
	pid 76644    (zsh)             fds 0,1,2,10  ctty
	pid 1978060  (node)            fds 0,1,2  ctty
	pid 1978245  (node_repl)         ctty
...
```

注意两种使用者是分开标的:stdio 重定向走了、只剩控制终端的 `node_repl` 只有 `ctty`;
反过来像 nvim 的 helper 进程那样只继承了 fd、没混成控制终端的,只有 `fds`。

和 (a) 推出来的结论一致。要求 Linux 5.6+(`pidfd_getfd`),且对目标进程有
`PTRACE_MODE_ATTACH_REALCREDS` 权限(同 uid 即可)。
好处是**目标进程完全无感**——fd 只是被借走看了一眼,没有 dup 到它的 fd 表里。

**(c) gdb attach 后 call**:老办法,`pidfd_getfd` 出来之前只能这样,现在基本可以退役了。

(a) 和 (b) 互为验证,见 §6.1。

## 2. 内核机制:从 open("/dev/ptmx") 到拿到 slave

`pty.c` 是全部戏份,`tty_io.c` / `tty_jobctrl.c` 提供会话和挂断等公共设施。

### 2.1 速查表

| 步骤 | 用户态调用 | 内核行为 |
|---|---|---|
| 开 master | `open("/dev/ptmx", O_RDWR\|O_NOCTTY\|O_CLOEXEC)` | devpts 分配一个空闲 pty 索引,返回 master fd |
| 权限 | `grantpt()` | devpts 上基本是 no-op(挂载时 gid/uid 规则定好了) |
| 解锁 | `unlockpt()` = `ioctl(TIOCSPTLCK, 0)` | 允许打开 slave |
| 开 slave | `ioctl(TIOCGPTPEER)` 或 `open("/dev/pts/N")` | TIOCGPTPEER(Linux 4.13+)从 master fd 直接拿 slave,flags 原子生效 |
| 新会话 | `setsid()` | 子进程脱离父进程的会话/控制终端 |
| 设控制终端 | `ioctl(slave_fd, TIOCSCTTY, 0)` | slave 成为该会话的控制终端 |
| 窗口变化 | `ioctl(master_fd, TIOCSWINSZ, &ws)` | 内核对 slave 前台进程组发 **SIGWINCH** |
| master 关闭 | close(master) | 内核对 slave 前台进程组发 **SIGHUP**(这就是关终端窗口杀掉 shell 的原理) |

下面几节把"内核行为"那一列落到实处。信号路径单独放在 §3。

### 2.2 索引是 devpts 分配的:`ptmx_open()`

`ptmx_open()`(`pty.c:767`)的主线:

```c
fsi = devpts_acquire(filp);                          // 拿到当前 mount 的 devpts 实例
scoped_guard(mutex, &devpts_mutex)
        index = devpts_new_index(fsi);               // ① 分配一个空闲索引 N
scoped_guard(mutex, &tty_mutex)
        tty = tty_init_dev(ptm_driver, index);       // ② 造出 master tty_struct
set_bit(TTY_PTY_LOCK, &tty->flags);                  // ③ 默认上锁,slave 还打不开
tty->driver_data = fsi;
tty_add_file(tty, filp);
dentry = devpts_pty_new(fsi, index, tty->link);      // ④ 在 devpts 下造出 /dev/pts/N
```

关键点:**`/dev/ptmx` 是一个公共 cdev**——`unix98_pty_init()` 里
`cdev_add(&ptmx_cdev, MKDEV(TTYAUX_MAJOR, 2), 1)`,每次都走同一个 `ptmx_open()`。
N 不在设备节点里,而在 devpts 实例的 idr 里。这就是 §1.1 那个不对称的来源。

同一函数里还能看到 slave 被默认锁住:

```c
set_bit(TTY_PTY_LOCK, &tty->flags); /* LOCK THE SLAVE */
```

对应用户态的 `unlockpt()` = `ioctl(master, TIOCSPTLCK, &zero)`,清这个 bit。
所以"先开 master 再开 slave"不只是惯例,是硬约束——见 §2.5。

### 2.3 `pty_common_install()`:一对互相 link 的 tty_struct

`pty.c:355`,master/slave 是**成对创建**的,不是先有 master 再另找 slave:

```c
if (driver->subtype != PTY_TYPE_MASTER)
        return -EIO;                 /* Opening the slave first has always returned -EIO */
...
o_tty = alloc_tty_struct(driver->other, idx);        // 同一个 idx!
...
tty->link   = o_tty;                 // 互相指
o_tty->link = tty;
tty_buffer_set_limit(ports[0], 8192);                // 双向缓冲各 8K
tty_buffer_set_limit(ports[1], 8192);
o_tty->port = ports[0];
tty->port   = ports[1];
```

所以一个 pty 对在内核里是**两个 `tty_struct` + 两个 `tty_port`**,共享同一个索引,
靠 `tty->link` 互相指。写 master 的字节从 `tty->link` 那一侧的 ldisc 冒出来(§3.1)。

### 2.4 `TIOCGPTN` 只是 `tty->index`

`pty.c:637`,一行的事:

```c
case TIOCGPTN: /* Get PT Number */
        return put_user(tty->index, (unsigned int __user *)arg);
```

也就是说 N 就是 `ptmx_open()` 里 `devpts_new_index()` 分下来那个值,一直挂在
`tty->index` 上,从来没有离开过内核——`/proc` 不展示它,是因为没人去问。
`tty->link->index` 是同一个值,所以对 master fd 问和对 slave fd 问结果一样。

### 2.5 为什么 master 不能按编号 lookup

```c
static struct tty_struct *ptm_unix98_lookup(struct tty_driver *driver,
                struct file *file, int idx)
{
        /* Master must be open via /dev/ptmx */
        return ERR_PTR(-EIO);
}
```

`ptm_unix98_lookup()`(`pty.c:671`)**永远返回 `-EIO`**。所以不存在
`open("/dev/ptm/N")` 这种东西,master 只能从 `/dev/ptmx` 这一扇门进来。

slave 侧则相反(`pts_unix98_lookup()`,`pty.c:688`):

```c
guard(mutex)(&devpts_mutex);
/* Master must be open before slave */
return devpts_get_priv(file->f_path.dentry) ? : ERR_PTR(-EIO);
```

它不是按 idx 去驱动的 `ttys[]` 数组里查,而是问这个 dentry(devpts 上的
`/dev/pts/N`)当初 `devpts_pty_new()` 时塞进去的私有指针。
**master 没开 → dentry 没建 → 打开 slave 必然 `-EIO`**;master 关了,
`pty_close()` 里 `devpts_pty_kill()` 把这个 dentry 干掉(§3.4)。

## 3. 数据与信号通路

### 3.1 字节怎么走

```txt
shell.write(1, buf)          shell.read(0, buf)
        │ write()                    ▲ read()
        ▼                            │
   slave tty_struct ──── ldisc(N_TTY) ──── slave tty_struct
        │ pty_write()                            ▲ pty_read()
        ▼                                        │
   master tty_struct ──────────────────── master tty_struct
        │ read()                                 ▲ write()
        ▼                                        │
alacritty 事件循环 ──── 渲染/解析 ANSI ──── 键盘输入
```

要点:

- **两个方向各有一个独立的 `tty_port`**,缓冲上限都是 8192(`pty_common_install()`);
- **N_TTY 挂在 slave 侧**。规范模式(ICANON)、回显(ECHO)、Ctrl-C(ISIG)都是
  slave 的 ldisc 在做;master 读到的是"处理完"的字节流;
- 所以 alacritty 在 master 上做 `tcsetattr()` 设的是**对端 slave 的 termios**
  (unix.rs:207 那个 `tcgetattr(&master)` + `IUTF8` 就是这么生效的);
- ANSI 转义序列的**解析和绘制**完全在用户态(alacritty 的渲染器),内核一个字节都不认识。

### 3.2 Ctrl-C:n_tty 的 ISIG

slave 侧 `n_tty` 收到 `^C` 时走 `n_tty.c` 的 `isig(SIGINT, tty)` → `__isig()`:

```c
static void __isig(int sig, struct tty_struct *tty)
{
        struct pid *tty_pgrp = tty_get_pgrp(tty);
        if (tty_pgrp) {
                kill_pgrp(tty_pgrp, sig, 1);
                put_pid(tty_pgrp);
        }
}
```

信号发给 **`tty->ctrl.pgrp`,即该控制终端的前台进程组**——这就是
`TIOCSCTTY` 之后还要有前台进程组的原因,也是 §6 里 `PGID == TPGID` 那行数据的含义。
`^Z` → `SIGTSTP`、`^\` → `SIGQUIT` 同理(`n_tty.c:1120` 附近)。

值得注意的是:**这条路径上 alacritty 完全没参与**——它写的只是 `^C` 这个字节,
信号由内核 ldisc 直接发给 nvim。终端模拟器不转发信号,它只搬运字节。

### 3.3 SIGWINCH:`pty_resize()`

`TIOCSWINSZ` 落到 pty 自己的 `pty_resize()`(`pty.c:282`),不是通用的
`tty_do_resize()`。区别在于 pty 有 `tty->link`,所以**两侧的前台进程组都通知**:

```c
pgrp  = tty_get_pgrp(tty);
rpgrp = tty_get_pgrp(pty);              /* tty->link */
if (pgrp)  kill_pgrp(pgrp,  SIGWINCH, 1);
if (rpgrp != pgrp && rpgrp)
        kill_pgrp(rpgrp, SIGWINCH, 1);
tty->winsize = *ws;
pty->winsize = *ws;                     /* Never used so will go away soon */
```

两个 `winsize` 只有真正被 `TIOCGWINSZ` 读的那个是权威值,另一个是历史包袱
(内核注释自己写了 "will go away soon")。
这就是 `kill -WINCH $pid` / `stty size` / 拖窗口三者能联动的原理:
alacritty 只负责把新的 rows/cols 用 `TIOCSWINSZ` 告诉内核(`unix.rs:414`),
剩下的全是内核的事。

### 3.4 SIGHUP:`pty_close()` → `tty_vhangup()`

`pty.c:47` 的 `pty_close()`,master 侧的分支:

```c
if (tty->driver->subtype == PTY_TYPE_MASTER) {
        set_bit(TTY_OTHER_CLOSED, &tty->flags);
#ifdef CONFIG_UNIX98_PTYS
        if (tty->driver == ptm_driver) {
                guard(mutex)(&devpts_mutex);
                if (tty->link->driver_data)
                        devpts_pty_kill(tty->link->driver_data);  /* 撤掉 /dev/pts/N */
        }
#endif
        tty_vhangup(tty->link);                                   /* 对 slave 挂断 */
}
```

`tty_vhangup()` → `__tty_hangup()`(`tty_io.c:568`)做三件事:

1. 把 slave 上所有已打开的 `file` 换成 `hung_up_tty_fops`(之后 read/write/ioctl 全部 `-EIO`);
2. `tty_signal_session_leader()`(`tty_jobctrl.c:196`):对**会话领袖**发 `SIGHUP` + `SIGCONT`,
   再对**前台进程组** `kill_pgrp(SIGHUP)`;
3. `tty_ldisc_hangup()` 重置线路规约,清掉 `tty->ctrl.session` / `tty->ctrl.pgrp`。

所以"关掉终端窗口 → shell 和 nvim 一起没了"的完整链条是:

```txt
alacritty 窗口关闭
  → 进程退出,close(master_fd)                [退出时 fd 表自动关]
    → pty_close() → devpts_pty_kill() + tty_vhangup(slave)
      → 会话领袖(zsh)收 SIGHUP,前台进程组(nvim)收 SIGHUP
        → zsh 默认动作是退出;nvim 收到后会先善后再退
```

反过来,**slave 侧全关**(`pty_close()` 的 else 分支,`tty->count` 降到 2 以下)不会发 SIGHUP,
只是把 `TTY_OTHER_CLOSED` 打到 master 上让 master 的 read 返回 0——
也就是"程序退出了"而不是"终端断了"。这两种语义的区分是 pty 故意做的,
对 §4 的 tmux 很重要:pane 里最后一个程序退出 ≠ pane 的终端断线。

## 4. tmux 在中间又叠了一层

### 4.1 两层 pty 的分工

回到 §0 的套娃。这里其实有**两对彼此无关的 pty**:

| | master 持有者 | slave 挂在谁身上 | 对谁而言是"终端" |
|---|---|---|---|
| 外层 | alacritty (`/dev/ptmx` → pts/40) | tmux client 的 stdio | tmux client 的"物理终端" |
| 内层 | tmux server (每个 pane 一个 master) | pane 里 zsh 的 stdio | pane 里每个进程的"物理终端" |

tmux server 是个纯粹的字节搬运工:把内层各 pane master 读到的东西按
"哪个 client attach 着哪个 window/pane"重新排版,再写进外层那个 master;
反过来把外层读到的按键按焦点 pane 分发到对应的内层 master。
termios、ISIG、SIGWINCH 这些**两层各做各的**:

- 你在 tmux 里按 `^C`,字节先走外层 ldisc(alacritty → pts/40),tmux client/server
  收到的是 `^C` 字符本身;server 把它写进内层 master,内层 ldisc 才把它变成 `SIGINT`
  发给 nvim 的进程组。也就是说 **`^C` 被解释了两次**;外层那次必须不产生信号,
  所以 tmux client 把外层 slave 的 termios 设成 raw,否则 `^C` 在外层就变成 SIGINT
  把 tmux client 自己打死;
- 你拖 alacritty 窗口,`TIOCSWINSZ` 打在外层 master 上,前台组(tmux client)收 SIGWINCH;
  tmux 算出新的 pane 尺寸,**再对每个内层 master 各发一次 `TIOCSWINSZ`**。
  所以 `stty size` 在 pane 里看到的是 tmux 算出来的值,不是 alacritty 的窗口值;
- 同理 §3.4 那个"slave 全关 ≠ 挂断"的区分,让 tmux 能在 pane 里最后一个进程退出时
  只是回收 pane,而不是把整棵客户端树 SIGHUP 掉。

### 4.2 为什么 alacritty 只有一个 master

因为 alacritty 的模型是**一个窗口 = 一个 pty 对 = 一个子进程**:
`tty::new()` 每次开一个窗口才调用一次(unix.rs:195)。多路复用从来不是它的职责。
tmux server 的模型则是**一个进程持有 N 个 master**,N = 活着的 pane 数;
它自己不需要控制终端(`TTY=?`),因为对它来说"终端"是它造给别人用的,不是它的。

验证(实测):

```sh
# 外层:alacritty 只有一个 ptmx
$ ls -l /proc/$(pidof alacritty)/fd | grep ptmx
lrwx------ 1 martins3 martins3 64 Sep 23 16:19 /proc/655445/fd/30 -> /dev/ptmx

# 内层:tmux server 的每个 ptmx 各管一个 pane,一个命令全列出来(顺带谁在用)
$ ./code/tiocgptn.out 2399319        # tmux server 的 pid,见 §0
```

和 §0 用子进程关联法推出来的 pts/121、pts/4、pts/26、pts/14、pts/7 应该是同一组 N——
这是 §1.2(a)(b) 的交叉验证。

## 5. alacritty 源码走读

代码在 `alacritty_terminal` crate,核心就一个文件:
`alacritty_terminal/src/tty/unix.rs`。

### 5.1 创建 pty 对

`tty::new()`(unix.rs:195)→ `rustix_openpty::openpty(None, Some(&winsize))`:

```rust
pub fn new(config: &Options, window_size: WindowSize, window_id: u64) -> Result<Pty> {
    let pty = openpty(None, Some(&window_size.to_winsize()))?;
    let (master, slave) = (pty.controller, pty.user);
    from_fd(config, window_id, master, slave)
}
```

alacritty 不手写 posix_openpt 三件套,交给 `rustix-openpty 0.2.0`(源码在
`~/.cargo/registry/src/.../rustix-openpty-0.2.0/src/lib.rs`),Linux 分支
(`openpty()` 的 `#[cfg(any(target_os = "android", target_os = "linux"))]` 块):

```rust
let flags = OpenptFlags::RDWR | OpenptFlags::NOCTTY | OpenptFlags::CLOEXEC;
let controller = openpt(flags)?;      // open("/dev/ptmx") → master
grantpt(&controller)?;                // devpts 上基本 no-op
unlockpt(&controller)?;               // TIOCSPTLCK=0
let user = open_user(&controller, flags | OpenptFlags::CLOEXEC)?;  // slave
if let Some(winsize) = winsize { tcsetwinsize(&user, *winsize)?; }  // 窗口大小在 spawn 前就设好
```

`open_user`(同文件 :235)优先用 `TIOCGPTPEER`(4.13+ 一步到位拿 slave fd),
失败(NOSYS/PERM)再回退 `ptsname()` + `openat("/dev/pts/N")`:

```rust
match rustix::pty::ioctl_tiocgptpeer(controller, flags) {
    Ok(fd) => return Ok(fd),
    Err(io::Errno::NOSYS) | Err(io::Errno::PERM) => {}
    Err(e) => return Err(e),
}
let name = rustix::pty::ptsname(controller, Vec::new())?;
openat(CWD, name, flags.into(), Mode::empty())
```

用 `TIOCGPTPEER` 的好处是**不需要拼 `/dev/pts/N` 这个路径**,从 master fd 直接
在同一个 devpts 实例里拿到 slave,天然避开 mount namespace / chroot / 符号链接问题;
而且 `O_CLOEXEC` 这类 flags 能**原子**生效(回退分支的 `openat` 就做不到)。
回退分支只在老内核或 seccomp 环境才走——注释里点名了 Android,seccomp 会杀掉这条 syscall。

`winsize` 在 spawn **之前**就设到 slave 上,是为了让 shell 一睁眼 `stty size`
就是对的;否则第一屏 prompt 的换行会先按 80×24 算一遍。

### 5.2 fork/exec 子 shell

`from_fd()`(unix.rs:202):

```rust
// 子进程的 stdio 全部指向 slave
builder.stdin(slave.try_clone()?);
builder.stderr(slave.try_clone()?);
builder.stdout(slave);

unsafe {
    builder.pre_exec(move || {
        let err = libc::setsid();               // ① 新会话,脱离 alacritty 的控制终端
        if err == -1 { return Err(...); }
        set_controlling_terminal(slave_fd)?;    // ② ioctl(slave_fd, TIOCSCTTY, 0)
        libc::close(slave_fd);                  // ③ 子进程不再需要这两个 fd
        libc::close(master_fd);                 //    (stdio 已经 dup 成 0/1/2)
        libc::signal(libc::SIGCHLD, libc::SIG_DFL);  // ④ 信号恢复默认
        ...
        Ok(())
    });
}
```

①② 的顺序不能换:`TIOCSCTTY` 要求调用者**是会话领袖且还没有控制终端**,
`setsid()` 正好一次满足两个条件(新会话领袖 = 自己的 PID,且尚未关联任何 ctty)。
alacritty 自己是不是 `TTY=?` 反倒无所谓(见下文「从终端启动 vs 从 GNOME 启动」),
因为子进程换了新会话。

④ 那一串 `signal(..., SIG_DFL)` 也值得看一眼:alacritty 为了收 SIGCHLD
在自己进程里装了 handler(`signal_hook::low_level::pipe`,`unix.rs:283`),
必须在 `exec` 前恢复默认,否则 shell 会继承一个指向 alacritty 事件管道的 handler,
行为完全错乱。**这是所有 `fork`+`exec`+`pre_exec` 组合都容易漏的一步。**

spawn 成功后父进程(alacritty)这边:

```rust
set_nonblocking(master_fd);
Ok(Pty { child, file: File::from(master), signals, sig_id })  // master fd 一直留在 alacritty 手里
```

### 5.3 运行期

- 事件循环 poll master fd:读 shell 输出 → 渲染;键盘输入 → 写 master;
- resize:`ioctl(master, TIOCSWINSZ, &win)`(unix.rs:414),内核走 §3.3 的
  `pty_resize()` 给 shell 发 SIGWINCH,所以 `kill -WINCH` / `stty size` 能联动;
- shell 退出:SIGCHLD 通过 signal pipe 通知事件循环(`ChildEvent::Exited`,
  `try_wait()` 在 unix.rs:395),窗口退出。

## 6. 验证实验(自编 0.18.0-dev,实测数据)

```sh
./target/release/alacritty -t PTY-EXPERIMENT \
  -e zsh -c 'ps -o pid,ppid,sid,pgid,tty,comm; ls -l /proc/$$/fd; sleep 300'
```

从外部采集:

```txt
$ ps -o pid,ppid,sid,tty,comm -p 690730        # alacritty 本尊
  PID    PPID     SID TT       COMMAND
690730    2202  690727  ?       alacritty       # TTY=?:终端模拟器自己没有控制终端

$ ls -l /proc/690730/fd | grep ptmx
33 -> /dev/ptmx                                # master fd,链接不显示 pts N

$ ps -o pid,ppid,sid,pgid,tty,tpgid,comm --ppid 690730
  PID    PPID     SID    PGID TT         TPGID COMMAND
690747  690730  690747  690747 pts/19    690747 sleep
#              ^^^^^^^ 子进程 SID == 自己的 PID:setsid() 生效,它是会话领袖
#              且 PGID == TPGID == 自己:它也是前台进程组

$ ls -l /proc/690747/fd
0 -> /dev/pts/19
1 -> /dev/pts/19
2 -> /dev/pts/19        # stdio 全是 slave,pre_exec 里 close(master_fd) 已生效
```

三组数据正好和源码一一对应:master(33→ptmx)留父进程;子进程 SID=PID
(setsid)、控制终端 pts/19(TIOCSCTTY)、stdio=slave、多余 pty fd 已关。

### 6.1 补一刀:从外部确认 N

用 §1.2(b) 的办法确认 N,不用碰子进程:

```txt
$ ./code/tiocgptn.out 690730
pid 690730 fd 33 (/dev/ptmx) -> /dev/pts/19
	pid 690747   (sleep)           fds 0,1,2  ctty
```

和 `ls -l /proc/690747/fd` 看到的 pts/19 对上了——**两条独立路径同一个答案**,
这就是 §1.2 里 (a) 和 (b) 可以互为验证的原因。

### 6.2 意外收获:fd 继承泄漏

子进程 fd 表里除了 0/1/2 还有一串不属于它的东西:

```txt
103 -> /tmp/.mount_ZCode.*/v8_context_snapshot.bin
 39 -> /tmp/.mount_ZCode.*/resources/app.asar
 37 -> anon_inode:inotify
 38 -> socket:[...]
```

alacritty 的 `pre_exec` **只显式关 master/slave 两个 fd**,其余继承到的 fd 原样传给
子进程。这里泄漏的是启动它的父进程环境(AppImage 挂载点的内部文件句柄,且没设
FD_CLOEXEC)。教训:父进程链上任何一个不干净的 fd 都会一路传到底,CLOEXEC 是唯一
可靠的闸门。可以用 `close_range(3, ~0U, CLOSE_RANGE_UNSHARE)` 一刀切干净。

值得注意的是这**不是 alacritty 的 bug**——它开 pty 时规规矩矩带了 `O_CLOEXEC`
(§5.1 的 `OpenptFlags::CLOEXEC`),泄漏的是更上层启动它的东西。`close_range` 那一刀
应该是最终 exec 的那一层在做,谁 spawn 谁负责。
反过来说,§1.2(b) 的 `pidfd_getfd` 之所以好用,也是因为 fd 一路都是明账——
fd 表越规范,审计越容易。

## 7. ioctl 速查(本次实验涉及)

| ioctl         | 作用                                     | 谁调用              | 内核入口(`pty.c`)                     |
| ------------- | ---------------------------------------- | ------------------- | ------------------------------------- |
| `TIOCSPTLCK`  | 上锁/解锁 slave(unlockpt)                | 创建方              | `pty_set_lock()`,清/置 `TTY_PTY_LOCK` |
| `TIOCGPTLCK`  | 查询当前锁状态                           | 创建方              | `pty_get_lock()`                      |
| `TIOCGPTN`    | 查询 master 对应的 pts 编号 N            | 创建方              | `put_user(tty->index, ...)`           |
| `TIOCGPTPEER` | 直接从 master fd 打开 slave(4.13+)       | 创建方              | `ptm_open_peer_file()`                |
| `TIOCSCTTY`   | 把 fd 设为本会话控制终端                 | 子进程(setsid 之后) | `tty_io.c` / `tty_jobctrl.c`          |
| `TIOCSWINSZ`  | 设置窗口大小,内核给前台组发 SIGWINCH     | 模拟方              | `pty_resize()`                        |
| `TIOCGWINSZ`  | 查询窗口大小(`stty size`)                | 任意                | 通用 `tty_ioctl`                      |
| `TIOCPKT`     | 打开 packet 模式,master 收到带外状态字节 | 模拟方(可选)        | `pty_set_pktmode()`                   |
| `TIOCGPKT`    | 查询 packet 模式                         | 任意                | `pty_get_pktmode()`                   |
| `TIOCSIG`     | 从一端向另一端的前台组发任意信号         | 模拟方(可选)        | `pty_signal()` → `kill_pgrp()`        |

后三个 alacritty **没用**:它靠"把 `^C` 当字节写进 master"让内核 ldisc 发信号(§3.2),
而不是直接 `TIOCSIG`。`TIOCPKT` 是 sshd/tmux 那类需要区分"数据"和"控制状态"
(比如对方 `^S` 停了输出,`pty.c:326` 会往 master 送 `TIOCPKT_STOP`)的模拟器才要的,
纯显示型终端不需要。
`pty_resize()` 也可以对照 §3.3:它比通用 `tty_do_resize()`(`tty_io.c:2324`)多做了一件事——
把 `tty->link` 那一侧的前台组也一起 SIGWINCH 了。

## 8. 参考

- pty(7) man page:`https://man7.org/linux/man-pages/man7/pty.7.html`
- pidfd_getfd(2):`https://man7.org/linux/man-pages/man2/pidfd_getfd.2.html`
- alacritty 源码:`alacritty_terminal/src/tty/unix.rs`(本仓库 commit 2f03c30283a1)
- rustix-openpty 0.2.0:`~/.cargo/registry/src/index.crates.io-*/rustix-openpty-0.2.0/src/lib.rs`
- 内核:`drivers/tty/pty.c`(`ptmx_open()`、`pty_common_install()`、`pty_close()`、
  `pty_resize()`)、`drivers/tty/tty_io.c`(`__tty_hangup()`、`tty_do_resize()`)、
  `drivers/tty/tty_jobctrl.c`(`tty_signal_session_leader()`)、
  `drivers/tty/n_tty.c`(`__isig()`)、`fs/devpts/inode.c`(`devpts_new_index()`)
- 本目录:`code/find-ptmx.sh`、`code/tiocgptn.c`、`code/pty.c`、`code/pty-slave.c`,
  `tty-architecture-overview.md`(整体分层)、`pty-driver.md`(用户态 driver 视角)、
  `getty.md` / `login.md`(VT 上的登录路径)、`tmux.md`(tmux 操作)


## 很有意思
1. 写一个新的 emulater 出来发
	- 回答一个问题，为什么要去绕一圈 ？
2. alacritty 在 termianl 中启动和直接利用 gnome 启动，原理有何区别?
	- 和
3. 为什么需要 tty2 ?


### 为什么要绕这一圈:pty 到底在适配什么

shell / ncurses / vim 要的不是一个"能显示字符的 socket",而是**一个内核 tty 设备**:
`isatty()` 得为真,`tcgetattr/tcsetattr` 得有意义,得能 `TIOCSCTTY` 成控制终端,
得有前台进程组(`tcsetpgrp`)从而有 job control,`^C`/`^Z` 得是终端行为而不是字节。
这一整套语义在内核里已经存在了几十年——为串口和 VT 写的,所有 tty 共用。

而终端模拟器是**用户态程序**,它天然只能提供"字节从某个 fd 进出"。
pty 就是这对矛盾的适配器,它把两件事劈开:

```txt
      终端语义(内核的、重的、历史包袱多的)     终端绘制(用户态的、要 GPU/字体/滚动)
      ─────────────────────────────────     ────────────────────────────────
      N_TTY ldisc / termios / ISIG           ANSI 解析、字体、滚动历史
      ctty / 会话 / 前台进程组                 多 tab、分屏、复制粘贴
      TIOCSWINSZ / SIGHUP                    与 compositor 对接
              ↑                                      ↑
          slave 侧                               master 侧
              └───────── 一对 pty 互相 link ─────────┘
```

所以"绕一圈"不是绕远路,是**把标准化的便宜部分外包给内核**。
反方案为什么都不成立:

| 反方案 | 为什么不行 |
|---|---|
| 模拟器和 shell 用 socketpair 直连 | `isatty()` 为假,ncurses 直接降级或拒跑;没有 ctty 就没有 job control;`^C` 只是一个字节,谁来发 SIGINT? |
| 终端模拟放进内核 | Linux console 就是这条路(VT 层),能力上限 = DRM + 内核字体,滚动历史 / tab / GPU 全没有 |
| 模拟器做成内核模块 | 一个字体渲染 bug 就是内核 panic;GUI toolkit 也进不去 |

对"写一个新的 emulator"来说,结论很直接:**`open("/dev/ptmx")` 免费拿到完整的 tty 语义**,
§2 那一整套行为你一行都不用写,`write(master, ...)` 就有 shell 在对面等着。
要跟别人竞争的只剩下绘制和输入——这才是"绕一圈"的回报,也是这个行当能不断出新模拟器的原因。
`code/pty.c` / `code/pty-slave.c` 就是在反复体会这件事:一旦让 slave 侧以为自己不是 tty,
bash 立刻开始抱怨 `cannot set terminal process group` / `no job control in this shell`。

### 从终端启动 vs 从 GNOME 启动:差别只在 alacritty 自己

先说结论:**alacritty 从不给自己 `setsid()`,它自己的控制终端完全是继承来的。**
窗口里那个 shell 的行为两种启动方式**一模一样**,差别只落在 alacritty 这个进程身上。

实测三组数据(同一台机器,同一份二进制):

```txt
# A. 从终端启动(在一个 tmux pane 里,外层 pts/27)
$ ps -o pid,ppid,sid,pgid,tty,comm -p 822802
 822802  822796  822796  822796 pts/27   alacritty
#        ^^^^^^  ^^^^^^  ^^^^^^ ^^^^^^^
#        父=pane shell  同会话  同进程组  ★有控制终端,继承自外层 pts

# B. 从 GNOME 启动(点图标)
$ ps -o pid,ppid,sid,pgid,tty,comm -p 655445
 655445 1337095 1337095 1337095 ?        alacritty
#        ^^^^^^^ gnome-shell   ★无控制终端
$ head -1 /proc/655445/cgroup
0::/user.slice/user-1000.slice/user@1000.service/app.slice/app-gnome-Alacritty-655445.scope

# C. 从一个自己就没有 ctty 的 shell 里启动(脚本 / CI / 工具链)
$ ps -o pid,ppid,sid,pgid,tty,comm -p 819799
 819799  819798  819796  819798 ?        alacritty
$ head -1 /proc/819799/cgroup
0::/.../app.slice/tmux-spawn-aa5d0dcb-20a4-4d74-b070-0842524efed3.scope
```

判别式就一行:

```sh
ps -o tty= -p $(pidof alacritty)     # pts/N → 从终端启动;? → 图形会话或无 ctty 环境
```

三个层面的差别:

1. **pty 的创建路径完全一样**。`tty::new()`(unix.rs:195)不看自己有没有 ctty,
   无条件 `open("/dev/ptmx")`;子进程 `setsid()` + `TIOCSCTTY` 挂在**新**会话上,
   跟父进程的 ctty 无关(§5.2)。所以 `stty`、job control、`^C`、SIGWINCH
   在窗口里的表现一字不差。
2. **alacritty 自己的抗打击能力不一样**(这才是问题的实质):
   - A:它是外层 pts 的会话成员。按 §3.4 的链条,外层终端一挂断
     (关掉那个 pane / 敲 `exit` / ssh 断线),`tty_vhangup()` 把 SIGHUP 送到会话领袖,
     alacritty 跟着死——"我从终端里起的窗口,把终端关了窗口也没了"就是这么来的。
     交互 shell 下按 `^C` 还会直接打到它头上(它是外层前台进程组成员)。
   - B/C:无 ctty,不受任何终端挂断影响,生命周期交给 systemd user / 启动器。
3. **cgroup 归属不一样**,但只影响资源配额和崩溃归属,不影响 pty 语义:
   A/C 继承启动者的 cgroup(`tmux-spawn-*.scope`),B 由 GNOME/g_spawn 给它单开
   一个 app scope(`app-gnome-Alacritty-*.scope`)。

一个反直觉的推论:**"终端模拟器自己有没有控制终端"不是必需品**。
它对面那个 shell 有(而且必须有,`TIOCSCTTY` 就是干这个的),
它自己有没有都行。§0 里 tmux server `TTY=?` 是同一个道理的极端版本——
**给别人造终端的进程,自己不需要终端**。

### 为什么底下还需要一个 tty2:VT 是会话锚点和救生舱

这里的 `tty2` 理解为"图形会话底下那个 VT"。实测本机:

```txt
$ ps -eo pid,ppid,sid,tty,comm | grep -E 'gdm|gnome-session|gnome-shell' | head
  2198       1    2198 ?        gdm
1335798    2198    2198 ?        gdm-session-wor
1336444  1335798  1336444 tty2     gdm-wayland-ses      ← 真正持有 tty2
1337044  1336444  1336444 tty2     gnome-session-i      ← 同上
1337095    2202  1337095 ?        gnome-shell           ← 反倒没有

$ loginctl show-session 462 -p Type -p TTY -p Seat -p Class
Seat=seat0
TTY=tty2
Type=wayland
Class=user
```

图形会话(Wayland)底下仍然吊着一个 `tty2`,四个理由:

1. **登录路径的历史惯性**。getty 在 ttyN 上跑,`login` 认证完
   `setsid()` + `TIOCSCTTY` 把会话挂在 ttyN 上,logind 的 session 需要一个 `TTY`
   字段当身份。见 `getty.md` / `login.md`。换到图形时代,这套锚点没换掉,
   只是坐在上面的进程从 `login` 换成了 `gdm-wayland-session`。
2. **seat / 多座位归属**。logind 用 `(seat, VT)` 给会话划地盘,udev 的 `seat` 标签
   把键盘/鼠标/显示器分给 seat0。没有 VT 就没有稳定锚点,多座位(一台机器两个
   显示器各带各的键鼠)就没法表达。
3. **compositor 的逃逸舱**。DRM/KMS 的 master 权限和 VT 绑定;`Ctrl+Alt+F1` 切走
   时内核走 VT_RELDISP 把图形让出来,这样图形栈崩了、卡死了,你还能切到 tty1
   救命。这是"图形底下必须有个内核 VT"最硬的理由。
4. **VT 是往下的,pty 是往上的,两者方向相反**。pty(§2)是给用户态**造**出一堆
   终端给程序用;VT 是把内核**接**到键盘/显示这台"真终端"上。合起来才是完整的
   tty 栈,见 `tty-architecture-overview.md`。服务器上没有键盘/显示,底下那一层
   就换成 `ttyS0` 串口控制台,上层的 pty 一点不变。

顺带解释上一节的数据:gnome-shell 自己 `TTY=?`,真正持有 tty2 的是
`gdm-session-worker` → `gdm-wayland-session` → `gnome-session-binary` 这条会话领导链,
gnome-shell 挂在 systemd --user(2202)下面反而不带 ctty——又是那条
"给别人造界面的进程,自己不需要终端"。至于 alacritty,它比 gnome-shell 更靠上,
离 tty2 隔了好几层,自然也是 `?`。

## 再看看，原来是这个结果啊，这是不错的

tmux 的结果:
```txt
🧀  ls -la
lrwx------ - martins3 23 Sep 16:29 0 -> /dev/null
lrwx------ - martins3 23 Sep 16:29 1 -> /dev/null
lrwx------ - martins3 23 Sep 16:29 2 -> /dev/null
lr-x------ - martins3 23 Sep 16:29 3 -> pipe:[36073925]
l-wx------ - martins3 23 Sep 16:29 4 -> pipe:[36073925]
lrwx------ - martins3 23 Sep 16:29 5 -> /dev/ptmx
lrwx------ - martins3 23 Sep 16:29 6 -> socket:[36086328]
lrwx------ - martins3 23 Sep 16:30 7 -> /dev/ptmx
lrwx------ - martins3 23 Sep 16:33 8 -> /dev/ptmx
lrwx------ - martins3 23 Sep 16:29 9 -> /dev/ptmx
lrwx------ - martins3 23 Sep 16:29 10 -> /dev/ptmx
lrwx------ - martins3 23 Sep 16:29 11 -> socket:[355904676]
lrwx------ - martins3 23 Sep 16:29 12 -> socket:[355906695]
lrwx------ - martins3 23 Sep 16:29 13 -> /dev/ptmx
lrwx------ - martins3 23 Sep 17:25 15 -> /dev/ptmx
lrwx------ - martins3 23 Sep 23:26 16 -> socket:[355877212]
lrwx------ - martins3 23 Sep 23:26 17 -> /dev/pts/9
```

```txt
zsh     1341989 martins3  0u   CHR 136,22      0t0   25 /dev/pts/22
zsh     1341989 martins3  1u   CHR 136,22      0t0   25 /dev/pts/22
zsh     1341989 martins3  2u   CHR 136,22      0t0   25 /dev/pts/22
zsh     1341989 martins3 10u   CHR 136,22      0t0   25 /dev/pts/22
zsh     1341989 martins3 13w   CHR 136,22      0t0   25 /dev/pts/22
```

ghostty 的结果:
```txt
/proc/1341870🔒 on ☁️
🧀  ls -la fd
lr-x------ - martins3 18 Sep 11:12 0 -> /dev/null
lrwx------ - martins3 18 Sep 11:12 1 -> socket:[264563792]
lrwx------ - martins3 18 Sep 11:12 2 -> socket:[264563792]
lrwx------ - martins3 18 Sep 11:12 3 -> anon_inode:[eventfd]
lrwx------ - martins3 18 Sep 11:12 4 -> anon_inode:[eventfd]
lrwx------ - martins3 23 Sep 16:28 5 -> socket:[264551119]
lrwx------ - martins3 23 Sep 16:28 6 -> anon_inode:[eventfd]
lrwx------ - martins3 23 Sep 16:28 7 -> socket:[264525506]
lrwx------ - martins3 23 Sep 16:28 8 -> anon_inode:[eventfd]
lrwx------ - martins3 18 Sep 11:12 9 -> '/memfd:mutter-anonymous-file-dma
lrwx------ - martins3 23 Sep 16:28 10 -> socket:[264558115]
lrwx------ - martins3 23 Sep 16:28 11 -> /dev/dri/renderD128
lrwx------ - martins3 23 Sep 16:28 12 -> /dev/dri/renderD128
lrwx------ - martins3 23 Sep 16:28 13 -> /dev/dri/renderD128
lrwx------ - martins3 23 Sep 16:28 14 -> /dev/dri/renderD128
lrwx------ - martins3 23 Sep 16:28 15 -> socket:[264554704]
lrwx------ - martins3 23 Sep 16:28 16 -> anon_inode:[io_uring]
lrwx------ - martins3 23 Sep 16:28 17 -> anon_inode:[eventfd]
lrwx------ - martins3 23 Sep 16:28 18 -> anon_inode:[eventfd]
lrwx------ - martins3 23 Sep 16:28 19 -> anon_inode:[eventfd]
lrwx------ - martins3 23 Sep 16:28 20 -> anon_inode:[io_uring]
lrwx------ - martins3 23 Sep 16:28 21 -> anon_inode:[eventfd]
lrwx------ - martins3 23 Sep 16:28 22 -> anon_inode:[eventfd]
lrwx------ - martins3 23 Sep 16:28 23 -> /dev/ptmx
lrwx------ - martins3 23 Sep 16:28 24 -> anon_inode:[pidfd]
lrwx------ - martins3 23 Sep 16:28 25 -> /dmabuf:
lr-x------ - martins3 23 Sep 16:28 26 -> pipe:[264526605]
l-wx------ - martins3 23 Sep 16:28 27 -> pipe:[264526605]
lrwx------ - martins3 23 Sep 16:28 36 -> anon_inode:[io_uring]
lrwx------ - martins3 23 Sep 16:28 39 -> anon_inode:[eventfd]
lrwx------ - martins3 23 Sep 16:28 40 -> anon_inode:[eventfd]
lrwx------ - martins3 23 Sep 16:28 41 -> anon_inode:[eventfd]
lrwx------ - martins3 23 Sep 16:28 42 -> anon_inode:[io_uring]
lrwx------ - martins3 23 Sep 16:28 43 -> anon_inode:[eventfd]
lrwx------ - martins3 23 Sep 16:28 44 -> anon_inode:[eventfd]
lrwx------ - martins3 23 Sep 16:28 45 -> /dev/ptmx
lrwx------ - martins3 23 Sep 16:28 46 -> anon_inode:[pidfd]
lr-x------ - martins3 23 Sep 16:28 48 -> pipe:[310329974]
l-wx------ - martins3 23 Sep 16:28 49 -> pipe:[310329974]
```

## 调查一下 alacritty 之类的 terminal emualtor 和 terminal 的关系支持

才两万行，而且还支持三个平台

那么，windows 都是如何实现这个东西？

## 类似的
https://hpjansson.org/chafa/
	https://news.ycombinator.com/item?id=46278208
https://github.com/orhun/ratty

https://github.com/mmulet/term.everything

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
