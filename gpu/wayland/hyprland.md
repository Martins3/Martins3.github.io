# hyprland
<!-- daf638ce-6e6b-4791-8f89-02db5e1b7d46 -->

这个项目是 Hyprland（v0.54.0），一个动态平铺的 Wayland 合成器（compositor）。回答你的两个问题：

1. 它"实现"了 Wayland 吗？

准确说法是：Hyprland 实现了 Wayland 协议的服务端（compositor），而不是实现了 Wayland 协议本身。

- Wayland 本身只是一个协议规范（client 与 server 之间如何通信），它的参考实现是 libwayland（wayland-server / wayland-client 库）。
- 真正在屏幕上绘制窗口、管理输入输出的那个程序叫 Wayland compositor（显示服务器 / 合成器）。
- Hyprland 就是一个 compositor：窗口管理器 + 合成器二合一，负责窗口布局、渲染、输入事件分发等。源码里 src/Compositor.cpp、src/desktop/、src/render/、
  src/managers/ 就是干这些事的。

代码目录印证：它直接依赖 subprojects/hyprland-protocols（自己的 Wayland 协议扩展），以及 udis86/tracy 这类工具库，没有依赖 wlroots——README 里原话：

│ 100% independent, no wlroots, no libweston, no kwin, no mutter.

2. 和 GNOME 是什么关系？

没有代码上的关系，属于同类软件、互相替代/竞争关系。

┌─────────────────┬─────────────────────────────┬──────────────────────┐
│                 │ Hyprland                    │ GNOME                │
├─────────────────┼─────────────────────────────┼──────────────────────┤
│ 定位            │ 独立 tiling compositor      │ 完整桌面环境（DE）   │
├─────────────────┼─────────────────────────────┼──────────────────────┤
│ 核心 compositor │ 自己（Hyprland）            │ Mutter               │
├─────────────────┼─────────────────────────────┼──────────────────────┤
│ 依赖            │ 无 wlroots/libweston/mutter │ GNOME Shell + Mutter │
└─────────────────┴─────────────────────────────┴──────────────────────┘

- GNOME 是一个桌面环境，它的 compositor 叫 Mutter。Hyprland 和 Mutter 是两个互不依赖、各自独立实现的 Wayland compositor。
- 二者可以在同一台机器上共存，登录时通过显示管理器（GDM/SDDM 等）选择进入哪一个会话。
- Hyprland 只是 compositor（不带全套 DE 应用），所以很多人会把它和 GNOME/KDE 的某些应用混用。

一句话总结：Hyprland 是 Wayland 协议的一个独立 compositor 实现，与 GNOME 的 Mutter 是平级的竞争实现，代码上毫无关联。

