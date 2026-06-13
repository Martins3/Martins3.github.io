# tty0

在 alpine 虚拟机中做实验:

ssh 登录到虚拟机中，执行:
```txt
for file in /dev/tty*; do
        printf '%s\n' "$file"
        echo "$file" | sudo tee "$file"
done
```
发现 vnc 屏幕上会有两个结果，也就是 tty0 和 tty1

原因:

- /dev/tty0 不是某个固定的终端，而是当前活动的虚拟控制台（active virtual console）。你向它写入的内容，内核会直接显示在当前的 VT  上。
- /dev/tty1 是第一个虚拟控制台。很多 Linux 发行版的图形界面（X/Wayland）就运行在这个 tty 上，或者可以通过 Ctrl+Alt+F1 切换到它。

所以你的循环执行到这两步时：

```bash
  echo "/dev/tty0" | sudo tee /dev/tty0   # 写入当前活动控制台
  echo "/dev/tty1" | sudo tee /dev/tty1   # 写入 tty1
```

这些内容被内核直接渲染到了帧缓冲区（framebuffer）上。

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
