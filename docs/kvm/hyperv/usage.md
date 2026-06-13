# hypev 基本使用
## 没想到，真的没想到，windows 的 powershell 中可以直接创建出来虚拟机
https://learn.microsoft.com/en-us/windows-server/virtualization/hyper-v/get-started/create-a-virtual-machine-in-hyper-v?tabs=powershell
https://servermall.com/blog/microsoft-hyper-v-hypervisor-overview/?srsltid=AfmBOoqA-S-qHpDxuNh6zcEoAASh35Hi6Ms4UUOyAkWI5b7kkECKHa4Z

遇到了点问题，不知道为什么，windows 11 中用 hyper-v manger
不能按照 windows 11 虚拟机，真的奇怪啊

记住有两个错误:
1. SCSI DVD (0,0)The boot loader failed : 这个记得在屏幕前 press key ，可以安装，但是有问题
2. ... : 到时候在研究下，也是关闭安全启动吗？但是这样的话，windows 检查就无法通过了


如果安装 fedora 遇到
The signed image hash is not allow  : 关闭安全启动


既然 wsl 开源了，那么 hyper-v manger 的实现估计可以猜到吧

## 快速创建的 windows 虚拟机，远程连接上去，发现图形性能基本上无法区分了
可以进一步测试一下，是什么的原因导致的

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
