# unlink() 和 remove()

`unlink()` 删除非目录对象的目录项；`remove()`
还支持删除空目录。对普通文件，两者的删除语义相同。

`unlink()` 声明在 `<unistd.h>`，属于 POSIX 接口，Linux
提供对应系统调用。`remove()` 声明在 `<stdio.h>`，属于 C 标准库接口；POSIX
进一步规定它对非目录相当于 `unlink()`，对目录相当于 `rmdir()`。Linux 没有名为
`remove` 的系统调用。

两者都不是递归删除接口，`remove()` 不等于 `rm -r`。错误码表面向 Linux，本 demo
对非空目录检查 `ENOTEMPTY`；POSIX 也允许该情况返回 `EEXIST`。

## 一共存在那些系统调用

Linux 内核提供了几个相关系统调用：

 系统调用                        用途
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 unlink(path)                    删除非目录对象的目录项
──────────────────────────────  ──────────────────────────────────────────────────────────────────────────────────
 rmdir(path)                     删除空目录
──────────────────────────────  ──────────────────────────────────────────────────────────────────────────────────
 unlinkat(dirfd, path, flags)    支持相对目录 fd 定位；flags=0 时类似 unlink()，flags=AT_REMOVEDIR 时类似 rmdir()

remove() 是 libc 函数，内核没有对应的 remove 系统调用。 刚才本机的 strace 验证了：remove() 先调用 unlink()；如果返回 EISDIR，再调用
rmdir()。

另外，不同架构提供的系统调用集合有所不同，例如原生 arm64 使用 unlinkat()，libc 可以用它实现 unlink() 和 rmdir()。所以 C 函数名不一
定对应同名的内核系统调用。


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
