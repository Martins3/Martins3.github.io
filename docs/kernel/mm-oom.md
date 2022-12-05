# oom

核心结构体 `oom_control`，主要记录事发现场。

主要的入口为 `out_of_memory`，调用着有三个
- `mmecontrol:mem_cgroup_out_of_memory` : 用户程序分配内存的时候，经过 cgroup 的检查 `mem_cgroup_charge` 没有通过。
- `page_alloc.c:__alloc_pages_may_oom` : 在此处失败，是因为不受 cgroup 管理的用户进程分配内存失败。
- `sysrq:moom_callback` : 通过  sudo echo f > /proc/sysrq-trigger 手动触发

1. 为什么 oom 会因为 cpuset ？
```c
struct oom_control {
	/* Used to determine cpuset */
	struct zonelist *zonelist;
```
因为该进程运行执行的 node 上没有内存了。

2. reaper 是做啥的?

在 `oom_kill_process` 中，将那些**已经被杀死**进程持有的内存直接释放掉[^1]。

## TODO
- https://github.com/rfjakob/earlyoom : 了解这个工具的原理
- https://superuser.com/questions/1150215/disabling-oom-killer-on-ubuntu-14-04

## 其他参考
- http://linux.laoqinren.net/linux/out-of-memory/

[^1]: https://lwn.net/Articles/666024/
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
