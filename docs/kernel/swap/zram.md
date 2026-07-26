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

## 打开的方法
3. 写入配置文件
创建 /etc/systemd/zram-generator.conf：
```ini
  [zram0]
  zram-size = 32768
```

4. 加载模块并激活
    - modprobe zram num_devices=1
    - systemctl daemon-reload
    - systemctl start systemd-zram-setup@zram0.service

现在 /dev/zram0 是 systemd 管理的 swap，大小 32G，优先级 100，开机会自动创建。

## 应该测试下压缩速度和压缩率是多少


## 似乎 zswap 和 zram 总是傻傻分不清楚?

zram
- 在内存里创建一个基于 ram 的块设备，先压缩，再当作 swap 用。
- 因为数据压缩后体积变小，相当于用少量内存换更多“逻辑 swap”。
- 缺点是它本身占用的是内存，压力越大，可用的真实内存越少。
- 典型场景：内存紧张的嵌入式设备、笔记本、WSL、容器、云主机，用来避免直接换出到慢速磁盘。
- 现在大多数发行版（Fedora、Ubuntu、Chrome OS、Android 等）默认开启。

zswap
- 在真正把页换出到磁盘 swap 之前，先在内存里做一个压缩缓存层。
- 命中时直接从压缩缓存解压，避免磁盘 I/O；缓存满了或内存不够再落到真实 swap。
- 不额外制造 swap 设备，而是作为现有磁盘 swap 的前端加速器。
- 典型场景：台式机/服务器已经有 SSD/HDD swap，想减少 swap 读写的 I/O 延迟和磨损。


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
