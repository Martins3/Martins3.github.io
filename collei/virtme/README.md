## virtme-ng 集成
<!-- b75359ce-3899-434f-93e2-76affceba189 -->

2026-04-15 又搞了一下，基本上是没问题了。

相关文档：
- [virtme-ng-initramfs.md](virtme-ng-initramfs.md): virtme-ng initramfs 机制调研与
  collei 功能差距对比（2026-07-24）
- [FEATURE_ANALYSIS.md](FEATURE_ANALYSIS.md): 早期的 virtme-ng 全面功能分析

### 在 collei.py 中使用 virtme 模式

collei.py 支持 virtme-ng 风格的虚拟机，使用 virtio-fs 共享 host rootfs：

```bash
# 创建 virtme 类型的虚拟机
./collei/scripts/collei.py -V

# 创建后可修改 VM 的 opt/kernel 来指定内核目录
```

创建后，virtme 虚拟机会自动配置：
- 使用 virtio-fs 共享 host 的 `/` 作为 rootfs
- 自动生成精简的 initramfs
- 不需要磁盘镜像（类似 vmtest）

可选配置（在 `vm/<name>/opt/` 目录下）：
- `share_root`: 指定共享的根目录（默认是 `/`）
- `rodir`: 额外的只读目录（格式: `guest_path=host_path`）
- `rwdir`: 额外的可写目录（格式: `guest_path=host_path`）
- `virtme_rw`: 启用可写 overlay（/etc, /home, /var 等）
- `vsock`: 启用 VSOCK 支持（普通 VM 也会挂 vhost-vsock 设备；
  virtme 额外在 guest 内启动 VSOCK SSH 服务）
- `exec`: 启动时执行的命令或脚本

注意：选项文件不能为空，`touch opt/vsock` 不会生效，
需要写入内容（例如 `echo 1 > opt/vsock`）。

示例：
```bash
# 创建 virtme VM
cd ~/.config/collei/vm
cd virtme-test

# 添加额外共享目录
echo "/tmp=/host_tmp" > opt/rwdir

# 启用可写 overlay
touch opt/virtme_rw

# 运行
../../collei/scripts/collei.py
```

### 通过 vsock SSH 登录

启用 `vsock` 后，guest 内会启动一个 vsock SSH 服务
（`virtme-init.sh` 生成的 `/run/tmp/vsock-ssh.py`）：在 guest 里启动
只监听 127.0.0.1 的 sshd（配置和 host key 在 `/run/vsock-ssh/` 下每次
启动重新生成，因为 host 的 `/etc/ssh` 是 root-only，virtiofsd 读不到），
并把 guest 的 vsock:22 转发过去。host 侧通过 collei-action 登录：

```bash
# 直接登录
./collei/scripts/collei-action.py -a ssh_vsock -n virtme

# 只输出命令（用于脚本/自动化）
./collei/scripts/collei-action.py -a ssh_vsock_auto -n virtme
# ssh -o ... -o 'ProxyCommand=socat - VSOCK-CONNECT:<cid>:22' martins3@virtme
```

依赖：host 需要 `socat`；guest 复用 host rootfs 里的 `python3` 和 `sshd`。
登录用户与 `virtme_user` 一致（`opt/user` 或当前用户），使用该用户
home 下的 `~/.ssh/authorized_keys` 做认证；root 登录不可用，因为
`/root` 对 virtiofsd 不可读。

virtme VM 同时启用 `virtme` 和 `vsock` 后，普通的 `ssh` action
（`ge` / `gm` 别名）默认就走 vsock，不再需要显式指定 `ssh_vsock`。

### initramfs 内核模块匹配

`collei/scripts/virtme.py` 生成 initramfs 时会从 `opt/kernel` 指向的
构建树复制 virtio/vsock 模块。内核版本从实际启动的 bzImage/Image 提取
（`collei/scripts/kernel.py`），而不是 `include/config/kernel.release`
—— 后者可能被后续的 make 调用重新生成，与尚未重编的 bzImage/模块不一致。
复制的模块会去掉 `.BTF` 段，避免增量构建树中
"failed to validate module BTF: -22" 导致的 insmod 失败。

### guest 里看到全部 ko（.mod 目录）

`build/build.sh` 编译内核时已经执行了
`make modules_install INSTALL_MOD_PATH=<kernel>.mod INSTALL_MOD_STRIP=1`，
所以 `<kernel>.mod/lib/modules/<kver>/` 就是一份完整、带 depmod 依赖表的
模块安装结果。collei 直接复用它：`virtme.py` 把
`virtme_link_mods=<kernel>.mod/lib/modules/<kver>` 传给 guest init，
`/lib/modules/<kver>` 指向该目录后 `modinfo`/`modprobe`/`lsmod` 直接可用
（kver 从实际启动的内核镜像提取，见上一节）。

不需要 collei 再像 virtme-ng 的 `virtme-prep-kdir-mods` 那样自己构造
`.virtme_mods` 符号链接目录。注意路径同样依赖 host `/` 被整体共享
（默认 `share_root=/`）。

### drgn / 调试信息（vmlinux）

`prepare_debuginfo()` 会在 `<kernel>.mod/lib/modules/<kver>/` 下恢复
`build -> 构建树` 的符号链接（`make modules_install` 默认会创建，
`build/build.sh` 出于打包考虑把它删了）。这样 guest 里的 drgn 通过
标准搜索路径 `/lib/modules/<kver>/build/vmlinux` 就能找到带 DWARF 的
vmlinux，配合 cmdline 里的 `nokaslr` 和 guest 的 `/proc/kcore`，
直接可以调试活内核：

```bash
sudo drgn /home/martins3/data/vn/docs/kernel/tutorial/drgn/scripts/shmem_usage.py
```

已知限制：`.mod` 里的 .ko 经过 `INSTALL_MOD_STRIP=1` strip，drgn 会提示
"missing debugging symbols for <module>"（不影响 vmlinux 本身的分析）。
如果脚本需要模块的调试信息，对应 .ko 在构建树里的原始文件是未 strip 的。

### sudo 支持

guest 共享 host rootfs 时 sudo/su 有三层障碍（virtiofsd 以普通用户运行）：

1. Fedora 的 `/usr/bin/sudo` 是 `---s--x--x` (4111)，virtiofsd 无法读取，
   guest 里 exec 直接 permission denied（Ubuntu 是 4755，所以 virtme-ng
   不会遇到这个问题）
2. `/etc/sudoers`、`/etc/sudo.conf` 是 root-only，sudo 读不到配置
3. `/etc/shadow` 不可读，su 和 PAM 账户检查失败

参考 virtme-ng 的 `generate_sudoers`/`generate_shadow`，`virtme-init.sh`
的 `setup_sudo()` 做了：

- 生成新的 `/etc/sudoers`（root 和 virtme_user 均 NOPASSWD）bind-mount 覆盖
- 生成全空密码的 `/etc/shadow` bind-mount 覆盖（等价 `--empty-passwords`，
  `su` 因此可用；Fedora PAM 默认带 nullok）
- 生成最小 `/etc/sudo.conf` bind-mount 覆盖
- tmpfs 遮住 `/var/db/sudo`（或 `/var/lib/sudo`）；`/root` 不可读时挂 tmpfs
- `/usr/bin/sudo` 用 host 侧准备的 4755 副本 bind-mount 覆盖：
  `virtme.py` 的 `prepare_sudo()` 在启动时用 `sudo install -m 4755` 把
  `/usr/bin/sudo` 复制到 `vm/<name>/s/sudo.bin`（只在副本缺失或 host sudo
  更新时执行，不会每次启动都要密码），路径通过 `virtme_sudo_bin=` 传给 init

---

- https://github.com/arighi/virtme-ng
- https://github.com/amluto/virtme : 之前是这个项目
  - http://arighi.blogspot.com/
      - ubuntu 的内核工程师，这个 blog 有点意思，其他的内容也可以看看

```sh
/usr/bin/qemu-system-x86_64 \
-name virtme-ng \
-m 1G \
-chardev socket,id=charvirtfs5,path=/tmp/virtmeawtwk4o_ \
-device vhost-user-fs-device,chardev=charvirtfs5,tag=ROOTFS \
-object memory-backend-memfd,id=mem,size=1G,share=on \
-numa node,memdev=mem \
-machine accel=kvm:tcg \
-M microvm,accel=kvm,pcie=on \
-cpu host \
-parallel none \
-net none \
-echr 1 \
-chardev stdio,id=console,signal=off,mux=on \
-serial chardev:console \
-mon chardev=console \
-vga none \
-display none \
-smp 32 \
-kernel /root/.cache/virtme-ng/v6.6/amd64/boot/vmlinuz-6.6.0-060600-generic \
-append "virtme_hostname=virtme-ng nr_open=1073741816
virtme_link_mods=/root/.cache/virtme-ng/v6.6/amd64/lib/modules/6.6.0-060600-generic
virtme_rw_overlay0=/etc
virtme_rw_overlay1=/lib
virtme_rw_overlay2=/home
virtme_rw_overlay3=/opt
virtme_rw_overlay4=/srv
virtme_rw_overlay5=/usr
virtme_rw_overlay6=/var
virtme_console=ttyS0
psmouse.proto=exps \"virtme_stty_con=rows 42 cols 175 iutf8\"
TERM=xterm virtme_chdir=.
virtme_user=root
virtme_root_user=1
quiet loglevel=0 i
nit=/virtme-ng/virtme/guest/virtme-init"
-initrd /proc/self/fd/4
```

- https://virtio-fs.gitlab.io/howto-boot.html

## 基本操作

直接测试，配合 virtme-ng.nix 然后就可以运行了

sudo yum install busybox
vng -r ~/data/kernel/linux-build --dry-run

## TODO
virtme/commands/configkernel.py : 参考一下他的 kernel 制作

这个是做什么的?
virtme/virtmods.py

```txt
  --empty-passwords     Use empty passwords for all users
```

这些东西都尝试一下:
```txt
  --console [PORT]      Enable a server to communicate later from the host using '--console-client'. By default, a simple console will be offered using a VSOCK connection, and 'socat' for the proxy.
  --console-client [PORT]
                        Connect to a VM launched with the '--console' option for a remote control.
  --ssh [PORT]          Enable SSH server to communicate later from the host to using '--ssh-client'.
  --ssh-client [PORT]   Connect to a VM launched with the '--ssh' option for a remote control.
  --ssh-tcp             Use TCP for the SSH connection to the guest
  --remote-cmd COMMAND  To start in the VM a different command than the default one (--server), or to launch this command instead of a prompt (--c lient).
```

## key : init
1. 第一种是 rust ，也就是在 virtme_ng_init 中
注意，构建 init 必须使用这个方法，也就是静态构建:
```sh
RUSTFLAGS='-C target-feature=+crt-static' cargo build -r
```

sudo ./venv/lib/python3.13/site-packages/virtme/guest/virtme-init

2. 原来就是有一个基于 bash 的
```txt
virtme/guest/virtme-init
```
通过参数 --no-virtme-ng-init 来控制

那么 -initrd /proc/self/fd/4 是如何构造出来的

如何理解这个东西?
```txt
    need_initramfs = args.force_initramfs or qemu.cannot_overmount_virtfs
```
--force-initramfs

### 为什么还需要 cpio write 之类的
不该是，仅仅 initrd 就可以了吗?

busybox 如何做的? 这个不是和他提供的功能重叠了吗?
virtme/mkinitramfs.py : 直接手动制作一个 initramfs 出来的

首先，任何时候，永远，我们都是需要一个 initrd 文件系统，然后来执行一些命令的
来做切换 root 的操作的，所以这个都是必须的

(先不用着急这个吧，这个显然需要一段时间才可以搞完的)

## key : share
https://lwn.net/Articles/951313/

此后，他添加了许多不同的功能，首先是使用virtiofs和overlayfs，从而可以对 host 的整个文件系统中进行写时复制导出（copy-on-write export），而不是使用 9p。

在 virtiofs 之上添加 overlayfs 允许客户端访问并写入主机文件系统，而不会进行任何永久更改。它使用 tmpfs 作为上层目录，因此当虚拟机退出时，所做的任何更改都会消失。他遇到了 overlayfs 使用 O_NOATIME 标志导致 virtiofs FUSE 守护程序权限错误的问题，但这个问题现在已经在 virtiofs 上游得到了修复。

一位观众问及是否支持更改 user ID，
以便使用 tar 文件中的根文件系统而不是主机文件系统。
Righi 表示对于这种用例有一些支持，但 user ID 问题并没有得到完美解决；
ID-mapped mounts 是人们提出的更好处理该问题的潜在方案。
另一个问题是其他开源项目的采用情况；
Righi 表示Mutter正在使用 virtme-ng 进行测试。
还有一家不愿透露姓名的公司正在使用该工具测试网络摄像头，这让他感到惊讶，原来这家公司正在使用 QEMU 的选项将 USB 设备从 host 传递到 guest，以测试带有 webcam 的多个内核

(有选项可以控制写下去么?)

虚拟机中的基本观察结果:
```txt
Filesystem          Size  Used Avail Use% Mounted on
ROOTFS              3.7T  1.1T  2.7T  28% /
run                 478M   64M  415M  14% /run
devtmpfs            462M     0  462M   0% /dev
virtme_rw_overlay0  478M   64M  415M  14% /etc
virtme_rw_overlay5  478M   64M  415M  14% /usr
virtme_rw_overlay2  478M   64M  415M  14% /home
virtme_rw_overlay3  478M   64M  415M  14% /opt
virtme_rw_overlay4  478M   64M  415M  14% /srv
virtme_rw_overlay6  478M   64M  415M  14% /var
virtme_rw_overlay7  478M   64M  415M  14% /tmp
tmpfs               478M     0  478M   0% /dev/shm
tmpfs               478M  4.0K  478M   1% /var/log
tmpfs               478M     0  478M   0% /var/tmp
tmpfs               478M     0  478M   0% /var/lib/portables
tmpfs               478M     0  478M   0% /var/lib/private
tmpfs               478M     0  478M   0% /var/cache
none                478M     0  478M   0% /usr/lib/modules <-- 指向了 kernel 的目录
```

~~可惜不能执行 sudo 命令~~（已修复，见下面"sudo 支持"）:
```txt
🤒  sudo ls
zsh: permission denied: sudo
```

太 nb 了，这个是让 virtme 来连接的:
```txt
lrwxrwxrwx     - root 26 Jan 20:49   6.18.3-00001-gd99e6e036338-dirty -> /home/martins3/data/kernel/linux-build/.virtme_mods/lib/modules/0.0.0
```

忽然意识到之前的 -kernel 和 -initrd 似乎是很差的，这个就完全自动了
不过这个牺牲了 systemd ?

### 那么 virtiofsd 现在没有 bug 了吗?
/home/martins3/.nix-profile/bin/virtiofsd --syslog --no-announce-submounts --socket-path /tmp/virtmezd2l4jfg \
--shared-dir / --sandbox none -o cache=always

## key : systemd
./vng -r ~/data/kernel/linux-build --systemd

## key : network

```txt
  --network, -n NETWORK
                        Enable network access: user, bridge(=<br>), loop
```

如果是 -net user 配置:
```txt
-device virtio-net-device,netdev=n0
-netdev user,id=n0
-net none
-append ' ... net.ifnames=0  ...'
```

如果是 -net loop 配置:
```txt
-net none
-device virtio-net-device,netdev=n0
-netdev hubport,id=n0,hubid=0
-device virtio-net-device,netdev=n1
-netdev hubport,id=n1,hubid=0
-append ' ... net.ifnames=0  ...'
```

如果 -net 是 bridge ，结果为:
```txt
-net none
-device virtio-net-device,netdev=n0
-netdev bridge,id=n0,br=virbr0
-kernel /home/martins3/data/kernel/linux-build/arch/x86/boot/bzImage
-append ' ... net.ifnames=0  ...'
```

## 基本源码分析

1. virtme/commands/run.py (后端引擎: virtme-run)
这是原始 virtme 项目的核心逻辑，负责“脏活累活”。
 * 功能：直接与 QEMU 交互。它负责拼接复杂的 QEMU 命令行参数，设置文件系统共享 (virtiofs/9p)，生成
   initramfs，处理网络设备、串口重定向等底层虚拟化细节。
 * 入口点：对应命令行工具 virtme-run。
 * 特点：比较底层，参数繁多且复杂，主要关注“怎么运行这个内核”。

2. virtme_ng/run.py (前端界面: vng)
这是 virtme-ng 新增的现代化用户界面。
 * 功能：提供一站式工作流。它负责内核源码的下载、配置、编译（包括远程编译）、清理以及用户参数的解析。
 * 入口点：对应命令行工具 vng。
 * 特点：用户友好，自动化程度高，关注“如何从源码到运行”。

3. 代码中的连接点
在 virtme_ng/run.py 的 run 方法中，你可以看到它明确构造了一个 virtme-run 的命令并执行它。

参见 virtme_ng/run.py 的代码片段 (约 1245 行):
```txt
  1         # Start VM using virtme-run
  2         cmd = (
  3             "virtme-run "
  4             + f"{self.virtme_param['name']} "
  5             + f"{self.virtme_param['exec']} "
  6             # ... 省略大量参数拼接 ...
  7             + f"{self.virtme_param['qemu']} "
  8             + f"{self.virtme_param['qemu_opts']} "
  9         )
 10         if args.pin:
 11             self.set_affinity(args)
 12         check_call(cmd, shell=True)  # <--- 这里调用了 virtme-run
```

总结流程
当你运行 vng --build 时：
 1. `virtme_ng/run.py` 接管，检查内核源码。
 2. 它调用 make 编译内核。
 3. 编译完成后，它解析你的参数（如 --cpus 4 --memory 4G）。
 4. 它将这些参数转换为 virtme-run 能理解的格式。
 5. `virtme_ng/run.py` 执行 shell 命令 virtme-run ...。
 6. `virtme/commands/run.py` 启动，通过 Python 的 subprocess 或 exec 启动 QEMU 进程，虚拟机开始运行。


### virtme/commands/mkinitramfs.py 和 virtme/mkinitramfs.py 的关系是什么?

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
