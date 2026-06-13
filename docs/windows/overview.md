看下 C 盘里面都有什么东西:

使用 gdu 分析，好吧，的确没什么奇怪的东西:
```txt
. 209.0 GiB ██▋       ▏/Users
  173.5 GiB ██▏       ▏/WeGameApps
   99.9 GiB █▎        ▏/Windows.old
.  81.8 GiB █         ▏/Program Files
   60.3 GiB ▋         ▏/Xilinx
.  39.8 GiB ▌         ▏/$Recycle.Bin
.  33.1 GiB ▍         ▏/Windows
   29.9 GiB ▎         ▏/Program Files (x86)
   12.7 GiB           ▏hiberfil.sys
.  10.9 GiB           ▏/ProgramData
    2.0 GiB           ▏pagefile.sys
    1.7 GiB           ▏/CloudMusic
    1.1 GiB           ▏/baidunetdiskdownload
   16.0 MiB           ▏swapfile.sys
  109.1 KiB           ▏appverifUI.dll
   65.7 KiB           ▏vfcompat.dll
   12.0 KiB           ▏/LegionZone
   12.0 KiB           ▏DumpStack.log.tmp
   12.0 KiB           ▏DumpStack.log
    8.0 KiB           ▏/OneDriveTemp
```

hiberfil.sys 和 pagefile.sys 原来是这样的

还有一个 windows.old 是做什么的?

## 看看有没有办法实现找到开源的 windows ，然后编译一个出来

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
