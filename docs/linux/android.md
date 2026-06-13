## 有趣的
https://github.com/termux/termux-packages

## 这也太容易了吧
不过，现在需要让我可以 ssh 到机器上

可以拉取文档

ifconfig 获取 ip
whoami 获取 user
passwd 配置密码

然后:
```txt
ssh u0_3a74@192.168.2.188 -p 8022
```

pkg install 基本可以安装常用的任何命令:
```txt
 fastfetch
         -o          o-             u0_a374@localhost
          +hydNNNNdyh+              -----------------
        +mMMMMMMMMMMMMm+            OS: Android REL 16 aarch64
      `dMMm:NMMMMMMN:mMMd`          Host: Xiaomi REDMI K90 Pro Max (25102RK)
      hMMMMMMMMMMMMMMMMMMh          Kernel: Linux 6.12.23-android16-5-g6928k
  ..  yyyyyyyyyyyyyyyyyyyy  ..      Uptime: 21 days, 21 hours, 17 mins
.mMMm`MMMMMMMMMMMMMMMMMMMM`mMMm.    Packages: 114 (dpkg)
:MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM:    Shell: bash 5.3.9
:MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM:    Display: 1200x2608 @ 3x [Built-in]
:MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM:    DE: HyperOS 3.0.20.0.WPMCNXM
:MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM:    WM: WindowManager (SurfaceFlinger)
-MMMM-MMMMMMMMMMMMMMMMMMMM-MMMM-    Terminal: linker64
 +yy+ MMMMMMMMMMMMMMMMMMMM +yy+     CPU: Qualcomm Snapdragon 8 Elite Gen 5 z
      mMMMMMMMMMMMMMMMMMMm          GPU: Qualcomm Adreno (TM) 840 [Integrat]
      `/++MMMMh++hMMMM++/`          Memory: 9.46 GiB / 14.65 GiB (65%)
          MMMMo  oMMMM              Swap: 4.71 GiB / 16.00 GiB (29%)
          MMMMo  oMMMM              Disk (/): 845.98 MiB / 845.98 MiB (100%]
          oNMm-  -mMNs              Disk (/storage/emulated): 122.05 GiB / e
                                    Local IP (rmnet_data5): 10.67.0.234/30
                                    Local IP (tun0): 172.19.0.1/30
                                    Local IP (wlan0): 192.168.2.188/24
                                    Locale: en_US.UTF-8
```

安装 neovim make npm git
pkg install nodejs-lts

然后打开 neovim 基本可以自动安装

给墨水屏安排上吧，然后添加上几条基本命令基本上就可以了，不用执着于 nix 了。

## nix
https://github.com/nix-community/nix-on-droid


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
