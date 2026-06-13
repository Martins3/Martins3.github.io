# sockmap


```c
struct proto unix_dgram_proto = {
	.name			= "UNIX",
	.owner			= THIS_MODULE,
	.obj_size		= sizeof(struct unix_sock),
	.close			= unix_close,
	.bpf_bypass_getsockopt	= unix_bpf_bypass_getsockopt, // 给 SEC("cgroup/getsockopt") 用的
#ifdef CONFIG_BPF_SYSCALL
	.psock_update_sk_prot	= unix_dgram_bpf_update_proto, // 居然是给 sockmap 用的
#endif
};
```

最后走到这里的: net/unix/unix_bpf.c
```c
static void unix_dgram_bpf_rebuild_protos(struct proto *prot, const struct proto *base)
{
	*prot        = *base;
	prot->close  = sock_map_close;
	prot->recvmsg = unix_bpf_recvmsg;
	prot->sock_is_readable = sk_msg_is_readable;
}
```

## sockmap
- https://arthurchiao.art/blog/socket-acceleration-with-ebpf-zh/
  - https://github.com/ArthurChiao/socket-acceleration-with-ebpf/blob/develop/bpf/load.sh
  - 写的很好了

- [ ] https://github.com/dippynark/bpf-sockmap

- https://blog.cloudflare.com/sockmap-tcp-splicing-of-the-future

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
