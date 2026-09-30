# virtme-ng 集成
<!-- b75359ce-3899-434f-93e2-76affceba189 -->

## 在 collei.py 中使用 virtme 模式

collei.py 支持 virtme-ng 风格的虚拟机，使用 virtio-fs 共享 host rootfs：

```bash
# 创建 virtme 类型的虚拟机
./collei/scripts/collei-install.py -V

# 创建后可修改 VM 的 config.ini 中的 kernel 来指定内核目录
```

创建后，virtme 虚拟机会自动配置：

- 使用 virtio-fs 共享 host 的 `/` 作为 rootfs
- 自动生成精简的 initramfs
- 不需要磁盘镜像（类似 vmtest）

可选配置位于 `vm/<name>/config.ini` 的 `[vm]` 节：

- `share_root`: 指定共享的根目录（默认是 `/`）
- `rodir`: 额外的只读目录（格式: `guest_path=host_path`）
- `rwdir`: 额外的可写目录（格式: `guest_path=host_path`）
- `virtme_rw`: 启用可写 overlay（/etc, /home, /var 等）
- `cwd`: 指定 guest 初始目录（默认继承启动 collei 时的 host 当前目录）
- `network`: guest 自动配网模式。缺失、`auto` 或兼容旧值 `1` 时默认启用； `off`
  或 `0` 时保留网卡设备但不在 guest 中配置地址
- `net_backend`: 用户态网络后端，缺失或 `user` 使用 SLIRP，`passt` 独立启动后
  通过 QEMU 通用 vhost-user 后端连接；passt 模式要求 host 的 `PATH` 中存在
  `passt`
- `vsock`: 启用 VSOCK 支持（普通 VM 也会挂 vhost-vsock 设备； virtme 额外在
  guest 内启动 VSOCK SSH 服务）
- `tty`: 每行写一个 `frontend backend`，例如 `serial socket`、`hvc pty` 或
  `virtserialport socket`。路径、日志名和 virtserialport guest name 按顺序自动生成。
  同一前端可以重复，分别从 0 编号。例如 `serial stdio` 后接 `serial socket`，
  在 x86 guest 中对应 `ttyS0` 和 `ttyS1`，第二路 socket 为 `serial1.socket`。
  `hvc` 同样独立编号为 `hvc0`、`hvc1`；硬件串口数量受 QEMU machine 限制。
  socket 统一命名为 `<frontend><index>.socket`，例如 `hvc0.socket`、
  `virtserialport0.socket`；stdio 和 PTY 通道也占用该前端的编号。
- virtme 配置至少保留一个 `virtserialport` 通道；virtme init 会自动在第一个通道
  (`org.qemu.vport.0`) 上启动 `qemu-ga`。host 侧的 `qemu-ga` 绝对路径通过
  kernel cmdline 传入，host 端用 `-a qga` 连接。

完整的配置片段见 [`serial-demo.config.ini`](serial-demo.config.ini)。它是一个
`[vm]` 配置片段，添加到现有 VM 的 `config.ini` 中；它不是单独可启动的 VM。

普通 VM 与 virtme 使用同一套串口布局。backend 由 `tty` 配置决定；普通 foreground
和 background 都保持配置中的 stdio。只有 `-d` GDB 前台运行时，stdio 才自动切换
到对应 socket，不需要修改配置。多个 stdio frontend 会自动共享一个 QEMU mux，多个 PTY 或 socket 保持独立。`console`、`hvc`
可重新连接 socket/PTY，`serial_log`、`hvc_log` 查看交互通道日志。

使用 pueue backend 时，后台任务仍保留 stdio，`pueue follow` 查看输出、`pueue send`
发送输入；systemd backend 的 stdout 进入 journal，stdio 不再是可交互终端，需要
交互时应在 `tty` 中选择 socket 或 PTY。QEMU monitor 始终使用独立 QMP/HMP socket，
不与 serial stdio 共用。

注意：缺失或空值表示未启用；启用布尔选项时写入 `1`。 `network` 是例外：virtme
默认自动配网，必须显式写 `network = off` 才关闭。

示例：

```bash
# 进入创建好的 virtme VM 目录
cd ~/data/hack/vm/virtme-test

# 编辑 config.ini
# [vm]
# rwdir = /tmp=/host_tmp
# virtme_rw = 1

# 运行
../../collei/scripts/collei.py
```

运行中的 VM 可以通过 QMP 注入键盘序列；不带 key 时默认发送 Ctrl+Alt+Delete：

```bash
./collei/scripts/collei-action.py -a send_key -n virtme-test
./collei/scripts/collei-action.py -a send_key -n virtme-test ctrl alt f1
```

## 自动网络配置

virtme 默认按 MAC 而不是 `ens4`、`ens5` 等接口名配置两张 QEMU 网卡：

- 用户态网卡固定使用 `52:54:00:12:34:56`，通过 SLIRP 或 passt 的 DHCP 获取
  地址、默认路由和 DNS；只有 SLIRP 后端固定使用 `10.0.2.15/24`。
- 启用 host bridge 时，tap/vhost 网卡使用 `52:54:00:<level>:<VM id>:00`，配置
  `10.0.<VM id>.<level>/16`，只提供 connected
  route，不覆盖用户态网卡的默认路由。

额外网卡不会被自动配置。接口名可能随 PCI 拓扑变化，应使用 MAC、地址和路由
确认配置结果。全局配置为 `bridge = no` 时只配置用户态网卡。

## 通过 vsock SSH 登录

启用 `vsock` 后，guest 内会启动一个 vsock SSH 服务 （Rust init 中的
`run_vsock_sshd()`）：在 guest 里启动只监听 127.0.0.1 的 sshd（配置和 host key
在 `/run/vsock-ssh/` 下每次启动重新生成，因为 host 的 `/etc/ssh` 是
root-only，virtiofsd 读不到），并用内置 Rust proxy 把 guest 的 vsock:22
转发过去。host 侧通过 collei-action 登录：

```bash
# 直接登录
./collei/scripts/collei-action.py -a ssh_vsock -n virtme

# 只输出命令（用于脚本/自动化）
./collei/scripts/collei-action.py -a ssh_vsock_auto -n virtme
# ssh -o ... -o 'ProxyCommand=socat - VSOCK-CONNECT:<cid>:22' martins3@virtme
```

依赖：host 需要 `socat`；guest 复用 host rootfs 里的 `sshd`。 登录用户与
`virtme_user` 一致（`config.ini` 中的 `user` 或当前用户），使用该用户 home 下的
`~/.ssh/authorized_keys` 做认证；root 登录不可用，因为 `/root` 对 virtiofsd
不可读。

virtme VM 同时启用 `virtme` 和 `vsock` 后，普通的 `ssh` action （`ge` / `gm`
别名）默认就走 vsock，不再需要显式指定 `ssh_vsock`。

## initramfs 内核模块匹配

`collei/scripts/virtme.py` 生成 initramfs 时会从 `config.ini` 中 `kernel` 指向的
构建树复制 virtio/vsock 模块。内核版本从实际启动的 bzImage/Image 提取
（`collei/scripts/kernel.py`），而不是 `include/config/kernel.release` ——
后者可能被后续的 make 调用重新生成，与尚未重编的 bzImage/模块不一致。
复制的模块会去掉 `.BTF` 段，避免增量构建树中 "failed to validate module BTF:
-22" 导致的 insmod 失败。

initramfs 有缓存：输入（loader、Rust init、busybox、模块 .ko、machine/vsock
选项）没变化时直接复用上次的 `virtme-initramfs.cpio.zst` （同目录 `.stamp`
文件记录输入指纹），否则重新打包。模块定位对整个 构建树只做一次遍历，打包走外部
`zstd -1`——之前每次启动逐模块 rglob 加 Python gzip level 9 压缩要在 QEMU
启动前白等 ~4s。

## guest 里看到全部 ko（.mod 目录）

`build/build.sh` 编译内核时已经执行了
`make modules_install INSTALL_MOD_PATH=<kernel>.mod INSTALL_MOD_STRIP=1`， 所以
`<kernel>.mod/lib/modules/<kver>/` 就是一份完整、带 depmod 依赖表的
模块安装结果。collei 直接复用它：`virtme.py` 把
`virtme_link_mods=<kernel>.mod/lib/modules/<kver>` 传给 guest init，
`/lib/modules/<kver>` 指向该目录后 `modinfo`/`modprobe`/`lsmod` 直接可用 （kver
从实际启动的内核镜像提取，见上一节）。

不需要 collei 再像 virtme-ng 的 `virtme-prep-kdir-mods` 那样自己构造
`.virtme_mods` 符号链接目录。注意路径同样依赖 host `/` 被整体共享 （默认
`share_root=/`）。

## drgn / 调试信息（vmlinux）

`prepare_debuginfo()` 会在 `<kernel>.mod/lib/modules/<kver>/` 下恢复
`build -> 构建树` 的符号链接（`make modules_install` 默认会创建，
`build/build.sh` 出于打包考虑把它删了）。这样 guest 里的 drgn 通过 标准搜索路径
`/lib/modules/<kver>/build/vmlinux` 就能找到带 DWARF 的 vmlinux，配合 cmdline
里的 `nokaslr` 和 guest 的 `/proc/kcore`， 直接可以调试活内核：

```bash
sudo drgn /home/martins3/data/vn/docs/kernel/tutorial/drgn/scripts/shmem_usage.py
```

已知限制：`.mod` 里的 .ko 经过 `INSTALL_MOD_STRIP=1` strip，drgn 会提示 "missing
debugging symbols for <module>"（不影响 vmlinux 本身的分析）。
如果脚本需要模块的调试信息，对应 .ko 在构建树里的原始文件是未 strip 的。

## sudo 支持

guest 共享 host rootfs 时 sudo/su 有三层障碍（virtiofsd 以普通用户运行）：

1. Fedora 的 `/usr/bin/sudo` 是 `---s--x--x` (4111)，virtiofsd 无法读取， guest
   里 exec 直接 permission denied（Ubuntu 是 4755，所以 virtme-ng
   不会遇到这个问题）
2. `/etc/sudoers`、`/etc/sudo.conf` 是 root-only，sudo 读不到配置
3. `/etc/shadow` 不可读，su 和 PAM 账户检查失败

参考 virtme-ng 的 `generate_sudoers()`/`generate_shadow()`，Rust init 做了：

- 生成新的 `/etc/sudoers`（root 和 virtme_user 均 NOPASSWD）bind-mount 覆盖
- 生成全空密码的 `/etc/shadow` bind-mount 覆盖（等价 `--empty-passwords`， `su`
  因此可用；Fedora PAM 默认带 nullok）
- 生成最小 `/etc/sudo.conf` bind-mount 覆盖
- tmpfs 遮住 `/var/db/sudo`（或 `/var/lib/sudo`）；`/root` 不可读时挂 tmpfs
- `/usr/bin/sudo` 用 host 侧准备的 4755 副本 bind-mount 覆盖： `virtme.py` 的
  `prepare_sudo()` 在启动时用 `sudo install -m 4755` 把 `/usr/bin/sudo` 复制到
  `vm/<name>/s/sudo.bin`（只在副本缺失或 host sudo
  更新时执行，不会每次启动都要密码），路径通过 `virtme_sudo_bin=` 传给 init

## Rust init 支持

`virtme/virtme-ng-init` 从 virtme-ng 的 Rust init 移植而来。启动分两阶段：

1. `virtme-init-loader.sh` 在 initramfs 中加载 virtiofs、overlay、vsock 等模块，
   只负责挂载 `ROOTFS`；
2. loader 把自动构建的 `virtme-ng-init.out` 放进 guest 私有 `/tmp`，再通过
   `switch_root` 将它启动为 PID 1；
3. Rust init 挂载 proc/sys/dev/cgroup、建立 overlay 和模块链接、修复系统文件，
   并启动服务与用户会话。

`VirtmeSetup._build_rust_init()` 按源码时间戳增量执行 Cargo release build，
最终打包的 binary 以 `.out` 结尾。Rust init 在真实 rootfs 生效后才执行，
因此不要求 host 安装 glibc-static。

## udev coldplug

Rust init 的 `run_udevd()` 会启动 `systemd-udevd`，依次触发 subsystem/device
coldplug 并等待 settle。initramfs 只硬编码“挂载 ROOTFS 前必须加载”的模块；
virtio-blk、virtio-scsi、virtio-net 等运行期设备由 udev 自动加载和命名。

所以，现在 lsmod 可以观察 intel_kvm

## virtme-ng 调研

### Links
- https://github.com/arighi/virtme-ng
- https://github.com/amluto/virtme : 之前是这个项目
  - http://arighi.blogspot.com/
      - 作者的 blog
- https://virtio-fs.gitlab.io/howto-boot.html
- https://lwn.net/Articles/951313/

### 细节
启动 QEMU 脚本

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

配合 virtme-ng.nix

```txt
sudo yum install busybox
vng -r ~/data/kernel/linux-build --dry-run
```

### console
```txt
  --console [PORT]      Enable a server to communicate later from the host using '--console-client'. By default, a simple console will be offered using a VSOCK connection, and 'socat' for the proxy.
  --console-client [PORT]
```
并不存在 vsock 后端可以作为 console ，其实是 guest 中也启动了一个 socat ，
然后 host 也启动了一个 socat

### network

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
