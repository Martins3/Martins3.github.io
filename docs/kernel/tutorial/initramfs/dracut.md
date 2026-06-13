# dracut
- 源码: https://github.com/dracutdevs/dracut

- https://github.com/dracut-ng/dracut-ng : 这才是源码

```sh
dracut --list-modules
dracut -f --add-drivers sha3_generic /boot/vmlinuz-4.19.90
```
- 手动 initramfs 压缩和解压缩的办法:
  - https://access.redhat.com/solutions/24029
- 配置 drucut 来实现加入模块
  - https://www.suse.com/support/kb/doc/?id=000019945 :

安装完驱动 ko 之后，可以这样
```sh
dracut -v --force --kver $(uname -r)
kdumpctl rebuild
```
## man/ 都需要看看

```txt
dracut
dracut-cmdline.service
dracut.modules
dracut-pre-trigger.service
dracut.bootup
dracut.conf
dracut-mount.service
dracut-pre-udev.service
dracut-catimages
dracut-initqueue.service
dracut-pre-mount.service
dracut-shutdown.service
dracut.cmdline
dracut.kernel
dracut-pre-pivot.service
```

1. dracut.kernel(8) 提供了很多 kernel 参数

## dracut-pre-mount

## 子命令
### lsinitrd

## 其他
mkinitrd 被删掉了 https://github.com/dracutdevs/dracut/commit/43df4ee274e7135aff87868bf3bf2fbab47aa8b4

如果制作 dracut 看看 build.tar.sh 吧

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
