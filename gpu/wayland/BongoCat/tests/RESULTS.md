# 验证记录

2026-10-15，本机 Linux x86_64、Rust 1.98.0、GNOME Shell/Mutter 50.5。
使用独立 D-Bus 会话中的 GNOME Wayland 合成器，虚拟屏幕 1280×720，
Intel i915 渲染。测试没有重启主桌面，但配置隔离失败，误改了主桌面的扩展
启用列表；详见下文「GNOME 托盘图标消失事件」。

## 自动检查

- `cargo fmt --check`、`cargo clippy --all-targets --locked -- -D warnings` 通过。
- 6 个普通测试通过：位图按键状态、组合键与多设备、重复事件、键鼠组合、
  短点击及延迟出帧、图集帧选择与预乘透明度、CLI 无效参数（部分场景合并在同一测试中）。
- 两个忽略的 Wayland 集成测试已显式执行通过，不只是编译：每项运行 60 秒。
- release 构建通过；安装脚本通过 ShellCheck、shfmt 和 Bash 语法检查。
- 真实设备权限不足时，CLI 正确报告设备路径及 `Permission denied` 并退出。

## 旧四帧实现的历史性能

默认 306×177 逻辑像素，release，窗口可见。CPU 以单核 100% 计，
通过 `/proc/self/stat` 的进程 CPU ticks 计算；测试程序包含一个 Rust 测试线程，
实际应用事件循环为单线程。

| 场景 | 时间 | 提交画面 | CPU | 结束时 RSS | 峰值 RSS |
| --- | ---: | ---: | ---: | ---: | ---: |
| 持续键鼠状态输入 | 60 s | 2398 | 0.1333% | 5776 KiB | 11344 KiB |
| 独立进程空闲复测 | 60 s | 1 | 0.0000% | 5768 KiB | 11772 KiB |

空闲的 0% 表示低于本次 CPU tick 计时精度，并非严格零开销。
持续输入测试每 10 ms 驱动一次测试定时器，每 100 ms 产生一组键盘与鼠标
按下/释放；运行时本身没有该定时器。连续在同一测试进程执行两个用例时，
分配器保留内存使空闲 RSS 达到 12372 KiB，仍低于 20 MiB 目标。

独立 GNOME Shell 进程在扩展启用/禁用后各测 15 秒空闲 CPU，两次均为
0.1333%。这是短时观测，不能证明扩展完全没有开销；应用的 CPU 数值不包含合成器。

## 桌面交互

- 窗口实际矩形 `(958, 527, 306, 177)`，主屏右下角各留 16 逻辑像素。
- `above`、跨工作区和 `skip_taskbar` 均为 true。
- 禁用扩展后恢复原矩形 `(487, 288, 306, 177)` 及原窗口属性，重新启用恢复挂件位置。
- 检查实际截图：透明背景、四姿态猫咪、无窗口装饰。
- 在猫咪背后放置 GTK 测试按钮，通过测试合成器的虚拟指针点击猫咪区域，
  下层按钮收到点击并打印 `CLICK_PASSED_THROUGH`。
- 在下层窗口已聚焦时启动猫咪，最终焦点仍为下层窗口。
- 工作区切换后猫咪保持跨工作区属性。

测试合成器临时开启了调试 Eval，修改仅在临时测试副本中，未加入交付扩展。

## 尚未实测

本终端没有免密 sudo，真实键鼠 event 节点不可读，因此未实测物理设备输入、
热拔插、真实高速鼠标移动及 `SYN_DROPPED` 溢出恢复。已提供仅匹配本机两个
指定设备的 udev 规则和运行步骤。事件过滤 ioctl 的 ABI 和过滤行为已对照
本机 Linux `input.h`、`drivers/input/evdev.c` 核实。

未进行真实多屏、HiDPI/分数缩放及锁屏交互测试。不得将状态机注入测试
描述为真实 evdev 端到端性能测试。

## 后续硬件验证与修复

用户为两个设备添加读取 ACL 后，实际启动暴露了 `EVIOCSMASK` 的 ABI 错误：
原实现传入 4 字节位图，64 位内核的 `bits_from_user()` 要求长度为原生
`unsigned long` 大小的整数倍，因此返回 `EINVAL`。此前仅静态核对是不充分的。

位图现改为 `c_ulong`，长度从类型大小取得。新增 `live_evdev_mask_round_trip`
集成测试，已分别在本机 ROYUAN 键盘和 Compx VGN 鼠标上执行通过：设置掩码成功，
读回仅启用 SYN/KEY，独立打开的另一个读取者保持原掩码。错误提示现在只在
`PermissionDenied` 时引导检查权限，并给事件过滤错误加上 `EVIOCSMASK` 上下文。
真实设备热拔插、溢出恢复和高速移动的性能测量仍未完成。

## GNOME 托盘图标消失事件

2026-10-15，GNOME Shell/Mutter 50.5，Wayland。以下时间均为北京时间。
用户注销并重新登录后，发现微信和 Flameshot 的托盘图标消失。
根因是 BongoCat 集成测试误写了真实用户的 dconf 配置，禁用了 AppIndicator
等原有扩展；本次证据不指向 GNOME 自身故障或 BongoCat 运行时的窗口管理逻辑。

### 时间线与证据

- 17:39 的测试前检查记录显示，原来启用了
  `BingWallpaper@ineffable-gmail.com`、`kimpanel@kde.org` 和
  `appindicatorsupport@rgcjonas.gmail.com`。
- 18:33 的测试先启动 `dbus-run-session -- python3`，随后才在 Python
  子进程中设置临时 `XDG_CONFIG_HOME`、`XDG_DATA_HOME`，并通过 `gsettings`
  将 `org.gnome.shell enabled-extensions` 整体写为 `['bongocat-core@local']`。
- 19:09 用户注销重登。排查时真实桌面的启用列表仅剩 BongoCat；AppIndicator
  为 `Enabled: No`、`State: INITIALIZED`，会话总线上没有
  `org.kde.StatusNotifierWatcher`。
- 19:23 Flameshot 启动日志报告 `QDBusTrayIcon` 的
  `org.freedesktop.DBus.Error.ServiceUnknown`，提示 `The name is not activatable`。
- 19:24 同一测试方式再次运行时，通过 `/proc/<pid>/environ` 确认：嵌套
  GNOME Shell 使用临时配置目录，而该独立 D-Bus 会话启动的 `dconf-service`
  没有 `XDG_CONFIG_HOME`，`HOME` 仍指向真实用户目录；真实
  `~/.config/dconf/user` 的修改时间也落在此次测试期间。

### 根因

后续复查确认 19:27 的优化性能复测再次触发了同一问题：
`/tmp/bongocat-layered-optimized.log` 记录测试总线激活了 dconf，真实
`~/.config/dconf/user` 的最后修改时间为 19:27:09，启用列表再次仅剩 BongoCat。
20:04 排查时已没有残留嵌套 Shell 或测试总线；随后逐项恢复三个原有扩展，
保留 BongoCat，并验证托盘服务恢复。源码与运行中的 BongoCat 本身没有写入
扩展启用列表的代码。默认测试限制已加入仓库根目录 `AGENTS.md`。

在 D-Bus 启动后，仅修改其子进程的环境变量，不会更新 D-Bus 激活服务所继承
的环境。嵌套 Shell 与 `gsettings` 虽然指向临时配置目录，负责写入的
`dconf-service` 仍使用真实用户配置目录。因此独立会话总线并没有隔离磁盘上
的配置，测试中的扩展列表覆盖操作影响了真实桌面，注销重登后表现为托盘消失。

### 恢复与验证

已逐个重新启用 AppIndicator、Kimpanel 和 BingWallpaper，保留 BongoCat。
三个恢复的扩展均为 `ACTIVE`，启用列表包含上述四项。
`org.kde.StatusNotifierWatcher` 恢复服务，注册项中已确认 Flameshot 的
`Id` 和 `Title` 均为 `flameshot`；BongoCat 继续运行。
微信进程未被重启，其图标是否恢复没有做视觉确认。

### 防止复发

- 必须在启动 `dbus-run-session` **之前**设置临时 `XDG_CONFIG_HOME` 和
  `XDG_DATA_HOME`，使总线及其激活服务继承同一套隔离环境。
- 写入测试配置前，核实测试总线上的 `dconf-service` 实际环境和写入目标；
  不能只检查嵌套 GNOME Shell 的环境。
- 测试前后保存并比对真实桌面的 `enabled-extensions` 和 `disabled-extensions`，
  若发生变化则视为隔离失败。测试结果不能仅凭使用了独立 D-Bus 就宣称
  「未修改当前桌面配置」。
- 正常安装启用扩展时使用 `gnome-extensions enable`，保留已有扩展列表；
  不要将真实桌面的启用列表整体替换为测试扩展。

隔离要求已加入 [集成测试说明](README.md)。本次完成的是配置恢复和文档修正，
尚未新增或验证自动化隔离启动器。

## 固定底图与鼠标跟随修复

四张完整画面的生成素材并未像素对齐，按键切帧会使键盘看起来改变大小。
已移除该图集，改为单张固定底图和代码绘制的猫爪/鼠标；键盘侧的两个姿态
由同一底图生成并缓存。鼠标侧根据相对 X/Y 位移动态绘制，使用三个共享内存
缓冲区，并且只写入已收到 `wl_buffer.release` 的缓冲区。

- 8 个普通测试通过，包括跨按键状态/鼠标极限位置的固定像素一致性、
  移动时画面确实变化、相对位移累积/限位/忽略滚轮。
- 已在两个真实设备上重新执行事件掩码回读测试：SYN/KEY/REL 通过，
  相对事件仅保留 X/Y，独立读取者不受影响。
- 已渲染并检查四种应用画面，以及独立 GNOME 会话的实际截图。
- 原生 Wayland 持续输入测试加入每 10 ms 一次的 X/Y 位移；优化后的 release
  运行 60 秒，提交 3532 帧，单核 CPU 0.8333%，RSS 6600 KiB、峰值 12220 KiB。
  这是状态机注入的绘制负载，不是物理鼠标采样性能。
- 分层渲染空闲测试运行 60 秒，仅提交一次，CPU 0.0167%；同一测试进程
  先运行活动用例后，分配器保留内存，RSS 为 12684 KiB。

鼠标跟随按相对设备计数累计并限制在鼠标垫内；没有通过 GNOME 获取桌面
绝对指针坐标。程序已更新到本机安装路径并重启，真实键盘和鼠标初始化成功。
