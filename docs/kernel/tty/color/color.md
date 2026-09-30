# 终端颜色是如何显示出来的

## 结论

文件或程序输出的不是“带颜色的字符”，而是普通字符与终端控制序列组成的字节流。`cat`、C 程序和 Linux PTY 负责传递这些字节，Alacritty 这样的终端模拟器负责解析控制序列、维护字符属性，并用对应颜色绘制字符。

以 `color.txt` 中的黄色为例：

```text
1b 5b 30 3b 33 6d
ESC  [  0  ;  3  3  m
```

它通常写作：

```text
ESC[0;33m
```

其中：

- `ESC` 是值为 `0x1b` 的控制字符。
- `ESC [` 引入 CSI（Control Sequence Introducer）序列。
- `0;33` 是参数：`0` 重置属性，`33` 设置普通黄色前景色。
- `m` 是 SGR（Select Graphic Rendition）操作的结束字符。

后面的普通字符会继承当前的黄色前景色，直到遇到新的 SGR 序列。常用的 `ESC[0m` 会把颜色、粗体、下划线等属性全部恢复为默认值。

## 数据经过哪些层次

```text
color.c、cat 或其他应用
        |
        | write(2) 写入 stdout
        v
PTY slave 和 Linux TTY 层
        |
        | 传输字节，通常不解释 SGR
        v
PTY master
        |
        | Alacritty 读取字节
        v
vte 状态机解析 CSI/SGR
        |
        | 更新终端网格中字符的颜色属性
        v
Alacritty 将逻辑颜色转换为 RGB
        |
        v
OpenGL/GLES 绘制字符
```

TTY/PTY 是终端设备和传输通道。它可以处理回显、规范模式、信号和部分输出转换，但不会把 `ESC[33m` 绘制成黄色。颜色解析与绘制发生在用户态终端模拟器中。

## `cat` 和 `cat -v` 的区别

普通 `cat` 原样复制文件内容，因此真实的 `ESC` 字节会到达终端模拟器：

```bash
cat color.txt
```

Alacritty 收到 `ESC[33m` 后会显示黄色。

`cat -v` 会把不可打印控制字符转换成可见文本：

```bash
cat -v color.txt
```

显示结果中的：

```text
^[[0;33m
```

应当拆成：

```text
^[       ESC（0x1b）的可见表示
[0;33m   原序列中剩余的普通字符
```

此时终端实际收到的是普通字符 `^` 和 `[`，而不是真实的 `ESC`，所以不会把它当作颜色序列。需要查看精确字节时可以使用：

```bash
xxd -g 1 color.txt
```

## Alacritty 0.17.0 的实现链路

本机 `/usr/bin/alacritty` 的版本是 0.17.0，本地源码位于 `/home/martins3/data/alacritty`。以下逻辑在源码 tag `v0.17.0` 中确认过；该版本使用 `vte 0.15.0` 的 `ansi` 功能。

### 1. 从 PTY 读取数据

`alacritty_terminal/src/event_loop.rs` 中的 `EventLoop::pty_read()` 从 PTY master 读取数据，然后调用：

```rust
state.parser.advance(&mut **terminal, &buf[..unprocessed]);
```

在这一步，颜色序列和普通文本仍然位于同一个字节缓冲区中。

### 2. vte 解析 CSI 和 SGR

`vte-0.15.0/src/ansi.rs` 中的 `Processor::advance()` 使用字节状态机识别 `ESC [`、参数和结束字符。

当 `Performer::csi_dispatch()` 收到结束字符 `m` 时，它调用 `attrs_from_sgr_parameters()`。参数 `33` 映射为：

```rust
Attr::Foreground(Color::Named(NamedColor::Yellow))
```

参数 `0` 映射为 `Attr::Reset`。因此 `ESC[0;33m` 会先恢复默认属性，再把当前前景色改成逻辑颜色 `Yellow`。

### 3. 保存字符属性

`alacritty_terminal/src/term/mod.rs` 的 `impl Handler for Term<T>` 中，`terminal_attribute()` 将颜色保存到光标模板：

```rust
Attr::Foreground(color) => cursor.template.fg = color,
```

后续普通字符由 `input()` 和 `write_at_cursor()` 写入终端网格。`write_at_cursor()` 会把 `cursor.template.fg` 复制到字符单元，所以颜色状态会一直作用于后续字符，直到被其他 SGR 序列修改。

### 4. 将逻辑颜色转换成 RGB

`alacritty/src/display/color.rs` 中的 `List::fill_named()` 创建颜色表，把 `NamedColor::Yellow` 映射为配置中的 `colors.normal.yellow`。

本机 `~/.alacritty.toml` 链接到 `~/.dotfiles/config/alacritty.toml`，该配置没有定义 `[colors]`，所以使用 `alacritty/src/config/color.rs` 中 `NormalColors::default()` 的默认黄色：

```text
#f4bf75
```

`alacritty/src/display/content.rs` 中的 `RenderableCell::compute_fg_rgb()` 根据逻辑颜色、粗体和暗色等标志得到最终 RGB。

### 5. 绘制

最终 RGB 被交给文本渲染器。`alacritty/src/renderer/text/glsl3.rs` 和 `gles2.rs` 中的 `Batch::add_item()` 将字符单元的 `fg.r`、`fg.g`、`fg.b` 写入渲染实例，随后由 OpenGL 或 GLES shader 绘制到窗口中。

所以整条链路可以概括为：

```text
ESC[33m
  -> vte: NamedColor::Yellow
  -> Term: cursor.template.fg
  -> Cell: fg
  -> Alacritty color table: #f4bf75
  -> OpenGL/GLES renderer
```

## 注意事项

不要直接 `cat` 来源不可信的二进制或日志。终端控制序列除了修改颜色，还可以修改窗口标题、创建超链接，甚至在某些终端和配置下访问剪贴板。检查未知内容时，优先使用 `cat -v`、`less` 的安全显示方式或 `xxd`。

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
