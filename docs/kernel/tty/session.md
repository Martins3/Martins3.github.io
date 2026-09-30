## uptime 展示的 user 数量

```txt
🧀  uptime
 08:48:20 up 27 days, 22:43,  2 users,  load average: 1.21, 1.44, 1.26
```

我前面用的是 who、w 和 loginctl list-sessions --no-legend。

  进一步核对后，本机 uptime 通过 systemd 的会话接口获取这个数量，调用关系是               ：

  uptime
    → libproc2 的 procps_users()
      → sd_get_sessions()        列出会话
      → sd_session_get_class()   获取会话类别

  procps_users() 统计类别以 user 开头的会话。procps 源码中的 procps_users()
  (https://gitlab.com/procps-ng/procps/-/blob/4740a0efa79cade867cfc7b32955fe0f75bf5173/library/uptime.c)

  这些接口底层读取 /run/systemd/sessions/ 中的文件。systemd 接口实现
  (https://github.com/systemd/systemd/blob/main/src/libsystemd/sd-login/sd-login.c)

  本机文件里的记录是：

   会话 ID    类别       终端    是否计数
  ━━━━━━━━━  ━━━━━━━━━  ━━━━━━  ━━━━━━━━━━
   462        user       tty2    是
  ─────────  ─────────  ──────  ──────────
   520        user       tty3    是
  ─────────  ─────────  ──────  ──────────
   1          manager    无      否

  所以结果是 2 users。我也用 strace 验证了实际读取路径，你可以执行：

  strace -e trace=openat uptime

  输出中会看到它打开上述三个会话文件。

### 我感觉和经常看到的 setsid 不是一个东西

## controlling terminal、session、process group、job control、setsid 和信号的关系。

## last 命令

last reboot 是一个好命令，但是为什么 man last(1) 的时候发现，这个东西也是和 tty 有关的


## setsid 是做什么的，每次在写 initramfs 的 shell 的时候就会遇到这个问题。

docs/linux/tlpi/3/tlpi-chapter-34.md 之前分析过 job-control 的

## 看看类似 busybox 中的是如何处理 tty 的

当然，这里也是一个
https://github.com/Sweets/hummingbird

似乎，主要就是处理下 setsid 之类的，不过可以继续调查下吧。

## Fixing Ctrl+C in Rust terminal apps: Child process management (fiveonefour.com)
110 points by Callicles on July 30, 2025 | hide | past | favorite | 58 comments

https://news.ycombinator.com/item?id=44728796

## TODO
http://www.wowotech.net/345.html

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
