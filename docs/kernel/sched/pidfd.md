# pidfd
## 解决的问题
1. pid 可能出现回绕，应该极其难以触发吧 (1024 core 的机器不断的触发)
2. 使用 fd 可以使用 epoll 了

其实这里最前面的几个问题都可以解决:
https://lwn.net/Kernel/Index/#pidfd
- [Completing the pidfd API](https://lwn.net/Articles/794707/)
- [One more pidfdfs surprise](https://mp.weixin.qq.com/s/t-w5jLtqod0FeUqsn8rtng)

## pidfd
- https://man7.org/linux/man-pages/man2/pidfd_open.2.html
- https://mp.weixin.qq.com/s/lFPOHxcDhQgkYVliw2ZbWg
- https://www.corsix.org/content/what-is-a-pidfd

## pidfs
https://lwn.net/Articles/963749/ : 其实，就是 pidfd 原来的机制不能满足了

## 原来现在的 clone args 都是有 pidfd 的参数
```c
struct kernel_clone_args {
	u64 flags;
	int __user *pidfd;
	int __user *child_tid;
	int __user *parent_tid;
	const char *name;
	int exit_signal;
	u32 kthread:1;
	u32 io_thread:1;
	u32 user_worker:1;
	u32 no_files:1;
	unsigned long stack;
	unsigned long stack_size;
	unsigned long tls;
	pid_t *set_tid;
	/* Number of elements in *set_tid */
	size_t set_tid_size;
	int cgroup;
	int idle;
	int (*fn)(void *);
	void *fn_arg;
	struct cgroup *cgrp;
	struct css_set *cset;
};
```

原来 pidfd 可以通过这个展示，使用 sched/pidfd2.c 来测试
```txt
/proc/271669/fdinfo🔒 🔥
🧀  cat 3
pos:    0
flags:  02000002
mnt_id: 4
ino:    271109
Pid:    270809
NSpid:  270809
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
