# wine : 如何实现系统调用虚拟化

![](./img/wine.webp)

wine 的原理描述起来很简单:
1. 如果是普通指令，直接执行。
2. 如果是 syscall，那么使用 Linux 的 syscall 来模拟 windows 的 syscall 。

但是事情比要比想象的复杂的多，wine 的代码居然接近 500 万行。
```txt
 tokei
===============================================================================
 Language            Files        Lines         Code     Comments       Blanks
===============================================================================
 C                    3638      4929041      3913305       369566       646170
 C Header             1416       501239       391755        57036        52448
```

这里有很多原因，但是绝对不是 wine 的开发人员水平太差的原因，毕竟 WSL 1 就是翻译 Linux 程序，但是最后不了了之。

是 Windows 的系统调用和 Linux 的不同的规则不同，Linux 内核和用户态的库是两波人在维护，所以 Linux 对外必须提供一个稳定的接口，而 windows 的系统调用
则可以随意变动，因为其用户态的库和内核都是在 Microsoft 的控制下，只要库对于开发人员是稳定的即可。


## 尝试一个小 demo 吧

## 虚拟化技术的粗率总结
1. 硬件辅助虚拟化，代表为 vt-x + kvm，可以实现 Windows 操作系统在 Linux 中运行。
2. 二进制翻译，代表是 QEMU ，可以实现 ARM 程序在 x86 上运行。
3. V8 等语言虚拟机，可以让 js 代码在任何支持 v8 的平台运行（当然 v8 的平台支持非常麻烦）；
4. 容器，实现隔离，可以让
5. [cosmopolitan](https://github.com/jart/cosmopolitan) 之类的跨平台的库。
6. virtual function / interface 等编译语言技术，让实现该接口的 class 都可以调用。

暂时就想到这么多了，更详细的内容参考可以 @xieby1 的[总结](https://github.com/xieby1/runXonY) 。

## 相关资料
- [How the Windows Subsystem for Linux Redirects Syscalls](https://news.ycombinator.com/item?id=11864211)
  - Microsoft 介绍 wsl 的原理
- [How Wine works 101](https://news.ycombinator.com/item?id=33156727)
- [windows-syscall](https://github.com/j00ru/windows-syscalls)

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
