// SPDX-License-Identifier: GPL-2.0
/* Copyright Amazon.com Inc. or its affiliates. */
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>

SEC("iter/unix")
int change_sndbuf(struct bpf_iter__unix *ctx)
{
	struct unix_sock *unix_sk = ctx->unix_sk;
	int i, err;

	if (!unix_sk || !unix_sk->addr)
		return 0;

	if (unix_sk->addr->name->sun_path[0])
		return 0;

	// 也是输出到 ftrace 中的
	// TODO 但是不知道为什么，两个 d_name 输出总是为空
	bpf_printk("uds : %s", unix_sk->path.dentry->d_name);
	bpf_printk("uds : %s", unix_sk->path.mnt->mnt_root->d_name);

	return 0;
}

char _license[] SEC("license") = "GPL";
