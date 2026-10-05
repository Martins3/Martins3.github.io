# serial Terminal

## minicom 配置

```bash
# 启动 minicom
$ sudo minicom -D /dev/ttyUSB0 -b 115200

# 进入配置菜单 (Ctrl+A, O)
# - Serial port setup
# - Save setup as dfl

# 常用快捷键
Ctrl+A, Q  # 退出
Ctrl+A, X  # 重置
Ctrl+A, L  # 开启日志
```

## picocom（更简洁）

```bash
# 启动 picocom
$ picocom -b 115200 /dev/ttyUSB0

# 带日志
$ picocom -b 115200 --logfile serial.log /dev/ttyUSB0

# 退出 (Ctrl+A, Ctrl+X)
```

## screen

```bash
# 使用 screen 连接串口
$ screen /dev/ttyUSB0 115200

# 退出 (Ctrl+A, k, y)
```

这个的确有点奇怪了，但是的确，screen 是有两个功能的

## putty
- https://www.chiark.greenend.org.uk/~sgtatham/putty/

文档:
https://the.earth.li/~sgtatham/putty/0.83/htmldoc/

太复杂了
https://git.tartarus.org/simon/putty.git

putty 自己实现了 ssh 协议，很 nb


## TODO
2. https://news.ycombinator.com/item?id=43346816 看看
1. Xshell 做什么的?

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
