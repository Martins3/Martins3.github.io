# 为什么有 sigpipe ?
<!-- 2ff1cbc3-c12b-4d27-908e-21bb6ba46d6b -->

https://stackoverflow.com/questions/8369506/why-does-sigpipe-exist
看了一下，其实没什么道理，只能说是历史遗留因素吧。

SIGPIPE 触发条件：向一个 已关闭读端 的 socket 写数据。

在 main() 开头加一行 signal(SIGPIPE, SIG_IGN);，然后检查 write() 返回值和 errno == EPIPE，
是最简单、安全、可移植的方案。

demo : ./code/sigpipe.c

docs/shell/bash.md 中的 sigpipe 问题也是痛苦面具
具体参考 docs/shell/bash.md 吧


## EPIPE 和 ECONNRESET 是什么关系?
https://stackoverflow.com/questions/12890393/whats-the-difference-between-broken-pipe-and-connection-reset-by-peer

demo : code/src/c/signal/sigpipe.c
就是很容易测试出来 SIGPIPE

## 当一个 systemd 服务被强制重启的时候，那么这个服务接受到的是什么信号?
<!-- 30584c58-a1e5-4514-8c52-45a3447beadd -->

首先使用 SIGTERM 请求其优雅的退出 ，然后使用 SIGKILL 来强制退出
```txt
🧀  systemctl show sshd | grep -E "(KillSignal|TimeoutStopSec|SendSIGKILL)"
KillSignal=15
RestartKillSignal=15
FinalKillSignal=9
SendSIGKILL=yes
SurviveFinalKillSignal=no
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
