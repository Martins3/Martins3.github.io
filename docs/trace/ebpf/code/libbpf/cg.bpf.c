/* kernel source tree : samples/bpf/sock_flags.bpf.c */
// SPDX-License-Identifier: GPL-2.0
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#ifdef TEST
#include "net_shared.h"
#endif

char _license[] SEC("license") = "GPL";

#define AF_INET 2
#define AF_INET6 10

// TODO 不知道为什么，如果将四个程序都放上去，加载的时候会 segfault
#ifdef TEST
SEC("cgroup/sock")
int bpf_prog1(struct bpf_sock *sk)
{
	char fmt[] = "socket: family %d type %d protocol %d";
	char fmt2[] = "socket: uid %u gid %u\n";
	__u64 gid_uid = bpf_get_current_uid_gid();
	__u32 uid = gid_uid & 0xffffffff;
	__u32 gid = gid_uid >> 32;

	bpf_trace_printk(fmt, sizeof(fmt), sk->family, sk->type, sk->protocol);
	bpf_trace_printk(fmt2, sizeof(fmt2), uid, gid);

	/* block AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6 sockets
	 * ie., make ping6 fail
	 */
	if (sk->family == AF_INET6 && sk->type == SOCK_DGRAM &&
	    sk->protocol == IPPROTO_ICMPV6)
		return 0;

	return 1;
}

SEC("cgroup/sock")
int bpf_prog2(struct bpf_sock *sk)
{
	char fmt[] = "socket: family %d type %d protocol %d";

	bpf_trace_printk(fmt, sizeof(fmt), sk->family, sk->type, sk->protocol);

	/* block AF_INET, SOCK_DGRAM, IPPROTO_ICMP sockets
	 * ie., make ping fail
	 */
	if (sk->family == AF_INET && sk->type == SOCK_DGRAM &&
	    sk->protocol == IPPROTO_ICMP)
		return 0;

	return 1;
}
#endif

/**
 * code/module/c/udp_server.c 中测试，这个代码是无法 trigger 的，
 * 这是由于 cgroup_bpf_sock_enabled ，只有这个 socket 关联了 bpf 之后才可以，
 * 执行 bpf 才可以。
 *
 * 原来 cgroup 的含义是这个意思啊，可以
 */
SEC("cgroup/getsockopt")
int getsockopt(struct bpf_sockopt *ctx)
{
	char comm[TASK_COMM_LEN];
	bpf_get_current_comm(&comm, sizeof(comm));

	bpf_printk("cgroup/getsockopt by %s.", comm);
	return 1;
}

SEC("cgroup/setsockopt")
int setsockopt(struct bpf_sockopt *ctx)
{
	char comm[TASK_COMM_LEN];
	bpf_get_current_comm(&comm, sizeof(comm));

	bpf_printk("cgroup/setsockopt by %s.", comm);
	return 1;
}
