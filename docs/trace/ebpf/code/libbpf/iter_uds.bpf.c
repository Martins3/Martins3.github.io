// SPDX-License-Identifier: GPL-2.0
/* Copyright Amazon.com Inc. or its affiliates. */
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>

SEC("iter/unix")
int dump_unix(struct bpf_iter__unix *ctx)
{
	struct unix_sock *unix_sk = ctx->unix_sk;
	struct seq_file *seq;

	if (!unix_sk)
		return 0;

	seq = ctx->meta->seq;
	if (ctx->meta->seq_num == 0)
		BPF_SEQ_PRINTF(seq, "UID      Path\n");

	BPF_SEQ_PRINTF(seq, "%-8u ", ctx->uid);
	if (unix_sk->addr) {
		if (unix_sk->addr->name->sun_path[0]) {
			BPF_SEQ_PRINTF(seq, "%s", unix_sk->addr->name->sun_path);
		} else {
			__u64 i;
			__u64 len = unix_sk->addr->len - sizeof(short);

			BPF_SEQ_PRINTF(seq, "@");
			for (i = 1; i < len; i++) {
				if (i >= sizeof(struct sockaddr_un))
					break;
				BPF_SEQ_PRINTF(seq, "%c",
					       unix_sk->addr->name->sun_path[i] ?:
					       '@');
			}
		}
	}
	BPF_SEQ_PRINTF(seq, "\n");

	return 0;
}

char _license[] SEC("license") = "GPL";
