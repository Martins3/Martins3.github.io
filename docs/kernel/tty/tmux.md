# 如何停止另外的 tmux

调试方法
```txt
tmux display-message -p 'socket=#{socket_path} session=#{session_name}'
tmux list-clients
```

```txt
# 展示的是当前的 tmux 的结果
socket=/tmp/tmux-1000/default session=11

# 展示的是当前正在使用的内容:
/dev/pts/22: 11 [153x42 xterm-ghostty] (attached,focused,UTF-8)
/dev/pts/8: 0 [153x42 xterm-ghostty] (attached,UTF-8)
```
如果想要踢掉另外一个 process ，需要指定一下

```txt
tmux attach -d -t 11
```

不过用这个是最好了:
```sh
tmux detach-client -a
```

有两个 active 的 session ，而且 session 11 同时在两个 terminal 中 attach 上了:
```txt
tmux list-clients

/dev/pts/8: 0 [153x42 xterm-ghostty] (attached,UTF-8)
/dev/pts/22: 11 [153x42 xterm-ghostty] (attached,focused,UTF-8)
/dev/pts/12: 11 [136x40 xterm-ghostty] (attached,focused,UTF-8)
```

## 是这样的吗?
```txt
nvim (pts/4, 前台进程)
  ← zsh (pts/4, pane shell, PPID = tmux server)
    ← tmux server (TTY=?, 持有 pts/4 的 master fd)   [通过 unix socket 与 client 通信]
      ← tmux client "tmux attach -d" (跑在某个 pts 上)
        ← 该 pts 的 master 持有者 = 真正的终端模拟器(alacritty fd30 → pts/40)
```

## screen
https://news.ycombinator.com/item?id=41483789

## 类似工具
https://github.com/alejandroqh/term39

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
