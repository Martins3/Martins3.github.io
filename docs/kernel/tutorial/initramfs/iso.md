# initramfs : iso

简单来说，就是直接使用 iso 启动，
iso 中提供了启动需要的所有环境，
然后 iso 执行脚本，把盘格式化，mount ，然后把所有的包安装到那个盘。

最后重启。


## 原来打包的构建在这里的
https://github.com/rhinstaller/anaconda

排查问题的方法是:

https://access.redhat.com/solutions/5749321

参考:
chroot /mnt/sysimage/
dnf history info 1 | egrep -B1 'Scriptlet|exit status'

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
