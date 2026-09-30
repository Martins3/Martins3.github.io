# udev 机制深度解析

udev（userspace /dev）是 Linux 系统的用户空间设备管理器。它的核心职责是：**动态管理 `/dev` 目录下的设备节点文件，响应内核发出的设备热插拔事件，并根据可配置的规则执行相应的用户空间动作**。

简单来说，udev 回答了这样一个问题：当系统检测到一个新硬件（如插入 U 盘、识别到新网卡）时，谁负责在 `/dev` 下创建对应的设备文件？谁决定这个文件该叫什么、权限该设成什么、还要触发哪些后续操作？答案就是 udev。


## 一、背景：为什么需要 udev

### 1.1 静态 /dev 的困境

早期 Linux 系统在 `/dev` 下静态存放所有可能的设备节点文件。`/dev` 目录预先包含成千上万个设备文件，如 `/dev/sda` 到 `/dev/sdz`、`tty0` 到 `tty63` 等。这种方式存在明显缺陷：

- **浪费**：系统中实际存在的设备可能只有几十个，但 `/dev` 下却有几千个节点
- **major/minor 耗尽**：静态分配的设备号数量有限（早期 major 号只有 255 个），难以支撑大量设备
- **无法感知动态变化**：热插拔设备插入后，系统无法自动创建对应节点

### 1.2 devfs 的失败

Linux 2.4 引入了 devfs，试图在内核空间动态管理设备节点。但 devfs 存在严重问题：

- **策略混入内核**：设备命名、权限控制等策略逻辑被迫写在内核中，不够灵活
- **race condition**：设备节点创建时序问题难以解决
- **维护困难**：内核空间调试复杂，设备命名规则硬编码

devfs 最终被废弃，Linux 社区转向用户空间方案——udev。

### 1.3 udev 的设计哲学

udev 遵循**"机制在内核，策略在用户空间"**的设计原则：

- **内核**只负责：检测设备、初始化驱动、在 sysfs 中暴露设备信息、通过 netlink 发送事件通知
- **udev（用户空间）**负责：决定设备叫什么、权限如何、创建什么符号链接、触发什么外部程序

这使得设备管理策略可以完全通过配置文件调整，无需修改内核或重启系统。

## 二、udev 的核心组成

现代 Linux 发行版中，udev 已集成到 systemd，主要包含以下组件：

| 组件 | 说明 |
|------|------|
|`systemd-udevd`|udev 守护进程，监听内核事件并处理 |
|`libudev`|用户空间库，供应用程序查询设备信息 |
|`udevadm`|管理调试工具，用于测试规则、监控事件、查询设备属性 |
|`/lib/udev/rules.d/`|系统默认规则目录 |
|`/etc/udev/rules.d/`|用户自定义规则目录（优先级更高） |

## 三、核心机制：udev 如何工作

udev 的运作依赖于三个关键机制：**sysfs 设备信息树、netlink 事件通知、规则引擎**。

### 3.1 sysfs：设备的"户口本"

sysfs 是一个虚拟文件系统（挂载于 `/sys`），由内核维护，以文件和目录的形式暴露系统中所有设备的层次结构、属性和关系。每个设备在 sysfs 中都有对应路径，例如：

```
/sys/devices/pci0000:00/0000:00:1d.1/usb2/2-1/2-1:1.0/host2/target2:0:0/2:0:0:0/block/sda
```

该路径下的文件包含丰富的设备属性：

- `vendor`、`device`：厂商和设备 ID
- `modalias`：用于模块自动加载的标识字符串
- `size`、`queue/scheduler`：块设备特定属性
- `uevent`：可触发或查看该设备的事件记录

udev 在处理设备时，会读取 sysfs 中的属性来匹配规则。

### 3.2 netlink / uevent：内核到用户空间的"电话线"

当内核检测到有设备被添加、移除或状态变化时，会通过 **netlink socket**（协议族 `NETLINK_KOBJECT_UEVENT`）向用户空间广播一个 **uevent** 消息。典型的 uevent 内容如下：

```
add@/devices/pci0000:00/0000:00:1d.1/usb2/2-1
ACTION=add
DEVPATH=/devices/pci0000:00/0000:00:1d.1/usb2/2-1
SUBSYSTEM=usb
BUSNUM=002
DEVNUM=002
TYPE=0/0/0
PRODUCT=1234/5678/100
...
```

关键字段含义：

- `ACTION`：事件类型，`add`（添加）、`remove`（移除）、`change`（状态变化）、`move`（设备路径变化）
- `DEVPATH`：设备在 sysfs 中的路径
- `SUBSYSTEM`：设备所属子系统（usb、pci、block、net 等）

`systemd-udevd` 守护进程启动时会创建一个 netlink socket 并监听该协议族，实时接收内核发来的 uevent。

### 3.3 devtmpfs：提前准备"毛坯房"

现代 Linux 内核在启动早期会挂载 `devtmpfs`（一个基于 tmpfs 的文件系统）。内核在发现设备时，会**自动**在 devtmpfs 中创建基本的设备节点（确定 major/minor）。这确保了：

- 即使在 udev 尚未启动时，关键设备（如根文件系统所在的磁盘）也已有 `/dev` 节点可用
- udev 启动后，可以在这些已有节点上**修改权限、创建符号链接、触发额外动作**，而不是从零创建节点

devtmpfs 解决了早期 initramfs 阶段的"鸡生蛋"问题。

### 3.4 规则引擎：udev 的"大脑"

收到 uevent 后，`systemd-udevd` 的核心工作就是**匹配规则并执行动作**。规则文件以 `.rules` 为后缀，按字典序加载。

#### 规则匹配条件

规则通过设备的属性进行匹配，常用匹配键包括：

| 匹配键 | 说明 |
|--------|------|
|`ACTION`|匹配事件类型（add/remove/change） |
|`SUBSYSTEM`|匹配子系统 |
|`KERNEL`|匹配内核设备名 |
|`ATTR{file}`|匹配 sysfs 中某属性文件的值 |
|`ENV{key}`|匹配环境变量 |
|`PROGRAM`|执行外部程序，根据返回值判断是否匹配 |

#### 规则执行动作

匹配成功后，可以执行的动作包括：

| 动作键 | 说明 |
|--------|------|
|`NAME="xxx"`|设置设备节点名称 |
|`SYMLINK+="xxx"`|创建指向该设备的符号链接 |
|`OWNER="user"`|设置设备文件所有者 |
|`GROUP="group"`|设置设备文件所属组 |
|`MODE="0660"`|设置设备文件权限 |
|`RUN+="/path/to/program"`|执行外部命令 |
|`ENV{key}="value"`|设置环境变量 |

#### 规则示例

```udev
# 让某个 USB 设备对所有用户可读写
SUBSYSTEM=="usb", ATTR{idVendor}=="1234", ATTR{idProduct}=="5678", MODE="0666"

# 为 NVMe 盘创建基于序列号的持久化符号链接
SUBSYSTEM=="block", ENV{DEVTYPE}=="disk", ATTRS{serial}=="?*", \
    SYMLINK+="disk/by-nvme-serial/$env{ID_SERIAL_SHORT}"

# PCI 设备出现时自动加载驱动并绑定
SUBSYSTEM=="pci", ATTRS{vendor}=="0x1d94", ATTRS{device}=="0x1456", \
    RUN+="/bin/sh -c 'modprobe hct; echo $id > /sys/bus/pci/drivers/hct/bind'"
```

## 四、完整工作流程：从插入 U 盘到桌面弹出图标

以插入一个 USB 存储设备为例，展示 udev 的完整工作链：

```
[USB 设备插入]
        |
        v
[USB Host Controller 检测到电气信号变化]
        |
        v
[内核 USB 核心层枚举设备，加载对应驱动（如 usb-storage）]
        |
        v
[内核在 sysfs 中创建设备目录 /sys/devices/.../usb2/2-1/.../block/sda]
        |
        v
[内核通过 netlink 广播 uevent (ACTION=add, SUBSYSTEM=block)]
        |
        v
[systemd-udevd 接收到 uevent]
        |
        v
[udev 根据 DEVPATH 读取 sysfs 属性]
        |
        v
[udev 依次匹配 /lib/udev/rules.d/ 和 /etc/udev/rules.d/ 中的规则]
        |
        v
[udev 执行匹配规则的动作：
  - 确保 /dev/sda 节点存在（利用 devtmpfs 已创建的节点）
  - 设置 OWNER/GROUP/MODE
  - 创建 /dev/disk/by-uuid/xxx、/dev/disk/by-label/xxx 等符号链接
  - 触发 /lib/udev/ 下的辅助程序（如 scsi_id、blkid）收集额外属性
  - 通过 D-Bus 或 socket 通知其他用户空间程序（如桌面环境）]
        |
        v
[桌面环境（GNOME/KDE）收到通知，弹出"新卷已挂载"提示]
```

-
## 五、持久化设备命名

现代 Linux 系统不鼓励直接使用 `/dev/sda`、`/dev/eth0` 这类内核动态分配的名称，因为：

- 设备探测顺序可能随启动变化（特别是 USB/SATA 设备）
- 同一插槽更换设备后，希望名称不变

udev 通过规则创建**持久化符号链接**解决此问题：

| 目录 | 用途 |
|------|------|
|`/dev/disk/by-uuid/`|按文件系统 UUID 链接 |
|`/dev/disk/by-label/`|按文件系统标签链接 |
|`/dev/disk/by-id/`|按设备硬件 ID 链接 |
|`/dev/disk/by-path/`|按物理拓扑路径链接 |
|`/dev/input/by-id/`|输入设备的持久化链接 |
|`/dev/net/by-addr/`|网络设备（较少使用） |

例如，`/etc/fstab` 中推荐使用 `UUID=xxx` 或 `/dev/disk/by-uuid/xxx`，而非 `/dev/sda1`。

### 可预测网络接口命名

udev/systemd 还实现了 **Predictable Network Interface Names**，将 `eth0`、`wlan0` 等名称替换为基于物理位置或 MAC 地址的名称（如 `enp3s0`、`wlp2s0`），避免多网卡服务器重启后网卡名错乱。

## 六、模块自动加载与 modalias

udev 与内核模块自动加载机制紧密配合。每个设备在 sysfs 中都有一个 `modalias` 文件，包含该设备的硬件标识：

```bash
$ cat /sys/devices/pci0000:00/0000:00:06.0/0000:02:00.0/modalias
pci:v00001E49d00000041sv00001E49sd00000041bc01sc08i02
```

内核在发送 uevent 时会携带 `MODALIAS` 环境变量。udev 规则中有专门触发模块加载的机制：

```udev
# 80-drivers.rules 中的典型规则
SUBSYSTEM=="pci", ENV{MODALIAS}=="?*", RUN{builtin}+="kmod load"
```

这会自动调用 `modprobe` 并根据 `/lib/modules/$(uname -r)/modules.alias` 中的映射关系加载匹配的驱动模块。因此，当你插入一个新硬件时，驱动模块的加载往往是由 udev 触发的。

## 七、调试与诊断

udev 提供了强大的调试工具 `udevadm`：

```bash
# 实时监控 uevent
udevadm monitor --property --udev

# 查询某个 sysfs 设备的全部属性和规则匹配过程
udevadm info --attribute-walk --path=/sys/class/block/sda

# 模拟对某个设备执行规则处理（不实际修改系统）
udevadm test /sys/class/block/sda

# 重新加载规则
udevadm control --reload-rules

# 触发某个设备的 uevent（重新走一遍规则）
udevadm trigger --action=change --sysname-match=sda
```

## 八、udev 与 systemd 的融合

在早期（udev 独立项目时代），`udevd` 是一个独立的守护进程。如今：

- `systemd-udevd` 是 systemd 的一部分，但可以在无 systemd 的系统上编译使用
- udev 事件可以触发 systemd 服务单元（通过 `SYSTEMD_WANTS` 等环境变量）
- systemd 的 `.device` 单元会根据 udev 规则生成的状态进行依赖管理

例如，systemd 的 `local-fs.target` 可以依赖某个 `.device` 单元，而该单元只有在 udev 完成对应块设备的规则处理后才会标记为"ready"。

## 九、总结

| 问题                           | udev 的答案                                |
|--------------------------------|--------------------------------------------|
| 谁创建 `/dev` 下的设备文件？   | devtmpfs 创建基础节点，udev 调整权限和链接 |
| 内核如何通知用户空间有新设备？ | 通过 netlink socket 发送 uevent            |
| 设备文件该叫什么名字？         | 由 udev 规则决定，支持持久化命名           |
| 插入硬件后驱动怎么自动加载？   | udev 根据 modalias 触发 `modprobe`         |
| 如何自定义设备权限/所有者？    | 编写 udev 规则                             |
| 桌面如何知道插入了 U 盘？      | udev 规则触发辅助程序，通过 D-Bus 通知桌面 |

udev 是现代 Linux 设备管理的基石。它把设备管理的**策略**完全放到用户空间，使系统管理员无需触碰内核代码，就能精确控制每一个硬件在系统中的表现形式。理解 udev，是理解 Linux 设备模型、驱动加载、热插拔和容器/虚拟化设备透传等高级话题的前提。

## 参考

- [Writing udev rules](https://www.reactivated.net/writing_udev_rules.html)
- [SUSE udev documentation](https://documentation.suse.com/sles/12-SP5/html/SLES-all/cha-udev.html)
- [Arch Wiki - udev](https://wiki.archlinux.org/title/Udev)
- [LWN - devtmpfs](https://lwn.net/Articles/331818/)
- [Linux kernel: drivers/base/devtmpfs.c](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/base/devtmpfs.c)

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
