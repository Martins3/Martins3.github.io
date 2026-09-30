## 同时所有的程序都可以接受消息 ?

例如在 shutdown 的时候

实际上的测试结果:
```txt
wall hi

Broadcast message from martins3@oe2403-vhost-api (pts/1) (Sun Sep 27 23:17:34 2

hi
```

发送这类消息的机制叫 wall，你也可以用 wall '消息内容' 手动向已登录终端发送一条。Broadcast message from ...
是这类消息的标题。wall 手册 (https://www.man7.org/linux/man-pages/man1/wall.1.html)

关机时，具体发送进程取决于消息来源：在 systemd 系统上，shutdown 或 systemctl poweroff 会触发关机通知，通常由
systemd-logind 发送，并不一定会启动 /usr/bin/wall 程序。systemd-logind 手册
(https://man7.org/linux/man-pages/man8/systemd-logind.8.html) 如果标题写的是 Broadcast message from
systemd-journald@...，则是 systemd-journald 在转发日志。journald 配置手册
(https://www.freedesktop.org/software/systemd/man/252/journald.conf.html)

## 再例如
```txt
dracut -f --add-drivers sha3_generic /boot/vmlinuz-martins3-4.19.x86_64
Failed to install module sha3_generic

Broadcast message from systemd-journald@localhost.localdomain (Fri 2025-09-05 03:28:53 EDT):

dracut[5080]: Failed to install module sha3_generic


Message from syslogd@localhost at Sep  5 03:28:53 ...
 dracut:Failed to install module sha3_generic
```

## 如果内核触发日志，基本上没一个 tmux 的 windows 都可以收到这个消息
```txt
Message from syslogd@localhost at Nov  3 01:23:47 ...
 kernel:Disabling IRQ #16
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
