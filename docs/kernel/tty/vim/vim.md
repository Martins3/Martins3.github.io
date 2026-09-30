• 对，你理解的这个机制就是 “终端控制序列”（terminal escape sequences）。

  Vim 把普通文字和控制序列混合，通过 write() 写入终端；Kitty、Ghostty 从 PTY master 读到这些字节，解析后更新屏幕。 例如“移动光
  标”“设置红色”“清除一行”，都可以编码成字节序列。

  1. 传输接口就是普通的读写

  在图形终端里运行 Vim，输出路径是：

  Vim
    │ write(终端 fd, 字节串, 长度)
    ▼
  PTY slave
    │ 内核传输
    ▼
  PTY master
    │ read(...)
    ▼
  Kitty / Ghostty
    │ 解析文字和控制序列
    ▼
  更新字符网格、光标和颜色，再绘制到窗口

  前面说的 ioctl(TIOCGWINSZ) 用来查询终端属性；绘制内容主要通过输出字节来表达。

  2. “移动光标并显示红字”实际发送什么

  假设要在第 3 行、第 10 列显示红色的 Hello，可以发送：

  ESC[3;10H ESC[31m Hello ESC[0m

  上面的空格只是为了分隔各部分，实际可以连在一起。这里 ESC 表示一个值为 0x1b 的字节。

  你可以直接执行：

  printf '\033[3;10H\033[31mHello\033[0m\n'

  终端会依次解释：

   字节序列     含义
  ━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   ESC[3;10H    将光标移到第 3 行、第 10 列，通常以左上角为原点，从 1 开始
  ───────────  ────────────────────────────────────────────────────────────
   ESC[31m      将后续文字的前景色设为红色
  ───────────  ────────────────────────────────────────────────────────────
   Hello        在当前光标位置写入文字，并推进光标
  ───────────  ────────────────────────────────────────────────────────────
   ESC[0m       恢复默认文字样式

  类似的还有 ESC[2J 清除显示区域、ESC[K 清除光标到行末的内容。这些序列在 XTerm 控制序列文档
  (https://invisible-island.net/xterm/ctlseqs/ctlseqs.html) 中都有定义。

  3. 这套协议叫什么？

  日常通常统称 ANSI escape sequences，但实际是几层历史规范和扩展的组合：

  - ANSI / ECMA-48：定义许多基本控制功能及序列格式。
  - DEC VT100 及后续终端的控制序列：大量终端兼容行为的来源。
  - xterm 和现代终端扩展：增加鼠标报告、备用屏幕、更多颜色等能力。

  所以基础功能广泛兼容，但不同终端对扩展功能的支持存在差异。Ghostty 也把这部分明确称为 Terminal API（VT）
  (https://ghostty.org/docs/vt)。

  4. Vim 怎么知道该发送哪一种序列？

  Vim 会参考 $TERM 标识的终端类型，以及 terminfo / termcap 能力数据库和自己的内置终端定义。

  这些定义描述的是：

  “移动光标”这个能力 → 应当发送什么字节模板
  “清除到行末”       → 应当发送什么字节串
  “设置前景色”       → 应当发送什么字节模板

  因此，Vim 可以围绕“移动光标”“设置属性”等操作组织自己的屏幕更新，再把它们转换成终端认识的序列。Vim 的 term.txt 文档
  (https://vimhelp.org/term.txt.html) 中，t_cm 就是光标定位能力。

  在 Vim 的 src/term.c (https://github.com/vim/vim/blob/master/src/term.c) 中，out_str()、out_char() 等函数组织输出，out_flush()
  再通过 ui_write() 发送缓冲的数据。

  5. 终端为什么能解析连续混在一起的文字和命令？

  终端内部有一个解析状态机。例如：

  普通状态：遇到 H → 显示 H

  遇到 ESC → 进入转义状态
  遇到 [   → 进入 CSI 控制序列状态
  遇到 3;10 → 收集参数
  遇到 H   → 执行“光标定位”，回到普通状态

  因此，同样是 H，在普通状态下是文字，在 ESC[3;10H 里则是命令的结束字符。序列即使被拆成多次
  read()，解析器也会保留状态，继续处理。XTerm 解析规则 (https://invisible-island.net/xterm/ctlseqs/ctlseqs.html)

  在 Kitty/Ghostty 这个场景里，内核 PTY 不解释“红色”或“移动光标”；解释这些绘制命令的是终端模拟器。 Vim 决定显示什么，终端模拟器
  负责把字符、样式和光标状态画出来。

## TODO 那么为什么我发现在 vim 使用的 terminal 和普通的有点不同的?

但是我理解本质上实现原理相同的:
```txt
🤒  bash /home/martins3/data/vn/a.sh
pid 453887: no ptmx fd
pid 453888 fd 39 (/dev/ptmx) -> /dev/pts/22
	pid 453975   (zsh)             fds 0,1,2,10  ctty
	pid 454118   (htop)            fds 0,1,2  ctty
pid 453888 fd 40 (/dev/ptmx) -> /dev/pts/22
	pid 453975   (zsh)             fds 0,1,2,10  ctty
	pid 454118   (htop)            fds 0,1,2  ctty
pid 453888 fd 41 (/dev/ptmx) -> /dev/pts/22
	pid 453975   (zsh)             fds 0,1,2,10  ctty
	pid 454118   (htop)            fds 0,1,2  ctty
pidfd_open: No such process
pid 502519: no ptmx fd
pid 502520 fd 46 (/dev/ptmx) -> /dev/pts/29
	pid 1476556  (zsh)             fds 0,1,2,10  ctty
	pid 1476685  (node)            fds 0,1,2  ctty
	pid 1476692  (codex)           fds 0,1,2  ctty
	pid 1477026  (node_repl)         ctty
	pid 1479404  (codex-code-mode)   ctty
pid 502520 fd 47 (/dev/ptmx) -> /dev/pts/29
	pid 1476556  (zsh)             fds 0,1,2,10  ctty
	pid 1476685  (node)            fds 0,1,2  ctty
	pid 1476692  (codex)           fds 0,1,2  ctty
	pid 1477026  (node_repl)         ctty
	pid 1479404  (codex-code-mode)   ctty
pid 502520 fd 49 (/dev/ptmx) -> /dev/pts/29
	pid 1476556  (zsh)             fds 0,1,2,10  ctty
	pid 1476685  (node)            fds 0,1,2  ctty
	pid 1476692  (codex)           fds 0,1,2  ctty
	pid 1477026  (node_repl)         ctty
	pid 1479404  (codex-code-mode)   ctty
pid 771283: no ptmx fd
pid 3044360: no ptmx fd
pid 3442892: no ptmx fd
pid 3442893 fd 47 (/dev/ptmx) -> /dev/pts/27
	pid 3521282  (zsh)             fds 0,1,2,10  ctty
	pid 3521618  (claude)          fds 0,1,2,7,8,9  ctty
pid 3442893 fd 48 (/dev/ptmx) -> /dev/pts/27
	pid 3521282  (zsh)             fds 0,1,2,10  ctty
	pid 3521618  (claude)          fds 0,1,2,7,8,9  ctty
pid 3442893 fd 49 (/dev/ptmx) -> /dev/pts/27
	pid 3521282  (zsh)             fds 0,1,2,10  ctty
	pid 3521618  (claude)          fds 0,1,2,7,8,9  ctty
pid 3442986: no ptmx fd
pid 3603726: no ptmx fd
pid 3603727: no ptmx fd
```

## minicom 和 screen

使用 minicom 来连接: minicom -D /dev/pts/5 qemu 提供的 pts ，最后是
可以登录的。

https://salsa.debian.org/minicom-team/minicom

难道接受的字符流需要特殊处理?

还是需要买一个 serial 吧，测试下 ttyUSB0 吧

TODO : 我感觉这里有一些非常有意思的，有点不能理解
minicom 和 screen 距离很远，为什么看上去他们在一个路线上啊?

## Writing Programs with Ncurses (invisible-island.net)
https://news.ycombinator.com/item?id=43452789

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
