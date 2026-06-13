#include "vmlinux.h"
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "ext4.h"

char LICENSE[] SEC("license") = "GPL";

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, struct fname_key);
    __type(value, __u64);
} file_count SEC(".maps");

// fentry: 监控 filemap_fault
SEC("fentry/filemap_fault")
int BPF_PROG(fentry_filemap_fault, struct vm_fault *vmf)
{
    struct file *file;
    struct dentry *dentry;
    struct fname_key key = {};
    __u64 initval = 1, *count;

    if (!vmf || !vmf->vma)
        return 0;

    file = vmf->vma->vm_file;
    if (!file)
        return 0;

    dentry = file->f_path.dentry;
    if (!dentry)
        return 0;

    // 从 dentry->d_name.name 读取文件名
    if (bpf_core_read_str(&key.name, sizeof(key.name), dentry->d_name.name) < 0)
        return 0;

    // 更新计数
    count = bpf_map_lookup_elem(&file_count, &key);
    if (count)
        __sync_fetch_and_add(count, 1);
    else
        bpf_map_update_elem(&file_count, &key, &initval, BPF_NOEXIST);

    return 0;
}
