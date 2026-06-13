## DKMS

首先，和 [KMS](https://www.kernel.org/doc/html/latest/gpu/drm-kms.html) 没有什么关系的。

仓库 : https://github.com/dell/dkms

介绍 :

- https://wiki.archlinux.org/title/Dynamic_Kernel_Module_Support
- https://askubuntu.com/questions/408605/what-does-dkms-do-how-do-i-use-it 🌕 更加推荐

似乎就是方便的构建 out of tree 的 kernel module

## 经典项目
https://github.com/strongtz/i915-sriov-dkms

将 kernel-devel 和 dkms 的 rpm 都安装，然后执行这个
```sh
version=6.6
sudo dkms build -m mtgpu -v 2.6.5-000 --kernelsourcedir /lib/modules/$version/build -k $version
```
在驱动中是不是会好一点。

还有更加简单的方法，就是 dkms 的rpm 本来就是可以构建驱动的。

## 目前使用了关于 dkms 就是 nvidia

也就是:
```txt
dkms status
nvidia/575.57.08: added
```

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
