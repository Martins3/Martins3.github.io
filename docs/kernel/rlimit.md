## rlimit

代码
kernel/sys.c

具体的使用:
https://www.ibm.com/docs/pl/aix/7.2?topic=u-ulimit-command

include/uapi/asm-generic/resource.h

#### 2.5 Process Resource Limits
The `rlim_max` field is the maximum allowed value for the resource limit. By using the
`getrlimit()` and `setrlimit()` system calls, a user can always increase the `rlim_cur`
limit of some resource up to `rlim_max`.
However, only the superuser (or, more precisely, a user who has the `CAP_SYS_RESOURCE` capability) can increase the `rlim_max` field
or set the `rlim_cur` field to a value greater than the corresponding `rlim_max` field.
> @todo
> 1. 既然存在 cgroup, 为什么这个东西不是被 disable 掉的 ?

Whenever a user logs into the system, the kernel creates a process owned
by the superuser, which can invoke `setrlimit()` to decrease the `rlim_max` and `rlim_cur` fields for a resource.
The same process later executes a login shell and becomes
owned by the user. Each new process created by the user inherits the content of the
rlim array from its parent, and therefore the user cannot override the limits enforced
by the administrator.
> @todo 这个描述的过程也太棒了吧! 可以找到代码上的证据吗 ?

Linux provides the resource limit (`rlimit`) mechanism to impose certain system resource usage limits on
processes. The mechanism makes use of the rlim array in `task_struct`, whose elements are of the struct
rlimit type.

## 如何配置文件的大小
```txt
 cat /proc/self/limits
Limit                     Soft Limit           Hard Limit           Units
Max cpu time              unlimited            unlimited            seconds
Max file size             unlimited            unlimited            bytes
Max data size             unlimited            unlimited            bytes
Max stack size            8388608              unlimited            bytes
Max core file size        unlimited            unlimited            bytes
Max resident set          unlimited            unlimited            bytes
Max processes             unlimited            unlimited            processes
Max open files            1024                 524288               files
Max locked memory         8388608              8388608              bytes
Max address space         unlimited            unlimited            bytes
Max file locks            unlimited            unlimited            locks
Max pending signals       513462               513462               signals
Max msgqueue size         819200               819200               bytes
Max nice priority         0                    0
Max realtime priority     0                    0
Max realtime timeout      unlimited            unlimited            us
```

```txt
 ulimit  -S  -n 10000
bash-5.1# cat /proc/self/limits
Limit                     Soft Limit           Hard Limit           Units
Max cpu time              unlimited            unlimited            seconds
Max file size             unlimited            unlimited            bytes
Max data size             unlimited            unlimited            bytes
Max stack size            8388608              unlimited            bytes
Max core file size        unlimited            unlimited            bytes
Max resident set          unlimited            unlimited            bytes
Max processes             unlimited            unlimited            processes
Max open files            10000                524288               files
Max locked memory         8388608              8388608              bytes
Max address space         unlimited            unlimited            bytes
Max file locks            unlimited            unlimited            locks
Max pending signals       513462               513462               signals
Max msgqueue size         819200               819200               bytes
Max nice priority         0                    0
Max realtime priority     0                    0
Max realtime timeout      unlimited            unlimited            us
```

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
