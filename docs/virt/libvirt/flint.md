## flint
https://github.com/volantvm/flint
curl -fsSL https://raw.githubusercontent.com/volantvm/flint/main/install.sh | bash

sudo lvextend -l +100%FREE  /dev/fedora_linux/root
sudo xfs_growfs /home

our API key will be automatically generated and saved to:
  /home/martins3/.flint/config.json

然后执行 sudo flint serve 关掉 firewalld

## 值得观察的东西

1. pxe 的方式来安装系统
2. 存储池和存储卷的概念
3. Network Filters 配置，也是使用 xml 的
4. 镜像与模版的概念
5. cloudinit
6. 创建网络的各种选项
	- dhcp
7. 如何使用 libvirt 的


## 配置文件
```txt
{
  "server": {
    "host": "0.0.0.0",
    "port": 5550,
    "read_timeout": 30,
    "write_timeout": 30
  },
  "security": {
    "rate_limit_requests": 100,
    "rate_limit_burst": 20,
    "passphrase_hash": "$2a$12$f3WUe82mgGslg.2CA053x.UkAoM4ejWKma5AbTHMdzY6heXycYuEC"
  },
  "libvirt": {
    "uri": "qemu:///system",
    "iso_pool": "isos",
    "template_pool": "templates",
    "image_pool_path": "/var/lib/flint/images"
  },
  "logging": {
    "level": "INFO",
    "format": "json"
  }
}
```

```txt
 sudo virsh pool-list --all
[sudo] password for martins3:
 Name                  State    Autostart
-------------------------------------------
 flint-image-library   active   yes
 isos                  active   yes
 test                  active   yes
```

## 但是，很遗憾，我连虚拟机都创建不了
1. iso 需要上传，但是不知道如何直接指定?
2. 存储只能创建，不能删
3. 没有办法创建虚拟机

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
