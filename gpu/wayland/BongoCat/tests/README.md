# 手动集成测试

本工作区默认只运行单元测试、图片预览和纯渲染基准；完整 GNOME 集成测试应放到
独立测试用户或独立测试机。之前的嵌套测试已两次误改真实桌面配置，不要复用
`/tmp/bongocat-qa-*` 等旧测试目录或启动方式。

两个默认忽略的 Rust 测试会打开原生 Wayland 窗口，分别运行 60 秒。
它们使用生产代码中的渲染器和事件循环，通过状态机注入输入，不需要读取真实键鼠设备。
可以在上述独立测试环境中运行。

## GNOME 测试配置隔离

必须在启动 `dbus-run-session` **之前**设置临时 `XDG_CONFIG_HOME` 和
`XDG_DATA_HOME`，让总线及其激活的服务继承同一套隔离环境。
仅在子进程中设置这些变量是不够的：D-Bus 激活的 dconf 服务可能仍使用
真实用户配置目录，并覆盖桌面设置，包括 `enabled-extensions`。

写入任何测试设置前，应核实 dconf 服务的实际环境和写入目标。测试前后保存并
比对真实桌面的扩展启用/禁用列表。独立 D-Bus 会话本身不能隔离磁盘上的用户配置。
不要将真实桌面的扩展列表整体替换为测试扩展列表。

## Wayland 显示与性能

```sh
cargo test --release live_wayland_idle -- --ignored --nocapture --test-threads=1
cargo test --release live_wayland_active -- --ignored --nocapture --test-threads=1
```

空闲测试要求只提交一次画面。活动测试向状态机注入重叠的键盘/鼠标短按和持续
相对位移，检查提交频率不超过 60 Hz。两个测试都会输出进程 CPU 时间、峰值
常驻内存（RSS）和画面提交次数。测量包含 Rust 测试程序的开销，不包含合成器。

为真实设备配置读取权限后，使用安装后的程序连接实际键盘和鼠标。分别在空闲、
正常打字/点击和快速移动鼠标时运行：

```sh
pidstat -u -r -w -p PID 1 60
```

将 `PID` 替换为程序进程号。移动鼠标时猫爪应跟随，画面更新不超过 60 Hz；
停止移动且没有其他状态变化后，不应继续提交画面。可以检查等待系统调用：

```sh
strace -e epoll_wait,epoll_pwait,poll,ppoll -p PID
```

## 桌面交互与设备恢复

验证鼠标穿透、启动后焦点保持、工作区切换、主屏切换、显示缩放、扩展禁用后
重新启用，以及按住按键时拔出并重新插入设备。
窗口画面和位置应恢复，一个设备的释放事件不能清除另一个设备仍按住的按键。
全屏应用可能遮住猫咪；锁屏必须遮住它。

验证 `SYN_DROPPED` 时，应使用可丢弃的 uinput 测试设备：暂停程序、连续发送
大量事件，再恢复程序。evdev 同步流应恢复当前按键状态，不能让猫爪卡在按下状态。
不要向正常桌面的键盘注入这种事件洪流。

## evdev 位图 ABI 回归

将环境变量指向一个具有读取权限的设备：

```sh
BONGOCAT_TEST_DEVICE=/dev/input/by-id/usb-ROYUAN_Gaming_keyboard-event-kbd \
  cargo test live_evdev_mask_round_trip -- --ignored
```

测试设置并读回当前文件描述符的事件掩码，确认 X/Y 相对位移可通过、滚轮等
无关事件被屏蔽，并确认独立打开的另一个读取者不受影响。
测试不读取按键事件，也不独占设备。

## 原始素材与画面预览

```sh
cargo test idle_is_pixel_identical_to_original_cover
cargo test static_pixels_never_change_between_poses_or_mouse_positions
cargo test render_previews -- --ignored
```

前两项验证默认待机画面与原封面逐像素一致，以及活动区域之外的画面保持不变。
预览输出到 `target/previews/`，需要检查按键、鼠标点击和鼠标移动极限位置下的
猫爪形状、遮罩接缝和透明边缘。原始 PNG 文件直接来自 Git 历史，不应重新生成或重绘。

只测量绘制成本、无需打开桌面窗口时，可以运行：

```sh
cargo test --release render_benchmark -- --ignored --nocapture
```

本次原素材实现连续绘制 3600 帧耗时约 168 毫秒，平均 0.047 毫秒/帧。
该数值仅包含 CPU 画面生成，不包含 Wayland 提交、设备读取和合成器开销。
