# 实现颜色
文件或程序输出的不是“带颜色的字符”，而是普通字符与终端控制序列组成的字节流。`cat`、C
程序和 Linux PTY 负责传递这些字节，Alacritty
这样的终端模拟器负责解析控制序列、维护字符属性，并用对应颜色绘制字符。

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

后面的普通字符会继承当前的黄色前景色，直到遇到新的 SGR 序列。常用的 `ESC[0m`
会把颜色、粗体、下划线等属性全部恢复为默认值。

基本流程为:
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

TTY/PTY
是终端设备和传输通道。它可以处理回显、规范模式、信号和部分输出转换，但不会把
`ESC[33m` 绘制成黄色。颜色解析与绘制发生在用户态终端模拟器中。

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

此时终端实际收到的是普通字符 `^` 和 `[`，而不是真实的
`ESC`，所以不会把它当作颜色序列。需要查看精确字节时可以使用：

```bash
xxd -g 1 color.txt
```

不要直接 `cat`
来源不可信的二进制或日志。终端控制序列除了修改颜色，还可以修改窗口标题、创建超链接，甚至在某些终端和配置下访问剪贴板。检查未知内容时，优先使用
`cat -v`、`less` 的安全显示方式或 `xxd`。

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
