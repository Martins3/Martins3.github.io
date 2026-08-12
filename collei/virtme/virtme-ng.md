# virtme-ng 调研
- https://github.com/arighi/virtme-ng
- https://github.com/amluto/virtme : 之前是这个项目
  - http://arighi.blogspot.com/
      - 作者的 blog
- https://virtio-fs.gitlab.io/howto-boot.html
- https://lwn.net/Articles/951313/

## 启动 QEMU 脚本

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

## 基本操作

配合 virtme-ng.nix

```txt
sudo yum install busybox
vng -r ~/data/kernel/linux-build --dry-run
```


## console
```txt
  --console [PORT]      Enable a server to communicate later from the host using '--console-client'. By default, a simple console will be offered using a VSOCK connection, and 'socat' for the proxy.
  --console-client [PORT]
```
并不存在 vsock 后端可以作为 console ，其实是 guest 中也启动了一个 socat ，然后 host 也启动了一个 socat

## network 配置

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

## TODO
virtme/commands/configkernel.py : 参考一下他的 kernel 制作

3. 使用 rust 的 init ，是如何做的

4. **balloon** | 内存气球 | `--balloon` |

- `--systemd` 作为 init：实验性。

1. virtio-serial 退出码通道（exec 模式的核心短板）
3. guest 侧 DHCP（修 network 选项的现存 bug）

# virtme-ng initramfs 机制调研与 collei 对比
<!-- cccb7e01-8c20-4d10-ae5a-887cd811b804 -->

2026-07-24 调研。源码基准：`~/data/virtme-ng` 本地 clone。
结论先行：virtme-ng 的 initramfs 几乎什么都不做，而且是刻意如此；
真正的系统初始化发生在 switch_root 之后、运行在共享 host rootfs 上的第二阶段 init 里。

## 一、initramfs 生成路径

### 1.1 三种启动路径（`virtme/commands/run.py:1328-1338, 2066-2134`）

| 路径 | 触发条件 | 说明 |
|---|---|---|
| 无 initramfs，内核直接挂根 | **默认**。virtiofs/9p 编成 built-in | cmdline 直接 `rootfstype=virtiofs root=ROOTFS init=...`（run.py:2114-2134） |
| 自带 busybox initramfs | `--force-initramfs`，或 QEMU<=1.5 不能 overmount virtfs（qemu_helpers.py:44-46），或根文件系统驱动是模块（`kernel.modfiles` 非空） | run.py:2068-2103 调 `mkinitramfs.mkinitramfs()` |
| dracut/mkinitcpio/预编译 initramfs | **不存在** | 全仓库 grep `dracut\|mkinitcpio` 零命中；下载预编译的只有内核（mainline.py 的 KernelDownloader，不含 initramfs） |

关键理念：virtme-ng 靠 `virtme-configkernel` 引导用户把文件系统驱动编成
built-in 来**消灭** initramfs；initramfs 只是"模块型内核"和"测试 initramfs
路径"（`--force-initramfs`）的兜底。collei 固定走 initramfs，换来代码路径唯一、
模块不必编进内核。

### 1.2 initramfs 的内容（`virtme/mkinitramfs.py`）

纯 Python `cpiowriter.py` 写 newc cpio（未压缩，不依赖外部 cpio/gzip）：

- 目录骨架 + `bin->sbin`、`lib->lib64` 软链（:16-33）
- dev 节点仅 3 个：`null`、`kmsg`、`console`（:36-39）
- 静态 busybox（run.py:2079-2085 用 `file` 强制检查 static）+ 10 个 applet
  软链：`sh mount umount switch_root sleep mkdir mknod insmod cp cat`（:42-60）
- 一个假 modprobe（只报错并 exit 1，:73-85）
- 模块全部平铺在 `/modules/`（.zst 先解压），生成按拓扑序逐个 insmod 的
  `load_all.sh`（:88-107）
- `/init` 脚本（:110-153）

没有 udev、没有 kmod、没有 modalias、没有任何多余初始化。

### 1.3 /init 只做 4 步（mkinitramfs.py:110-153）

1. `source /modules/load_all.sh` 按序 insmod；
2. `mount -t virtiofs ROOTFS /newroot`，失败回退 9p；
3. 探测 virtfs overmount 能力（QEMU 1.5 bug），临时挂 /proc 解析 cmdline 的 `init=`；
4. `exec switch_root /newroot $init`。

### 1.4 模块选择机制

- 选哪些：`virtmods.MODALIASES` 硬编码 modalias 列表（`virtme/virtmods.py:8-28`）：
  `fs-9p`、`fs-virtiofs`、9pnet/virtconsole 的 virtio/PCI modalias、`virtio_pci`、
  `virtio_mmio`（microvm）、`unix`（udev）、`i8042`、`atkbd`；用 overlay 时追加 `"overlay"`。
- 怎么解析依赖：对每个 alias 在 **host 上**跑
  `modprobe --show-depends -C /var/empty [-d root] [-S kver]`，合并去重得拓扑序
  （`virtme/modfinder.py:24-69`）。不靠 lsmod、不扫 /sys、不放全部模块。
- 运行期 /lib/modules 三态（`mount_kernel_modules`，virtme-init:78-101 / main.rs:554-573）：
  - `virtme_root_mods=1`：rootfs 自带，不动；
  - `virtme_link_mods=<path>`：tmpfs 挂 /lib/modules + 软链（collei 已采用此方案，
    直接指向 build.sh modules_install 产出的 `.mod` 目录）；
  - 都没给但 /lib/modules/<kver> 存在：`ro,mode=0000` tmpfs **遮蔽** host 的不匹配
    模块，防误加载。

initramfs 里只放启动必需的少量 .ko；运行期模块靠 guest 内 udev + modprobe
经共享 /lib/modules 按需加载。

## 二、第二阶段 init（真正的初始化）

两个实现：bash 版 `virtme/guest/virtme-init`（745 行）和 Rust 版
`virtme/guest/bin/virtme-ng-init`（`virtme_ng_init/src/main.rs`，约 1300 行，
纯为启动速度）。native 架构默认 Rust 版，`--no-virtme-ng-init` 回退 bash。
步骤一致（main.rs:1269-1300 / virtme-init:715-745）：

1. 设 PATH。
2. 挂内核文件系统：proc / sys / /run(tmpfs) / devtmpfs / configfs / debugfs /
   tracefs / securityfs（main.rs:48-105）。
3. 设 hostname（`virtme_hostname` cmdline）。
4. 挂 cgroup2（`SYSTEMD_CGROUP_ENABLE_LEGACY_FORCE=1` 时模拟 systemd 挂 v1 全家桶）。
5. 传播 host 的 `nr_open` 限制（run.py:1376-1382）。
6. 对 `virtme_rw_overlayN` 建 overlay（vng 默认注入 /etc /lib /home /opt /srv
   /usr /var /tmp，upper/work 在 /run/tmp）。
7. 挂 devpts、/dev/shm、/var/log、/var/tmp 等一堆 tmpfs（main.rs:107-192）。
8. `mount_kernel_modules`（见 1.4 三态）。
9. `systemd-tmpfiles`（如果有）。
10. `virtme.vsockexec=` 时起 socat vsock console server。
11. **并行**三件事（main.rs:1286-1292）：
    - `run_udevd`：systemd-udevd --daemon → udevadm trigger（coldplug）→ settle；
    - `run_misc_services`：/dev/fd 软链、挂 --rodir/--rwdir 的 9p、修 dpkg locks、
      override_system_files（空 fstab、影子 shadow、NOPASSWD sudoers、hosts、
      假 /etc/lvm）、run_sshd、run_snapd；
    - `setup_network`：lo up；有 `virtme.dhcp` 时遍历 virtio_net 接口，
      `busybox udhcpc -s virtme-udhcpc-script` 配 IP/路由/DNS（resolv.conf 用
      bind-mount 绕过只读 rootfs，virtme-udhcpc-script:26-41）。
12. `virtme_chdir` 切工作目录。
13. 用户会话：解析 `virtme_user`、XDG_RUNTIME_DIR、非 root 用户时 /root 隔离；
    有 `` virtme.exec=`base64` `` 则经 **virtio-serial 端口** `virtme.stdin/stdout/stderr`
    执行脚本，退出码写 `virtme.ret` 端口回传 host，然后 poweroff
    （main.rs:799-879）；否则交互模式：chown console、stty 同步、
    `setsid su [-s $virtme_shell] [-- $virtme_user]`。
14. `poweroff -f`。

guest 工具**不复制进 guest**：root=`/` 时 `init=` 直接指向共享根里的
`virtme/guest/` 目录（run.py:1531）。

### 配置传递机制

全部走 kernel cmdline：`virtme_foo=bar`（下划线类被内核转成 init 环境变量）+
`virtme.foo`（点号类从 /proc/cmdline grep）+ base64 的 `` virtme.exec= ``。
另有 cmdline 超长保护（s390x 896 字节限制，落成临时脚本，run.py:2148-2161）。

理念差异：virtme-ng 无持久 VM 定义，"命令行即配置"；collei 是具名持久 VM +
opt/ 选项文件。

./vng -r ~/data/kernel/linux-build --systemd
- **udev coldplug**（virtme-init:251-277 / main.rs:650-668）：guest 内按需自动
  加载模块、生成 /dev/disk/by-id，替代 init 里硬编码 modprobe（virtme-init.sh:336-341）。

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
