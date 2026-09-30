# n_tty

- n_tty_receive_char_canon

```c
struct ktermios tty_std_termios = {	/* for the benefit of tty drivers  */
	.c_iflag = ICRNL | IXON,
	.c_oflag = OPOST | ONLCR,
	.c_cflag = B38400 | CS8 | CREAD | HUPCL,
	.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK |
		   ECHOCTL | ECHOKE | IEXTEN,
	.c_cc = INIT_C_CC,
	.c_ispeed = 38400,
	.c_ospeed = 38400,
	/* .c_line = N_TTY, */
};
```


## ctrl w
• 默认终端使用 N_TTY line discipline，Ctrl-W 即 ASCII 0x17，对应 termios.c_cc[VWERASE]：

  • 默认值定义：include/linux/termios_internal.h:21

    [VWERASE] = 'W' - 0x40   /* 0x17 */

  • 字符宏：include/linux/tty.h:48

    #define WERASE_CHAR(tty) ((tty)->termios.c_cc[VWERASE])

  实际处理路径是：

  tty_ldisc_receive_buf()
    -> n_tty_receive_buf2()
    -> n_tty_receive_buf_common()
    -> __receive_buf()
    -> n_tty_receive_buf_standard()
    -> n_tty_receive_char_special()
    -> n_tty_receive_char_canon()
    -> eraser()

  关键位置：

  • 将 VWERASE 放入特殊字符表：drivers/tty/n_tty.c:1807
  • 判断并进入删除处理：drivers/tty/n_tty.c:1245
  • Ctrl-W 选择 WERASE 类型：drivers/tty/n_tty.c:937
  • 按单词回退删除的核心循环：drivers/tty/n_tty.c:958

  生效条件是 canonical mode (ICANON) 且 IEXTEN 开启。如果是 raw/non-canonical 模式，Ctrl-W 通常会原样交给用户程序。


## ctrl c


也在 N_TTY line discipline 中，识别入口是 n_tty_receive_char_special() (drivers/tty/n_tty.c:1338)：

if (L_ISIG(tty)) {
    if (c == INTR_CHAR(tty)) {
        n_tty_receive_signal_char(tty, SIGINT, c);
        return;
    }
    ...
}

INTR_CHAR(tty) 对应 termios.c_cc[VINTR]，默认值 (include/linux/termios_internal.h:22)为 'C' - 0x40，即 0x03。

后续调用链：

n_tty_receive_signal_char(tty, SIGINT, c)
  -> isig(SIGINT, tty)
    -> __isig(SIGINT, tty)
      -> kill_pgrp(tty_pgrp, SIGINT, 1)

实际发送信号在 __isig() (drivers/tty/n_tty.c:1044)：通过 tty_get_pgrp() 获取终端的前台进程组，向整个组发送
SIGINT。

isig() (drivers/tty/n_tty.c:1067) 还会在未设置 NOFLSH 时清空输入、输出和回显缓冲。

与 Ctrl-W 不同，Ctrl-C 的信号处理依赖 ISIG，不要求 ICANON，所以非 canonical 模式也可以生效。

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
