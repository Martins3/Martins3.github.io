## raid1 的开机过程
<!-- 70e3978a-db0d-44fb-ad8a-48d23ce957b3 -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

(总体是这个意思，那么我想知道的是，不过我想继续知道那些配置文件让知道两个盘是谁的)

一句话总结：`root` 在 `RAID1` 上时，启动的关键不是“固件会读 Linux RAID1”，而是“先把 `kernel + initramfs` 启起来，再由 initramfs 里的 `mdadm/dracut` 把 RAID1 组出来，最后把这个 md 设备挂成真正的根文件系统”。

- `/usr/lib/dracut/modules.d/90mdraid/parse-md.sh`
- `/usr/lib/dracut/modules.d/90mdraid/module-setup.sh`
- `/usr/lib/dracut/modules.d/98dracut-systemd/parse-root.sh`
- `/usr/lib/dracut/modules.d/98dracut-systemd/rootfs-generator.sh`

它们分别承担这些职责：

1. `parse-md.sh`

- 解析 `rd.md`、`rd.md.uuid`
- 决定是否启用 md RAID 自动组装
- 在指定 UUID 时只等待目标 md 设备出现

2. `module-setup.sh`

- 把 `mdadm` 安装进 initramfs
- 注入 md 相关 udev rules
- 安装 mdraid 的 hook 和脚本

3. `parse-root.sh`

- 解析 `root=/dev/...` 或 `root=UUID=...`
- 生成“等待 root 设备出现”的逻辑

4. `rootfs-generator.sh`

- 为 root 设备生成 systemd mount 单元
- 把它挂到 `/sysroot`

所以在现代发行版里，典型真实过程是：

```text
GRUB 先启动 kernel/initramfs
-> initramfs 中的 mdadm 组 RAID1
-> /dev/mdX 出现
-> 将 /dev/mdX 挂成 /
-> switch_root
-> 真正的 userspace 启动
```

## dm 的开机过程

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
