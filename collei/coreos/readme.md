
```txt
sudo coreos-installer install /dev/sda --insecure-ignition --ignition-url  http://10.0.2.2:50000:config.ign
```

## iso 下载
- https://fedoraproject.org/coreos/download?stream=stable#arches
## 文档

https://docs.fedoraproject.org/en-US/fedora-coreos/

似乎所有的东西都是通过配置文件来控制的

## 真的可以有这种方法吗?
-fw_cfg name=opt/com.coreos/config,file=/path/to/config.ign

## 这个东西就是我所需要的
https://docs.fedoraproject.org/en-US/fedora-coreos/debugging-kernel-crashes/

## Fedora Atomic Image
也不完全是一个东西
https://github.com/zdyxry/isengard

## coreos 中如何生成 kdump

当前 fsc vm rocky 中使用了 ostree 系统，默认 kdump kernel 中不支持 ostree，
导致 kernel crash 后进入 kdump 时初始化 ostree 相关分区失败，
从而导致 vmcore 文件写入失败。
可按如下步骤配置 kdump：
编辑 /etc/kdump.conf 文件，在 dracut_args 中添加 ostree 模块 dracut_args --add "ostree"；
重建 kdump initramfs kdumpctl rebuild;
重启 kdump 服务 systemctl restart kdump；
使用命令进行测试 echo c | sudo tee /proc/sysrq-trigger；

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
