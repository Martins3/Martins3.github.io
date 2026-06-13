# UACPI

https://github.com/uACPI/uACPI

构建方法:
1. 做出这个修改:
```diff
diff --git a/meson.build b/meson.build
index 22f64142f5d7..9c03826579a0 100644
--- a/meson.build
+++ b/meson.build
@@ -23,3 +23,4 @@ sources = files(
 )
 
 includes = include_directories('include')
+library('uacpi', sources, include_directories: includes, install: true)
```

2. 构建
```sh
meson setup builddir
cd builddir
ninja # 会失败，但是有 compile_commands.json
```

目前就这样了，不会在 qemu 中验证的。
如果利用好 qemu 的 -kernel 功能，加上这个项目。
就可以搞一些小的测试内容了。



简单看看源码:
- uacpi_initialize

## 使用 UACPI 的项目
https://github.com/managarm/managarm



## 配合食用
https://wiki.osdev.org/UACPI#Namespace_Enumeration_&_Finding_Devices

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
