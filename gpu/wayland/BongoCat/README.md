# BongoCat · Rust / GNOME Wayland

只保留键鼠驱动的猫咪。针对 GNOME 50 Wayland：主屏右下角、透明、鼠标穿透、置顶、跨工作区；全局键盘控制键盘侧猫爪，鼠标左/中/右键控制鼠标侧猫爪，移动鼠标时猫爪带着鼠标在垫内移动。

一个 Rust 进程，加一个约百行的 GNOME 扩展。没有 Live2D、设置界面、声音、模型市场、托盘、手柄、X11 或其他桌面后端。直接复用原项目的封面、背景和拍爪 PNG，素材文件保持原样；默认待机画面与原封面一致。运行时只叠加原有猫爪、局部变形鼠标爪，键盘不会随切帧变化。不复现完整 Live2D 形变或逐键指示。

## 构建与安装

需要 Linux、Rust 1.85+、GNOME Shell 50、Wayland 会话。Cargo 负责下载 Rust 依赖，不需要 SDL、GTK 或 Cubism SDK。

```sh
cargo build --release --locked
./install-local.sh
```

安装到 `~/.local/bin/bongocat` 和用户 GNOME 扩展目录。已有同名程序首次备份为 `bongocat.before-rust`。
首次安装扩展后，GNOME 可能需要**注销并重新登录**才能发现它；脚本不会注销或重启桌面。

```sh
gnome-extensions enable bongocat-core@local
gnome-extensions info bongocat-core@local
```

扩展状态应为 `ACTIVE`。未启用时 Rust 窗口仍可显示，但不会自动定位、置顶或隐藏任务列表。
普通窗口置顶不保证覆盖全屏应用；锁屏和 GNOME 系统界面可遮住猫咪。

## 指定输入设备

evdev 需要读取权限，程序始终以普通用户运行，不独占设备，也不记录按键内容。

```sh
./target/release/bongocat --list-devices
ls -l /dev/input/by-id/
```

本仓库的 `90-bongocat-input.rules` **仅匹配此机器的 ROYUAN Gaming keyboard 和 Compx VGN Mouse 2.4G Receiver**。它不会授权所有键盘。更换设备时，先用 `udevadm info --query=property --name=/dev/input/eventN` 确认身份并修改规则。

首次配置专用组与规则（`groupadd` 仅在组不存在时执行）：

```sh
sudo groupadd --system bongocat-input
sudo usermod -aG bongocat-input "$USER"
sudo install -m644 90-bongocat-input.rules /etc/udev/rules.d/90-bongocat-input.rules
sudo udevadm control --reload-rules
sudo udevadm trigger --action=change --subsystem-match=input
```

随后注销并重新登录，使组成员身份生效。配置只授予专用组读取权限；无需把用户加入能访问全部设备的 `input` 组。

本机启动命令：

```sh
~/.local/bin/bongocat \
  --device /dev/input/by-id/usb-ROYUAN_Gaming_keyboard-event-kbd \
  --device /dev/input/by-id/usb-Compx_VGN_Mouse_2.4G_Receiver-if02-event-mouse
```

Ctrl+C 或 SIGTERM 退出。可选 `--width 306`，范围 64–612 逻辑像素，按原图比例确定高度。至少需要一个 `--device`；推荐稳定的 `by-id` 路径，以便设备重插后自动恢复。

## 性能设计

- 单线程 calloop 事件循环，空闲阻塞等待，无周期性设备扫描。
- `EVIOCSMASK` 在内核中只保留按键和鼠标 X/Y 位移，屏蔽滚轮、扫描码等无关事件。鼠标移动按需唤醒，停下后不持续刷新。
- 每个设备使用固定大小位图维护按键；自动重复和重复边沿不计数，多设备互不释放对方的按键。
- 原始 PNG 内嵌到二进制，只在启动或显示缩放变化时解码、预乘透明度和缩放。运行时复用三个共享内存缓冲区，只写入合成器已经释放的缓冲区；按键姿态预先缓存，移动时仅对原鼠标爪区域进行局部插值。
- 鼠标使用 evdev 相对位移：每 8 个设备计数对应底图 1 像素，活动范围限制在鼠标垫内；这是方向/位移跟随，不是桌面绝对坐标映射。
- 同时使用 Wayland frame callback 和 60 Hz 上限。按住时保持姿态；短点击至少显示 80 ms，来不及显示的点击不会被释放事件直接抹掉。高频点击合并为当前姿态，不排队播放过期动作。
- 设备拔出会清除该设备状态；通过 inotify 恢复连接；使用 evdev 同步流处理 `SYN_DROPPED`。

## 验证

```sh
cargo fmt --check
cargo clippy --all-targets --locked -- -D warnings
cargo test --locked
shellcheck -x install-local.sh
shfmt -d -ci -s -bn install-local.sh
```

显示/性能集成测试需要 Wayland，会打开一个 60 秒的测试窗口，不读取真实键盘；见 [测试说明](tests/README.md)。测试结果与尚未验证的硬件场景记录在 [验证记录](tests/RESULTS.md)。

## 项目参考

https://github.com/vladelaina/BongoCat 核心参考

https://github.com/ayangweb/BongoCat 不支持 Linux
```txt
  输入 → Runtime / Live2D 求值 → RenderSnapshot
                                     ├─ Windows：D3D11 → Win32 窗口
                                     └─ macOS：Metal → NSPanel
```

## 许可证与素材

代码为 [AGPL-3.0-only](LICENSE)。原始素材保留原项目的 [MIT 声明](assets/LICENSE-MIT)，来源与复用方式见 [素材说明](assets/README.md)。
