# ttyprintk

 CONFIG_TTY_PRINTK 提供一个虚拟 TTY：把写入 /dev/ttyprintk 的用户态文字转成 printk 内核日志。这样，启动脚本等程序的输出就能和内核消息一起出现在 dmesg
  中，方便按时间排查启动问题。

  例如，把 启动网络服务 写入该设备，日志里会出现带 [U] 前缀的记录。日志级别由 CONFIG_TTY_PRINTK_LEVEL 控制，默认是 6（info）。设置 console=ttyprintk 则可
  让写往 /dev/console 的用户态消息走这条路径。

  它不会自动收集所有程序的标准输出，也不是实际的屏幕或串口。普通使用场景保持 N 即可。实现见 drivers/char/ttyprintk.c:29。

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
