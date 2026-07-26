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

### 与 collei 的架构差异

collei 把系统文件修复（sudoers、shadow）、vsock-ssh 服务生成等工作前移到
initramfs 的 busybox sh 里，initramfs 反而比 virtme-ng 的"重"。两条路线都成立：
- virtme-ng：initramfs 极简，依赖共享 rootfs 上的完整工具链做第二阶段初始化；
- collei：initramfs 自包含，对 share_root 不是完整系统的场景更稳。

## 三、collei 缺失功能清单

collei 已有（不重复列）：opt/ VM 定义、virtiofs 根、overlay、9p、vsock ssh、
exec 模式、user 切换、sudo 支持、`.mod` 模块目录映射、drgn/debuginfo、QGA。

### 值得抄

- **guest 侧 DHCP（现存 bug）**：collei `scripts/virtme.py:108` 已传 `virtme.dhcp`，
  但 init 里只 `ip link set lo up`，udhcpc 逻辑缺失——`network` 选项目前是断的。
  virtme-ng 做法：busybox udhcpc + virtme-udhcpc-script（virtme-init:345-373）。
- **udev coldplug**（virtme-init:251-277 / main.rs:650-668）：guest 内按需自动
  加载模块、生成 /dev/disk/by-id，替代 init 里硬编码 modprobe（virtme-init.sh:336-341）。
- **virtio-serial 脚本 I/O + 返回码通道**（run.py:1802-1898, main.rs:799-879）：
  exec 输出走独立端口不污染 console，退出码经 `virtme.ret` 精确回传 host；
  collei 的 `/run/tmp/virtme-exit-code` 写在 guest tmpfs 里 host 拿不到。
- **`modprobe --show-depends` 模块解析**（modfinder.py）：替代 `_copy_modules`
  硬编码模块名表 + 手工 vermagic 匹配（scripts/virtme.py:361-445），自动处理
  依赖和压缩模块。
- **microvm 自动切换**（run.py:1005-1011, 1448-1477）：x86_64+KVM+无 NUMA 时
  `-M microvm` 显著加快启动。
- **遮蔽不匹配模块目录**：没给模块时 `mode=0000` tmpfs 盖住 /lib/modules/<kver>，
  防误加载 host 内核模块（main.rs:569-572）。
- **sshd inetd over vsock**（virtme-sshd-script:89-104，`systemd-socket-activate
  --inetd` + host 侧 ProxyCommand）：比 python 端口转发更稳，天然支持 scp/sftp。
- **vsock console server/client**（`--console`/`--console-client`，run.py:1070-1163）：
  socat VSOCK-LISTEN -> EXEC:...,pty，免 sshd 的远程 shell。
- **额外磁盘** `--disk`（virtio-scsi）/`--blk-disk`（virtio-blk），固定 serial
  便于 by-id 定位（run.py:1748-1776）。
- **NUMA** `--numa`/`--numa-distance`（run.py:1384-1404）与 **vCPU 绑核** `--pin`
  （QMP query-cpus-fast + sched_setaffinity）。
- **`--rodir/--rwdir/--overlay-rwdir`** 额外目录挂载（virtme_initmountN 9p）。
- **QMP 内存转储** `--debug` + `--dump`（dump-guest-memory），与 drgn 支持互补。
- **调试选项族**：`--show-command`、`--dry-run`、`--save-initramfs`、
  `--show-boot-console`、hvc0 内核日志重定向 host stderr（run.py:1673-1695）。
- 小环境细节：nr_open 传播、TERM 传播、stty 行列同步、root home tmpfs 遮蔽、
  `-echr 1`、`psmouse.proto=exps`、quiet/loglevel 分级。

### 可有可无

- `--force-9p`：9p 回退开关（collei 已有 9p 能力）。
- `--empty-passwords`：collei shadow 已全空密码，等价已实现。
- `--balloon`、`--sound`（pipewire）、`--graphics`/xinit GUI 会话。
- `--snaps`/snapd：Ubuntu 生态专用。
- `--systemd` 作为 init：实验性。
- mainline 内核下载、`--build-host` 远程构建、`--kconfig`：与 collei build/ 体系重叠。
- dpkg locks 修复、/etc/lvm 伪装：Debian 系特定痛点。
- MCP server、argcomplete 补全、spinner：外围体验。

### 不适合 collei

- `--arch` 跨架构 + `--root` Ubuntu cloud image chroot：virtme-ng 面向"任意架构
  内核测试"，collei 是围绕 host 系统快照的工作流，理念不同。
- 无 initramfs 直通 `root=` 模式：要求文件系统驱动 built-in，collei 固定
  initramfs 路径更统一。
- `.virtme_mods` 符号链接农场 + `--mods use/auto`：collei 的 `.mod` 目录映射
  效果相同且更简单（还顺带给 drgn 提供 build 软链）。
- Rust 版 virtme-ng-init：纯启动速度优化，引入 Rust 工具链不值。
- host rootfs 直接当 init 载体（init= 指向共享目录里的 guest tools）：collei
  initramfs 自包含，对 share_root 不是 / 的场景更稳。

## 四、建议优先级

1. virtio-serial 退出码通道（exec 模式的核心短板）
2. udev coldplug（按需加载模块，摆脱硬编码）
3. guest 侧 DHCP（修 network 选项的现存 bug）
4. microvm（启动提速）

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
