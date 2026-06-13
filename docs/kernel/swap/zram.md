## zram 基本使用
参考文档，内容应该是说很清楚了:
Documentation/admin-guide/blockdev/zram.rst

- http://www.wowotech.net/memory_management/zram.html : 就是这个吗 ?
  - https://github.com/maximumadmin/zramd

如果没有加载模块:
```sh
modprobe zram num_devices=1
```

如果已经加载了:
```sh
sudo zramctl -s 32G /dev/zram0
sudo mkswap /dev/zram0
sudo swapon /dev/zram0
```

fedora 中删掉这个来禁用 zram :
sudo dnf remove zram-generator-defaults

## 临时 disable 一下
dev-zram0.swap

似乎只有这样才可以彻底禁用:
```txt
sudo touch /etc/systemd/zram-generator.conf
sudo systemctl daemon-reload
sudo swapoff /dev/zram0
sudo systemctl stop systemd-zram-setup@zram0.service
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
