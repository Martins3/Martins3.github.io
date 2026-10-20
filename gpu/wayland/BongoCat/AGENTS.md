# 桌面配置保护

- 普通验证优先使用 `cargo test`、`render_previews`、`render_benchmark`，不要主动启动嵌套 GNOME 或修改用户桌面设置。
- 本项目的嵌套 GNOME 测试曾两次误写真实用户 dconf，禁用了 AppIndicator、Kimpanel 和 BingWallpaper。不得复用 `/tmp/bongocat-qa-*` 等旧测试会话目录或其启动方式。
- 需要完整 GNOME 集成验证时，使用独立测试用户或独立测试机，不能共享日常桌面的用户配置。独立 D-Bus 总线不等于配置隔离；仅在总线启动后的子进程中设置 XDG 环境变量无效。
- 禁止为测试整体覆盖、清空或重置 `enabled-extensions` / `disabled-extensions`。修复用户配置时按已知原状态逐项恢复，并核实扩展的实际 `ACTIVE` 状态。
- 改动 BongoCat 源码、图片或测试文档不需要重新配置 GNOME 扩展。保留工作区内用户已有的未提交修改。
