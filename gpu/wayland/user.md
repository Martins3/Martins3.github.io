## tauri

tauri 依赖 TAO

TAO 与 Wayland 的核心关系是：TAO 在 Linux 上通过 GTK 间接支持 Wayland，而非直接对接协议，这是其设计与依赖决定的。


Chrome（Chromium）与 Wayland 的关系，可以概括为：通过自研的 Ozone 抽象层，实现了对 Wayland 显示服务器的原生支持，是目前 Linux 平台上对 Wayland 支持最完善的浏览器之一


egui 本身不直接对接 Wayland，而是通过其官方窗口后端 winit 实现对 Wayland 的完整原生支持。在 Linux 上，egui 应用默认就是 Wayland 原生程序。

https://github.com/rust-windowing/winit

## 似乎每次到了这里都会迷糊?

gnome 和 wayland 的关系是什么?

技术上的真实流程（游戏 + Wayland）
不管游戏用 Vulkan / OpenGL / DXVK，流程都是：
游戏用 Vulkan 渲染一帧画面
渲染好的画面存在显存里
winit / SDL2 / GLFW 告诉 Wayland：
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


- 和 Tauri 同层次的是 Electron、Qt、GTK、Flutter 这类应用框架或应用技术栈
	- KDE Plasma
	- XFCE
	- LXQt
	- Cinnamon
	- MATE
	- Deepin Desktop Environment


忽然好奇 wps 都是如何工作的:

codex 给出来的回答如下:

> 你这台机器上的 WPS 现在不是原生 Wayland 客户端，它跑在 Wayland GNOME 会话里，但实际走的是 X11 -> Xwayland -> Mutter/GNOME Shell ->
DRM/EGL/NVIDIA 这条链路。Vulkan/EGL 主要在合成器侧，不在 WPS 自己进程里。

似乎是那么回事，但是这到底是如何工作的呢?
