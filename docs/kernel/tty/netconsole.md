# netconsole

将 printk 的信息输出到网络中，而非

内核文档:
- https://www.kernel.org/doc/Documentation/networking/netconsole.txt

具体使用还是参考
- https://wiki.ubuntu.com/Kernel/Netconsole
- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/networking_guide/sec-configuring_netconsole

配置相当麻烦，需要源端的 ip 和网卡名称，我的鬼鬼啊

```c
static struct console netconsole = {
	.name	= "netcon",
	.flags	= CON_ENABLED,
	.write	= write_msg,
};
```

## console 的代码

Linux 内核里 “console” 有好几层，先按层次给你定位（这是 linux-drm 树，最后单独说 DRM 那条链）：

1. printk 的 console 注册框架（核心）

- include/linux/console.h:338 — struct console 定义（write/device/setup 等回调）
- kernel/printk/printk.c:4060 — register_console()，printk.c:4325 — unregister_console()
- kernel/printk/nbcon.c — 新式 atomic/nbcon console
- kernel/printk/console_cmdline.h — console= 启动参数解析
- include/linux/printk.h

2. VT 虚拟终端层（/dev/ttyN、控制台切换）

- drivers/tty/vt/vt.c、vt_ioctl.c、keyboard.c、consolemap.c、vc_screen.c
- include/linux/console_struct.h、include/linux/vt_kern.h、include/linux/vt.h、include/linux/consolemap.h

3. 文本模式 console 驱动（con_ops / VT 后端）

- drivers/video/console/ — vgacon.c、dummycon.c、sticon.c、newport_con.c
- drivers/video/fbdev/core/fbcon.c（还有 bitblit.c、softcursor.c、fbcon_rotate*.c）— framebuffer 文本控制台

4. 具体的 struct console 后端（真正调 register_console）

┌─────────────────────┬─────────────────────────────────────────────────┐
│        类型         │                      位置                       │
├─────────────────────┼─────────────────────────────────────────────────┤
│ 串口 console        │ drivers/tty/serial/（8250 等，*_console.c）     │
├─────────────────────┼─────────────────────────────────────────────────┤
│ HVC                 │ drivers/tty/hvc/hvc_console.c                   │
├─────────────────────┼─────────────────────────────────────────────────┤
│ virtio-console      │ drivers/char/virtio_console.c                   │
├─────────────────────┼─────────────────────────────────────────────────┤
│ netconsole          │ drivers/net/netconsole.c                        │
├─────────────────────┼─────────────────────────────────────────────────┤
│ early_printk        │ arch/x86/kernel/early_printk.c                  │
├─────────────────────┼─────────────────────────────────────────────────┤
│ ttynull / ttyprintk │ drivers/tty/ttynull.c、drivers/char/ttyprintk.c │
└─────────────────────┴─────────────────────────────────────────────────┘

5. DRM 侧的 console（这个树里你多半关心这个）

DRM 自己不直接实现 struct console，而是通过 fbdev 模拟 + fbcon 挂到 VT：
- drivers/gpu/drm/drm_fb_helper.c — fbdev 模拟核心
- drivers/gpu/drm/drm_client.c、drm_client_modeset.c、drm_client_sysrq.c
- drivers/gpu/drm/clients/drm_fbdev_client.c、drm_client_setup.c — 新的 DRM fbdev client
- drivers/gpu/drm/drm_fbdev_{shmem,dma,ttm}.c — 各内存后端
- drivers/gpu/drm/sysfb/simpledrm.c — simple-framebuffer
- 头文件：include/drm/drm_fb_helper.h、include/drm/drm_client.h

6. 周边

- fs/proc/consoles.c — /proc/consoles
- kernel/power/console.c — 休眠时切 console
- Documentation/fb/fbcon.rst

入口建议：看 printk 侧从 include/linux/console.h + printk.c:4060 的 register_console() 入手；看 DRM 显示通路从
drm_fb_helper.c → fbcon.c → vt.c 这条链走。需要我顺着哪条链细看（比如 nbcon 或 drm_fbdev client）？


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
