## relay fs

> [!NOTE]
> 参考神奇海螺的意见，有待验证

Documentation/filesystems/relay.rst 对应的核心代码是：

- 实现：kernel/relay.c:1
- API/数据结构：include/linux/relay.h:1
- Kconfig：init/Kconfig:1461
- 一个典型使用者：blktrace，kernel/trace/blktrace.c:715

它虽然放在 Documentation/filesystems/ 下面，但现在重点不是“一个独立文件系统”，而是一个 kernel -> userspace 的高吞吐数据传输接口。历史上叫 relayfs，后来
变成通用 relay interface，通常借助 debugfs 暴露文件。

核心作用：

内核模块/子系统
   |
   | relay_write() / relay_reserve()
   v
per-cpu relay buffer
   |
   | relay_file_operations
   v
debugfs 文件
   |
   | read() / mmap() / poll()
   v
用户态工具

典型场景是内核里有大量 trace/log/binary event 要导出给用户态，比如：

- blktrace 导出块层 I/O trace
- 无线驱动导出 spectral scan / fwlog
- GPU 驱动导出 GuC log
- WWAN 驱动导出 modem trace

主要对象在 include/linux/relay.h:50：

struct rchan_buf   // 每 CPU 一个 relay buffer
struct rchan       // 一个 relay channel
struct rchan_callbacks

关键 API：

relay_open()       // 创建 relay channel
relay_close()      // 关闭
relay_flush()      // 刷出当前 sub-buffer
relay_write()      // 拷贝数据写入当前 CPU buffer
__relay_write()    // 类似 relay_write，但只关 preempt
relay_reserve()    // 预留空间，调用者自己填数据

以 blktrace 为例：

bt->rchan = relay_open("trace", dir, buf_size, buf_nr,
                       &blk_relay_callbacks, bt);

见 kernel/trace/blktrace.c:715。

它会在 debugfs 下创建类似 trace0, trace1, ... 的文件，每个 CPU 一个 buffer 文件。写事件时用：

t = relay_reserve(bt->rchan, trace_len);

见 kernel/trace/blktrace.c:124。

用户态工具再通过 read() 或 mmap() 从这些 debugfs 文件里拿数据。

所以一句话理解：

> relay 是内核提供给 tracing/logging 类子系统的高速数据通道，用 per-cpu buffer 减少竞争，再通过 debugfs 文件把数据暴露给用户态读取。


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
