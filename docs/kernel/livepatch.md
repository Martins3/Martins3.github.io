# livepatch

## 理解下 livepatch 的实现原理
https://linux-audit.com/livepatch-linux-kernel-updates-without-rebooting/

- [ ] 是不是只能 livepatch 模块?
	- [ ] 让内核模块像 rcu 一样热替换？

- kernel/livepatch/ 关联的源码
- scripts/livepatch/ : 构建 livepatch 的脚本

## 一般的 c/c++ 的 livepatch
https://news.ycombinator.com/item?id=38856696

2025 的会上又继续在分析了:
https://lpc.events/event/19/sessions/231/

## 用户态程序的 restart
应该是很多了:
https://github.com/cloudflare/tableflip

vhost 如何处理重启的

k8s 的重启

## 如果做不到整个内核的重启，可以有简单一点的方法吗?

多个 io scheduler ，可以运行时切换

## 看看
linux kpatch 内核态热补丁 - happyking的文章 - 知乎
https://zhuanlan.zhihu.com/p/1913973625599550764

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
