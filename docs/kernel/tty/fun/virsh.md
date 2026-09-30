# virsh console 和 virsh consoletty 什么关系


console 的原文 (https://libvirt.org/manpages/virsh.html#console)：

> Connect the virtual serial console for the guest.

即：连接 guest 的虚拟串口控制台，不指定设备时连接主控制台。

ttyconsole 的原文 (https://libvirt.org/manpages/virsh.html#ttyconsole)：

> Output the device used for the TTY console of the domain.

即：输出虚拟机 TTY 控制台所使用的设备。手册还说明：信息不可用时，退出码为 1。

按手册定义，console 建立连接，ttyconsole 报告设备信息。

这个说的也是莫名其妙，不知道他想要表达什么，
其实没有那么复杂，就是 **ttyconsole 打印一个字符串就结束；console 持续运行，转发你与虚拟机之间的字符。**

利用 libvirt 启动，在 x86 上:

```txt
  -chardev pty,id=charserial0
  -device isa-serial,chardev=charserial0,id=serial0

  -chardev file,id=charserial1,path=/dev/fdset/33,append=on
  -device isa-serial,chardev=charserial1,id=serial1
```
第一个的结果是:
```txt
guest /dev/ttyS0 ↔ 虚拟 UART ↔ 宿主机 /dev/pts/1
```
第二个是 ttyS1 的输出写入到 libvirt 的文件中:

如果在 arm 上，结果为:

```txt
  -chardev pty,id=charserial0
  -serial chardev:charserial0
```
很遗憾，arm 只有一个串口。

所以，virsh console 和 virsh consoletty 都是在使用一个 /dev/ttyS0 或者 /dev/ttyAMA0 而已，只是一个可以交互，一个不行。


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
