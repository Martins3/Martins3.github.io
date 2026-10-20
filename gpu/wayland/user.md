## 介于 wayland 协议和 appliation 之间的
https://github.com/rust-windowing/winit

### sdl 和 gtk

可以粗略理解为：

```txt
  普通桌面应用 → GTK ─┐
                      ├→ Wayland / X11 → 桌面显示系统
  游戏、模拟器 → SDL ─┘
```

也就是我们可以发现在 qemu 中的 ui 目录下发现有两个后端支持:


这里的 SDL 和 GTK 是 QEMU 在宿主机上显示虚拟机画面的两种可选 UI 后端。QEMU 要把 guest 的屏幕显示成窗口，还要把窗口里的键盘、鼠标操作传回
guest，因此需要这些 GUI 库。

 后端    库的定位                                      QEMU 中的用途
━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 SDL     跨平台多媒体库，提供窗口、绘图、输入事件等    提供较简单的虚拟机显示窗口
──────  ────────────────────────────────────────────  ────────────────────────────────────
 GTK     桌面 GUI 工具库，提供菜单、标签页等控件       提供带菜单、控制台切换等功能的界面

它们处在同一层，数据流大致是：

guest 显卡 → QEMU 显示设备实现 → 公共显示接口 → SDL / GTK → 宿主机窗口
guest 输入设备 ← QEMU 输入接口 ← SDL / GTK ← 宿主机键鼠

启动时通过 -display sdl 或 -display gtk 选择，前提是构建时包含相应后端。guest 内部用什么桌面，与这里选择 SDL 还是 GTK 无关。

从代码看，两者都注册成 QemuDisplay，并通过 DisplayChangeListenerOps 接收画面更新：

• sdl2.c (sdl2.c:815)：SDL 后端。
• gtk.c (gtk.c:566)：GTK 后端。
• meson.build (meson.build:152)：分别根据依赖是否存在构建这两个模块。

所以同时保留它们，是为了提供不同的界面体验和依赖选择；运行 QEMU 并不要求同时使用它们。

```txt
    <graphics supported='yes'>
      <enum name='type'>
        <value>sdl</value>
        <value>vnc</value>
        <value>spice</value>
        <value>egl-headless</value>
        <value>dbus</value>
      </enum>
    </graphics>
```

```bash
	# 这两个可以同时配置的
	# -vga none \
	# -device virtio-vga-gl,xres=2048,yres=1152 \
	# -display sdl,gl=on \
```


## 图形可以简单分为两种

1. 渲染画面
不管游戏用 Vulkan / OpenGL / DXVK，流程都是：
游戏用 Vulkan 渲染一帧画面
渲染好的画面存在显存里

2. 窗口系统
“我有一帧画面要显示”
Wayland 合成器（GNOME/KDE/Sway/Hyprland）
把画面贴到窗口里，显示到屏幕

所以：
Vulkan = 负责画得快、画得好
Wayland = 负责管理窗口、把画面送上屏幕
两者必须配合，缺一不可


在 GNOME 里，真正和 Wayland 直接打交道的是它的显示服务器/合成器 Mutter

所以更准确地说：
GNOME Shell + Mutter 可以把整个 GNOME 会话跑在 Wayland 协议之上。


## 案例分析

### wps
忽然好奇 wps 都是如何工作的:

codex 给出来的回答如下:

> 你这台机器上的 WPS 现在不是原生 Wayland 客户端，它跑在 Wayland GNOME 会话里，
但实际走的是 X11 -> Xwayland -> Mutter/GNOME Shell ->
DRM/EGL/NVIDIA 这条链路。Vulkan/EGL 主要在合成器侧，不在 WPS 自己进程里。

### tauri

- 和 Tauri 同层次的是 Electron、Qt、GTK、Flutter 这类应用框架或应用技术栈
	- KDE Plasma
	- XFCE
	- LXQt
	- Cinnamon
	- MATE
	- Deepin Desktop Environment

## tauri

tauri 依赖 TAO

TAO 与 Wayland 的核心关系是：TAO 在 Linux 上通过 GTK 间接支持 Wayland，而非直接对接协议，这是其设计与依赖决定的。

### chrome
Chrome（Chromium）与 Wayland 的关系，可以概括为：通过自研的 Ozone 抽象层，实现了对 Wayland 显示服务器的原生支持，
是目前 Linux 平台上对 Wayland 支持最完善的浏览器之一

egui 本身不直接对接 Wayland，而是通过其官方窗口后端 winit 实现对 Wayland 的完整原生支持。在 Linux 上，egui 应用默认就是 Wayland 原生程序。
