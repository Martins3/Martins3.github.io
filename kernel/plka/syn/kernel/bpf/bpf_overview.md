verifier.c                             1051           1739           7396
btf.c                                   656            532           3429
syscall.c                               529            177           2765
core.c                                  281            276           1672
hashtab.c                               289            164           1327
cgroup.c                                208            225           1131
arraymap.c                              173             99            815
offload.c                               129             22            561
devmap.c                                140            103            541
inode.c                                 127             32            522
bpf_lru_list.c                          131             61            502
stackmap.c                               97             53            482
bpf_struct_ops.c                        105             58            470
local_storage.c                         113             29            458
lpm_trie.c                              109            215            422
cpumap.c                                119            149            404
helpers.c                                86             21            394
trampoline.c                             56             41            329
reuseport_array.c                        60             69            231
disasm.c                                 22              7            229
xskmap.c                                 43              4            218
queue_stack_maps.c                       56             31            202
tnum.c                                   28             25            143
dispatcher.c                             28             17            113
percpu_freelist.c                        17              7             94
map_in_map.c                             19             13             88
bpf_lru_list.h                           13              7             62
sysfs_btf.c                               9              5             32
Makefile                                  1              1             30
disasm.h                                  7              4             29
percpu_freelist.h                         4              5             23
map_in_map.h                              4              3             14
bpf_struct_ops_types.h                    1              2              6


## 资源和总结
1. 为什么 package filter 和 IO, monitoring , tracing 的功能呢在一起？
2. 为什么需要在内核中间插入代码，而且必须使用llvm 的 ?

https://lwn.net/Articles/740157/  : 分析 bpf 的内核工作模式(感觉主要是 checker)

internal sandbox virtual machine

https://netflixtechblog.com/linux-performance-analysis-in-60-000-milliseconds-accc10403c55  : perf 常规知识补充


持有不少资源 : https://www.iovisor.org/technology/ebpf
https://github.com/iovisor/bcc : BCC - Tools for BPF-based Linux IO analysis, networking, monitoring
https://github.com/brendangregg : BCC 的作者还有其他的神奇的项目

https://docs.cilium.io/en/stable/bpf/ : cilium 提供的 bpf 文档

