## 什么东西
https://github.com/ghostty-org/ghostty

这是非常有趣的东西了，作者提到真的有趣的 libghostty 这个工具:
https://news.ycombinator.com/item?id=47206009

## vte 改进
https://bxt.rs/blog/just-how-much-faster-are-the-gnome-46-terminals/

vte 是有趣的工作，这个性能测试也是有趣的工作

## Neovim 使用 libvterm

那么 libvterm 在那个层次?

## Alacritty 的实现流程

### 1. 从 PTY 读取数据

`alacritty_terminal/src/event_loop.rs` 中的 `EventLoop::pty_read()` 从 PTY
master 读取数据，然后调用：

```rust
state.parser.advance(&mut **terminal, &buf[..unprocessed]);
```

在这一步，颜色序列和普通文本仍然位于同一个字节缓冲区中。

### 2. vte 解析 CSI 和 SGR

`vte-0.15.0/src/ansi.rs` 中的 `Processor::advance()` 使用字节状态机识别
`ESC [`、参数和结束字符。

当 `Performer::csi_dispatch()` 收到结束字符 `m` 时，它调用
`attrs_from_sgr_parameters()`。参数 `33` 映射为：

```rust
Attr::Foreground(Color::Named(NamedColor::Yellow))
```

参数 `0` 映射为 `Attr::Reset`。因此 `ESC[0;33m`
会先恢复默认属性，再把当前前景色改成逻辑颜色 `Yellow`。

### 3. 保存字符属性

`alacritty_terminal/src/term/mod.rs` 的 `impl Handler for Term<T>`
中，`terminal_attribute()` 将颜色保存到光标模板：

```rust
Attr::Foreground(color) => cursor.template.fg = color,
```

后续普通字符由 `input()` 和 `write_at_cursor()`
写入终端网格。`write_at_cursor()` 会把 `cursor.template.fg`
复制到字符单元，所以颜色状态会一直作用于后续字符，直到被其他 SGR 序列修改。

### 4. 将逻辑颜色转换成 RGB

`alacritty/src/display/color.rs` 中的 `List::fill_named()` 创建颜色表，把
`NamedColor::Yellow` 映射为配置中的 `colors.normal.yellow`。

本机 `~/.alacritty.toml` 链接到
`~/.dotfiles/config/alacritty.toml`，该配置没有定义 `[colors]`，所以使用
`alacritty/src/config/color.rs` 中 `NormalColors::default()` 的默认黄色：

```text
#f4bf75
```

`alacritty/src/display/content.rs` 中的 `RenderableCell::compute_fg_rgb()`
根据逻辑颜色、粗体和暗色等标志得到最终 RGB。

### 5. 绘制

最终 RGB 被交给文本渲染器。`alacritty/src/renderer/text/glsl3.rs` 和 `gles2.rs`
中的 `Batch::add_item()` 将字符单元的 `fg.r`、`fg.g`、`fg.b`
写入渲染实例，随后由 OpenGL 或 GLES shader 绘制到窗口中。

所以整条链路可以概括为：

```text
ESC[33m
  -> vte: NamedColor::Yellow
  -> Term: cursor.template.fg
  -> Cell: fg
  -> Alacritty color table: #f4bf75
  -> OpenGL/GLES renderer
```

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
