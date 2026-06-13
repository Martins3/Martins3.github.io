## systemd-boot

替代 grub 工具本身:
https://wiki.archlinux.org/title/Systemd-boot

类似的:
https://news.ycombinator.com/item?id=40959742
https://news.ycombinator.com/item?id=40959742
https://news.ycombinator.com/item?id=43883040
先给安装一下 theme 吧

## kernel-install
```txt
这是 systemd 提供的标准工具，用于安装、更新和移除内核。

# 安装所有已安装的内核到引导分区
kernel-install add-all

# 安装特定版本内核
kernel-install add <kernel-version> <kernel-image>

# 移除内核
kernel-install remove <kernel-version>

配置文件: /etc/kernel/install.conf

layout=bls              # 布局类型: bls, uki, auto, other
initrd_generator=dracut # initrd 生成器
uki_generator=ukify     # UKI 生成器

支持的布局:

• bls: Boot Loader Specification Type 1（systemd-boot 标准格式）
• uki: BLS Type 2（统一内核镜像格式）
• other: 其他传统布局（如 GRUB）
```

## bootctl : 用于管理 systemd-boot 引导加载器本身。

```txt
# 安装 systemd-boot 到 ESP
bootctl install

# 查看引导状态
bootctl status

# 更新引导加载器
bootctl update

# 设置超时时间
bootctl set-timeout 5
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
