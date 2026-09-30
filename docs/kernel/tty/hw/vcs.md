# vcs

🧀  ls /dev/vcs*
/dev/vcs   /dev/vcs3  /dev/vcs6   /dev/vcsa2  /dev/vcsa5  /dev/vcsu1  /dev/vcsu4
/dev/vcs1  /dev/vcs4  /dev/vcsa   /dev/vcsa3  /dev/vcsa6  /dev/vcsu2  /dev/vcsu5
/dev/vcs2  /dev/vcs5  /dev/vcsa1  /dev/vcsa4  /dev/vcsu   /dev/vcsu3  /dev/vcsu6


可以把它理解成：/dev/ttyN 是虚拟终端的输入输出流，而 /dev/vcsN 是这个终端当前“屏幕画面”的内核接口。

## 1. 你列出的节点如何对应

这些设备都是字符设备，主设备号是 7：

 设备          含义
━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 /dev/vcs      当前前台虚拟控制台的字符内容
────────────  ───────────────────────────────────────────────
 /dev/vcs1     /dev/tty1 屏幕内容
────────────  ───────────────────────────────────────────────
 /dev/vcs2     /dev/tty2 屏幕内容
────────────  ───────────────────────────────────────────────
 /dev/vcsu1    tty1 屏幕内容，以 32 位 Unicode 值表示
────────────  ───────────────────────────────────────────────
 /dev/vcsa1    tty1 屏幕内容，加字符属性、屏幕尺寸和光标位置
────────────  ───────────────────────────────────────────────
 /dev/vcsa     当前前台控制台的字符内容加属性

对应关系是：

/dev/tty1  <->  /dev/vcs1
/dev/tty2  <->  /dev/vcs2
/dev/tty3  <->  /dev/vcs3

/dev/vcs、/dev/vcsu、/dev/vcsa 不带数字时，表示当前前台控制台。

你这里只有 1 到 6，通常说明当前内核已经分配了 6 个虚拟控制台。虚拟控制台是动态分配的，未分配的 tty7 不一定有 /dev/vcs7。

最大数量在当前源码中是 63：

#define MIN_NR_CONSOLES 1
#define MAX_NR_CONSOLES 63

位置：include/uapi/linux/vt.h:12

## 2. 它解决了什么问题

终端输入输出流和屏幕画面不是同一回事。

例如：

/dev/tty1

代表的是一个终端设备。程序往里面写入 ANSI 转义序列、字符、颜色控制等，虚拟终端驱动解析后，才会更新屏幕缓冲区。

但是，如果你想知道屏幕上“现在显示了什么”，读取 /dev/tty1 并不能得到屏幕内容。于是内核提供了：

/dev/vcsN

用于直接读取虚拟控制台的屏幕缓冲区。

它可以用于：

• 截取文本控制台画面
• 文本模式下的屏幕监控
• 无障碍软件读取控制台内容
• 控制台选择、复制、粘贴工具
• 保存和恢复控制台屏幕
• 监听控制台画面更新

源码文件头部也直接说明了它的目的：

/*
 * Provide access to virtual console memory.
 *
 * This replaces screendump and part of selection,
 * so that the system administrator can control access
 * using file system permissions.
 */

位置：drivers/tty/vt/vc_screen.c:2

早期系统主要依赖 screendump 等特殊接口。/dev/vcs* 把这个功能变成了普通设备文件，因此可以使用文件权限控制谁能读取或修改控制台内容。

## 3. 三种数据格式

### /dev/vcsN

每个屏幕单元返回一个字节，表示字符 glyph：

行数 * 列数 个字节

例如 80 列、25 行，大约返回 2000 字节。

它不是完整 UTF-8 文本，而是虚拟控制台内部的字符表示。直接 cat 时通常没有换行，因为换行不是额外存储的，而是由行列位置决定的。

### /dev/vcsaN

每个单元返回两个字节：

字符 + 属性

此外开头还有 4 个字节：

lines, columns, cursor_x, cursor_y

字符属性通常包含前景色、背景色、闪烁等信息。

源码注释：

/dev/vcsaN: ... including attributes,
prefixed with the 4 bytes lines,columns,x,y

### /dev/vcsuN

每个屏幕单元返回一个 4 字节 Unicode 值，适合需要 Unicode 语义的程序。

实现中，如果虚拟控制台没有启用 UTF-8，可能返回 -ENODATA。

## 4. 内核读写路径

以 /dev/vcs3 为例：

open("/dev/vcs3")
        |
        v
主设备号 7 的 vcs 驱动
        |
        v
drivers/tty/vt/vc_screen.c:vcs_fops
        |
        +-- vcs_open()
        +-- vcs_read()
        +-- vcs_write()
        +-- vcs_poll()
        |
        v
vc_cons[2].d
        |
        v
虚拟控制台屏幕缓冲区

核心文件操作定义在：

drivers/tty/vt/vc_screen.c:763

注册设备的代码在：

int __init vcs_init(void)

位置：drivers/tty/vt/vc_screen.c:791

读取时：

vcs_read()
    -> vcs_vc()
    -> vcs_read_buf_noattr()
    -> vcs_scr_readw()

写入时也可以修改屏幕缓冲区，但这不是向 shell 输入字符，也不会经过正常的 TTY 输入处理，而是直接修改显示内容：

vcs_write()
    -> vcs_write_buf_noattr()
    -> vcs_scr_writew()

因此一般只把它当作屏幕抓取接口使用。

## 5. 屏幕缓冲区在哪里

虚拟控制台的状态和屏幕内存主要由 struct vc_data 保存，相关实现位于：

• drivers/tty/vt/vt.c
• include/linux/console_struct.h

例如：

struct vc_data {
    ...
    unsigned long vc_origin;
    unsigned long vc_visible_origin;
    unsigned long vc_pos;
    unsigned int vc_screenbuf_size;
    ...
};

vc_origin 是控制台自身的屏幕缓冲区起点，vc_visible_origin 是当前实际正在查看的位置，因此 /dev/vcs 在发生控制台回滚时可以读取当前“看到
的”内容。

关键函数：

screenpos()
screen_glyph()
vcs_scr_readw()
vcs_scr_writew()

其中 screenpos() 位于：

drivers/tty/vt/vt.c:317

## 6. 为什么节点会动态出现

分配一个虚拟控制台时，vt.c 调用：

vcs_make_sysfs(currcons);

位置：drivers/tty/vt/vt.c:1131

释放虚拟控制台时调用：

vcs_remove_sysfs(currcons);

所以 /dev/vcs7 是否存在，取决于对应的虚拟控制台是否已经被内核分配，而不是简单地一次性创建全部 63 个节点。

设备号和名称的完整表格见：

Documentation/admin-guide/devices.txt:176

一句话概括：

/dev/ttyN  = 和虚拟终端通信
/dev/vcsN  = 查看或修改虚拟终端的屏幕缓冲区
/dev/vcsaN = 屏幕缓冲区 + 颜色属性
/dev/vcsuN = 屏幕缓冲区的 Unicode 表示


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
