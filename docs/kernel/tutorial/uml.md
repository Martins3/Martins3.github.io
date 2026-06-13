# uml
## 首先按照这个操作
- https://hackmd.io/@sysprog/user-mode-linux-env
- https://user-mode-linux.sourceforge.net/

```sh
	# 不知道为什么，无法使用 llvm ，但是也不想调查了
make menuconfig ARCH=um
make ARCH=um
```
需要额外带上这些 config 来添加网卡:
```txt
CONFIG_MAY_HAVE_RUNTIME_DEPS=m
# CONFIG_UML_NET_ETHERTAP is not set
# CONFIG_UML_NET_TUNTAP is not set
# CONFIG_UML_NET_SLIP is not set
# CONFIG_UML_NET_DAEMON is not set
CONFIG_UML_NET_VECTOR=m
# CONFIG_UML_NET_MCAST is not set
# CONFIG_UML_NET_SLIRP is not set
```

# user mode linux

- https://docs.kernel.org/virt/uml/user_mode_linux_howto_v2.html
- [ ] https://stackoverflow.com/questions/32303095/how-does-the-user-mode-kernel-in-uml-interface-with-the-underlying-kernel-on-the


## 首先让我猜测一下是如何工作的

- 对于设备存在的模拟
- 内存的模拟。
  - 模拟出来多个地址空间出来。
- CPU 的模拟
  - 使用线程进行模拟的吧!
  - context swtich 也是特殊的才对。
- 每一个进程都是都是直接使用 sysenter 指令的，这些 sysenter 必然是首先进入到 host linux 中
  - 对于用户进程使用 ptrace
- 信号机制
  - 如果 uml 中的程序触发了一个 page fault ，这个信号是需要经过转发的，不然就是 uml 本身的 thread 直接挂掉了
- 特权指令的使用
  - 例如 wbinvd ，或者 flush remote tlb 的工作，应该是通过 um arch 特定的函数封装了，导致最后无需特殊指令的！

## 有趣

- https://xeiaso.net/blog/howto-usermode-linux-2019-07-07/
  - https://news.ycombinator.com/item?id=20379063


## 问题
1. 如何使用 vhost-user 来着 ?
  - 原理上不难理解，因为本来就是用户态，使用 vhost-user 来共享，所以 guest os 从原则上来说

运行的时候存在这个报错
```txt
epollctl add err fd 15, Operation not permitted
epollctl add err fd 15, Operation not permitted
epollctl add err fd 17, Operation not permitted
epollctl add err fd 17, Operation not permitted
epollctl add err fd 18, Operation not permitted
epollctl add err fd 18, Operation not permitted
winch_thread : TIOCSCTTY failed on fd 1 err = 1
                                               epollctl add err fd 19, Operation not permitted
epollctl add err fd 19, Operation not permitted
epollctl add err fd 21, Operation not permitted
epollctl add err fd 21, Operation not permitted
```
3. uml_mconsole 工具没找到
4. 为什么无法处理多线程的问题? 可能是遇到了什么问题?
   - 如果是 single thread 的，那么 core/sched 中的代码还会使用吗？应该不会用吧
5. 在 uml 中如何进行地址空间的共享 ?
  - 例如，一个程序使用了 io uring ，如何实现两个
6. https://docs.kernel.org/virt/uml/user_mode_linux_howto_v2.html 中分析了很多网络，例如 GRO

7. 为什么进入到系统中，root 分区默认是只读的，是因为没有配置 /etc/fstab 吗?

8. 测试 io uring 的效果

9. 如何实现地址空间的保护的?

### 为什么要实现一个 linux-uml/arch/um/drivers/null.c
- 似乎这会导致一个很奇怪的场景，这里直接打开了 host 的 /dev/null 传递进去的

## 如果继续
参考 https://docs.kernel.org/virt/uml/user_mode_linux_howto_v2.html ，解决 tap 设备的 nat 的问题，不然无法在镜像中下载各种工具
无法方便的 ssh ，无法更加方便的做测试。

看看这个，[How to user mode linux](https://christine.website/blog/howto-usermode-linux-2019-07-07)
也许有点过时了

## 为什么 um 还支持 vdso ？

## uml 和 rust 也是支持的，可怕
https://pen.guru/posts/rust-uml-relocation-error/

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
