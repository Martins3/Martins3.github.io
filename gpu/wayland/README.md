# 环境搭建

```sh
sudo yum -y install wayland-devel
sudo yum -y install wayland-protocols-devel
sudo yum -y install mesa-libEGL-devel
```

- 当前目录也支持直接用 `nix-shell` / `direnv` 进入开发环境:

```sh
cd /home/martins3/data/vn/gpu/wayland
direnv allow
make
make -C win
```

- 如果命令是在 SSH shell 里执行，还需要显式带上桌面会话的 Wayland 变量:

```sh
export XDG_RUNTIME_DIR=/run/user/1000
export WAYLAND_DISPLAY=wayland-0

./hello.out
./registory.out
./seat.out
timeout 5 ./win/window.out
timeout 5 ./win/window_egl.out
```

- 也可以不依赖当前 shell 环境，直接这样运行:

```sh
nix-shell --run 'make && XDG_RUNTIME_DIR=/run/user/1000 WAYLAND_DISPLAY=wayland-0 ./registory.out'
nix-shell ./default.nix --run 'make -C win && XDG_RUNTIME_DIR=/run/user/1000 WAYLAND_DISPLAY=wayland-0 timeout 5 ./win/window.out'
```

- [Wayland 入门教程](https://feater.top/wayland/wayland-learning-indexes) : 其实还可以
    - 配套代码: https://github.com/JackeyLea/Wayland_Freshman
  - 不容易: https://lists.freedesktop.org/archives/wayland-devel/2022-February/042103.html


## 看看 wayland 的源码是什么?

真正大的部分不在这个仓库里，而在：

- compositor/桌面壳：Weston、Mutter、KWin、wlroots/sway
- 渲染栈：Mesa
- 输入栈：libinput
- 内核 DRM/KMS、evdev

所以结论是：wayland 仓库“只有 1w 多行核心代码”是合理的；如果你以为它应该包含整个显示服务器、窗口管理器、渲染器，那就把范围想大了。

## wayland 到底要解决什么问题

• Wayland 主要解决的，不是“怎么画三角形”，而是“图形桌面里，客户端、合成器、输
入、显示输出之间怎么协作”这个老问题。它本质上是在替换 X11 那套
  窗口系统协议。

  核心改进可以概括成几件事。

  1. 简化架构
     X11 时代，X server 既像显示服务器，又背着很多历史包袱，协议很大，很多功能
 放在网络透明和服务端绘制那一套上。现代桌面实际早就变成“客户端
     自己渲染，窗口管理器/合成器再拼起来显示”。Wayland 直接围绕这个现实设计，
把模型收敛成：
     client 产出 buffer -> compositor 决定如何合成 -> 提交到 KMS/DRM
     少了一层历史兼容负担。
  2. 降低拷贝和状态同步成本
     X11 下常见问题是：

  - 客户端和 X server 对窗口状态都有一份理解
  - 窗口管理器又有第三份理解
  - 合成器如果单独存在，还会再多一层
    这导致状态同步复杂、时序容易乱。Wayland 里 compositor 是中心，客户端只负
  责提交 buffer 和响应 configure，职责更清楚，减少很多额外 round-
    trip 和语义冲突。

  3. 更好的时序控制
     图形系统最怕“谁在什么时候画、什么时候显示、什么时候丢帧”不清楚。Wayland
  把提交模型做得更明确：

  - 客户端提交 buffer
  - compositor 决定何时采纳
  - frame callback / configure 形成比较清晰的节奏
    这让动画、resize、vsync、避免 tearing 更容易做好。不是自动完美，但协议层
  更顺手。

  4. 安全模型更合理
     X11 最大的老问题之一是“默认同桌面客户端彼此几乎不隔离”。典型后果：

  - 任意客户端容易监听全局输入
  - 任意客户端能截屏/读别的窗口内容
  - 注入输入事件也比较容易
    Wayland 默认把这些能力收紧，很多高权限操作必须通过 compositor 或 portal
  明确授权。

  5. 更适合现代合成桌面
     今天桌面系统基本都依赖合成器，Wayland 直接把 compositor 作为一等公民，天
  然适合：

  - 多显示器
  - HiDPI / scaling
  - 旋转/变换
  - 混合刷新率
  - 直接扫描输出优化
    这些在 X11 里不是做不到，而是通常更别扭。

  6. 明确“谁负责渲染”
     Wayland 不负责替你定义 2D/3D API。它不试图像老 X11 那样承担大量绘图语
  义，而是让客户端用 Cairo/OpenGL/Vulkan/软件栈自己产出 buffer，再
     交给 compositor。这样协议本身更小，更不容易被渲染细节绑死。

  但它也有一个重要边界：Wayland 解决的是窗口系统和显示协作问题，不是 GPU 编程
  本身。
  它不解决：

  - Vulkan/OpenGL 怎么渲染
  - 驱动怎么管理显存
  - compositor 内部怎么做合成
  - 远程图形协议本身

  如果用一句话总结：

  Wayland 解决的是“现代 Linux 图形桌面里，谁负责生产图像、谁负责调度显示、谁
  有权访问输入/输出，以及这些动作如何更低延迟、更少歧义地协作”。

  如果你愿意，我可以下一条直接结合你这个 wl_shm demo，画一张“client -> buffer
  -> compositor -> KMS/DRM” 的数据流图，把 Wayland 到 GPU 的关系也串起来。

## hyprland 和 wayland 的关系

Wayland 是协议/生态，Hyprland 是这个生态里的一个具体 compositor。

更具体一点：

- Wayland 定义的是客户端和显示服务器怎么通信，比如窗口、输入、buffer 提交这些规则。
- Hyprland 是实现这些规则的软件之一，而且它本身就是一个 Wayland compositor，也同时承担了窗口管理器的角色。
- 所以你可以把它理解成：Wayland = 规范，Hyprland = 按这个规范实现出来的桌面/合成器。

历史上，Hyprland 曾经基于 wlroots；但官方在 2024 年 7 月 21 日宣布它已经“不再是 wlroots-based Wayland compositor”，而是“fully
independent implementation of the protocol”。也就是说：

- Hyprland 和 Wayland 的关系一直是“实现者”关系。
- Hyprland 和 wlroots 的关系则是：以前依赖，后来独立了。

如果再类比一下：

- Wayland 像接口规范
- wlroots 像一个给 compositor 用的实现库
- Hyprland 像一个最终产品

所以用户运行 Hyprland 时，实际是在运行一个 Wayland 合成器；Wayland 应用则是和 Hyprland 这个 compositor 说话。

来源：

- Hyprland 官方：https://hypr.land/news/independentHyprland (https://hypr.land/news/independentHyprland)
- Vaxry 博文：https://blog.vaxry.net/articles/2024-wlrootsRewrite (https://blog.vaxry.net/articles/2024-wlrootsRewrite)
- wlroots 项目说明：https://github.com/swaywm/wlroots (https://github.com/swaywm/wlroots)

如果你愿意，我下一条可以继续讲清楚：Hyprland / wlroots / KWin / GNOME Mutter 这几个在 Wayland 体系里分别处在什么层

### 的确的确可以找到这样的对应的关系

  先说结论
  在源码层面，这个关系体现得非常直接：

  - wayland 仓库提供的是“协议定义 + 通用运行时库”
  - hyprland 仓库提供的是“一个具体的 compositor 实现”，它用前者的库，把 wl_compositor、wl_shm、xdg_wm_base 这些协议对象真正做出来

  也就是说：

  - wayland 决定“消息长什么样、对象怎么建、registry/global/resource 怎么运转”
  - hyprland 决定“收到这些请求后，窗口怎么创建、surface 怎么 commit、buffer 怎么接收、怎么布局和合成”

  在 wayland 源码里，它更像“协议框架”
  /home/martins3/data/wayland/README.md:1 直接写了：Wayland 是“protocol + library implementation”。
  /home/martins3/data/wayland/doc/book/src/Protocol.md:50 说明核心接口来自 /home/martins3/data/wayland/protocol/wayland.xml:55，并且
  这些 XML 会生成 client/compositor 都用的 stub。
  同一个文件里还把线上的消息格式说得很明确：/home/martins3/data/wayland/doc/book/src/Protocol.md:62 说明它就是 Unix domain socket 上
  的消息协议；/home/martins3/data/wayland/doc/book/src/Protocol.md:132 说明 global 的版本由 wl_registry.bind() 决定。

  协议定义本身就在那里：

  - wl_display.get_registry 在 /home/martins3/data/wayland/protocol/wayland.xml:55
  - wl_compositor.create_surface 在 /home/martins3/data/wayland/protocol/wayland.xml:193
  - wl_shm.create_pool(fd, size) 在 /home/martins3/data/wayland/protocol/wayland.xml:468

  这些文件只是在定义“协议长什么样”，并不负责你的窗口该平铺、浮动还是做动画。

  再看服务端库实现：

  - display_get_registry() 在 /home/martins3/data/wayland/src/wayland-server.c:1191 里创建 wl_registry resource，并把当前 global_list
    广播给 client
  - wl_global_create() 在 /home/martins3/data/wayland/src/wayland-server.c:1454 里注册一个 global
  - wl_resource_create() 在 /home/martins3/data/wayland/src/wayland-server.c:2206 里给某个 client 创建一个协议对象 resource

  这些就是 Wayland 的“基础设施层”。它知道怎么管对象 ID、怎么发 global、怎么建 resource，但它不知道 Hyprland 想把窗口放哪。

  在 hyprland 源码里，它体现为“我来实现这些协议”
  /home/martins3/data/hyprland/README.md:11 直接说它是 “independent ... Wayland compositor”。

  先看它怎么把 Wayland server 拉起来：

  - /home/martins3/data/hyprland/src/Compositor.cpp:291 的 initServer()
  - /home/martins3/data/hyprland/src/Compositor.cpp:302 调 wl_display_create()
  - /home/martins3/data/hyprland/src/Compositor.cpp:306 取 event loop

  也就是说，Hyprland 是直接用 wayland-server 这套底层 API 启自己的 Wayland server。

  然后它开始把协议 global 注册出去。
  抽象层在 /home/martins3/data/hyprland/src/protocols/WaylandProtocol.cpp:23，IWaylandProtocol 构造函数里直接调用：

  - wl_global_create(g_pCompositor->m_wlDisplay, iface, ver, this, ...)

  也就是：Hyprland 把每个协议 manager 对象注册成一个 Wayland global。

  具体注册在哪：

  - wl_compositor 在 /home/martins3/data/hyprland/src/managers/ProtocolManager.cpp:145
  - wl_shm 在 /home/martins3/data/hyprland/src/managers/ProtocolManager.cpp:147
  - xdg_wm_base 在 /home/martins3/data/hyprland/src/managers/ProtocolManager.cpp:181
  - linux-dmabuf 在 /home/martins3/data/hyprland/src/managers/ProtocolManager.cpp:235

  而且它还维护一份自己要暴露给客户端的 global 集合，xdgShell->getGlobal() 也在里面：/home/martins3/data/hyprland/src/managers/
  ProtocolManager.cpp:357。

  最关键的是：Hyprland 真正实现了这些请求
  比如 wl_compositor.create_surface。

  协议里只说“create_surface 会创建一个 wl_surface”。
  Hyprland 对应的实现是在 /home/martins3/data/hyprland/src/protocols/core/Compositor.cpp:748：

  - /home/martins3/data/hyprland/src/protocols/core/Compositor.cpp:754 setCreateSurface(...)
  - /home/martins3/data/hyprland/src/protocols/core/Compositor.cpp:755 真正 new 出 CWlSurface
  - /home/martins3/data/hyprland/src/protocols/core/Compositor.cpp:768 发出 newSurface 事件

  这就体现了区别：

  - wayland.xml 只定义“有 create_surface 这个 request”
  - Hyprland 代码决定“收到这个 request 后，我创建哪种 surface 对象，以及后续怎么进 compositor 的管理流”

  再比如 wl_surface.commit。
  Wayland 协议只规定 commit 的语义；Hyprland 真正处理 pending/current state 的逻辑在 /home/martins3/data/hyprland/src/protocols/core/
  Compositor.cpp:554。这里的 commitState() 才是在做 compositor 该做的事：更新当前状态、纹理、damage、subsurface 同步等。

  你前面那个 wl_shm 例子也能一一对应上：

  - 协议定义 wl_shm.create_pool(fd, size) 在 /home/martins3/data/wayland/protocol/wayland.xml:468
  - Hyprland 接收这个请求的实现，在 /home/martins3/data/hyprland/src/protocols/core/Shm.cpp:183
  - 它把 client 传来的 fd 包成 CWLSHMPoolResource
  - 并在 bind wl_shm 时主动告诉 client 支持哪些格式，在 /home/martins3/data/hyprland/src/protocols/core/Shm.cpp:196

  这就和你那个 demo 完全对上了：
  你的 demo 手写 wl_shm.create_pool 消息，Hyprland 这边就会落到这里处理。

  xdg_shell 也一样。
  Hyprland 在 /home/martins3/data/hyprland/src/protocols/XDGShell.cpp:816 里实现 xdg_wm_base 的 bind。
  然后：

  - /home/martins3/data/hyprland/src/protocols/XDGShell.cpp:738 实现 create_positioner
  - /home/martins3/data/hyprland/src/protocols/XDGShell.cpp:755 实现 get_xdg_surface
  - /home/martins3/data/hyprland/src/protocols/XDGShell.cpp:779 给 wl_surface 赋上 xdg_surface role

  这就是“Hyprland 是 Wayland compositor”的最直观证据：
  它不是只会调用 client API，而是在实现 server 端协议对象和行为。

  把两边连起来看
  你可以把它理解成这条链：

  1. wayland/protocol/wayland.xml 定义协议名字、request/event、参数和版本
  2. wayland/src/wayland-server.c 提供 server 运行时：wl_display、wl_global、wl_resource
  3. hyprland/src/Compositor.cpp 创建自己的 wl_display
  4. hyprland/src/managers/ProtocolManager.cpp 注册 wl_compositor、wl_shm、xdg_wm_base 等 global
  5. hyprland/src/protocols/... 里分别实现这些协议对象收到请求后的真实行为

  所以“Wayland 和 Hyprland 的关系”在源码里不是抽象概念，而是：

  - wayland 给出语言和对象模型
  - hyprland 用这门语言搭了一个完整桌面 compositor

  顺手说一句，你那个 win/demo2.c:1 本质上是在“手写 Wayland wire protocol client”；而 Hyprland 这边的这些 Protocol*.cpp 则是在“实现对应的 server side”。

### demo2.out 连接的东西是什么

• 是 gnome-shell，具体是 PID 11698，它在你的 GNOME Wayland 会话里同时承担了 compositor 的角色，也就是 Mutter 的 Wayland server 部分。

  我本地直接查了 socket 监听者：

  ss -xlpn | grep /run/user/1000/wayland-0

  关键信息是：

  /run/user/1000/wayland-0 ... users:(("gnome-shell",pid=11698,...))

  所以 demo2.out 连的 server 端就是这个进程，不是单独一个叫 wayland 的 daemon。你这里还能看到：

  - gnome-shell 是主 Wayland compositor
  - Xwayland 是给 X11 应用兼容用的子进程
  - mutter-x11-frames 也是 GNOME/Xwayland 相关辅助进程
