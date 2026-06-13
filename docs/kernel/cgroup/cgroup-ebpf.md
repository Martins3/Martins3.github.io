# cgroup ebpf

这个做啥的?
```c
enum cgroup_bpf_attach_type {
	CGROUP_BPF_ATTACH_TYPE_INVALID = -1,
	CGROUP_INET_INGRESS = 0,
	CGROUP_INET_EGRESS,
	CGROUP_INET_SOCK_CREATE,
	CGROUP_SOCK_OPS,
	CGROUP_DEVICE,
	CGROUP_INET4_BIND,
	CGROUP_INET6_BIND,
	CGROUP_INET4_CONNECT,
	CGROUP_INET6_CONNECT,
	CGROUP_UNIX_CONNECT,
	CGROUP_INET4_POST_BIND,
	CGROUP_INET6_POST_BIND,
	CGROUP_UDP4_SENDMSG,
	CGROUP_UDP6_SENDMSG,
	CGROUP_UNIX_SENDMSG,
	CGROUP_SYSCTL,
	CGROUP_UDP4_RECVMSG,
	CGROUP_UDP6_RECVMSG,
	CGROUP_UNIX_RECVMSG,
	CGROUP_GETSOCKOPT,
	CGROUP_SETSOCKOPT,
	CGROUP_INET4_GETPEERNAME,
	CGROUP_INET6_GETPEERNAME,
	CGROUP_UNIX_GETPEERNAME,
	CGROUP_INET4_GETSOCKNAME,
	CGROUP_INET6_GETSOCKNAME,
	CGROUP_UNIX_GETSOCKNAME,
	CGROUP_INET_SOCK_RELEASE,
	CGROUP_LSM_START,
	CGROUP_LSM_END = CGROUP_LSM_START + CGROUP_LSM_NUM - 1,
	MAX_CGROUP_BPF_ATTACH_TYPE
};
```

BPF_PROG_TYPE_CGROUP_SOCK

```c
#define BPF_CGROUP_RUN_PROG_GETSOCKOPT(sock, level, optname, optval, optlen,   \
				       max_optlen, retval)		       \
({									       \
	int __ret = retval;						       \
	if (cgroup_bpf_enabled(CGROUP_GETSOCKOPT) &&			       \
	    cgroup_bpf_sock_enabled(sock, CGROUP_GETSOCKOPT))		       \
		if (!(sock)->sk_prot->bpf_bypass_getsockopt ||		       \
		    !INDIRECT_CALL_INET_1((sock)->sk_prot->bpf_bypass_getsockopt, \
					tcp_bpf_bypass_getsockopt,	       \
					level, optname))		       \
			__ret = __cgroup_bpf_run_filter_getsockopt(	       \
				sock, level, optname, optval, optlen,	       \
				max_optlen, retval);			       \
	__ret;								       \
})
```

do_sock_getsockopt 中有一段，内容如下：
```c
	if (!compat)
		err = BPF_CGROUP_RUN_PROG_GETSOCKOPT(sock->sk, level, optname,
						     optval, optlen, max_optlen,
						     err);
```

看看这里吧:
https://patchwork.ozlabs.org/project/netdev/cover/20190610210830.105694-1-sdf@google.com/

- 这和 cgroup 有啥关系 ?
- 而且为什么要截获这个东西?

- Documentation/bpf/prog_cgroup_sockopt.rst : 测试这个文档

## 做做基本测试

cgroup_bpf_sock_enabled 什么时候判断不为空?

似乎 cgroup 的含义我们理解错了，可以直接对于一个 cgroup 中的所有的程序配置
samples/bpf/test_cgrp2_sock.c

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
