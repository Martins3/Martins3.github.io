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

## rescue 内核做什么的
由 dracut-config-rescue 组件在安装内核时生成。

具体流程：

- vmlinuz-0-rescue-<machine-id>：并非重新编译的特殊内核，而是当时普通 / boot/vmlinuz-<版本> 的副本。
- initramfs-0-rescue-<machine-id>.img：由 dracut --no-hostonly -a rescue 生成，包含更通用的驱动和救援工具。

- 生成脚本是：
```txt
    - /etc/kernel/postinst.d/51-dracut-rescue-postinst.sh
    - /usr/lib/kernel/install.d/51-dracut-rescue.install
```

对应 RPM：dracut-config-rescue-059-6.oe2403.x86_64

用途是在普通内核或 initramfs 无法启动时，从 GRUB 选择 Rescue Image，
进入救援环境，用来修复文件系统、initramfs、GRUB、fstab、密码或其他启动问题。

文件名最后的：

c5bec1de6ac948de897bcbb6eb6ef32f

是生成时的 /etc/machine-id，不是内核版本。
如果它与当前 /etc/machine-id 不一致，通常说明系统曾被克隆、重装或修改过 machine-id，
文件可能是旧系统遗留的。

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
