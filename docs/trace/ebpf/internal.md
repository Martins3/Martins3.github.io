# ebpf 内部实现

## 基本的代码分析
![](https://raw.githubusercontent.com/bpftrace/bpftrace/master/images/bpftrace_internals_2018.png)

kernel/bpf/
```txt
 ./verifier.c                                         21435
 ./btf.c                                               8993
 ./syscall.c                                           6020
 ./core.c                                              3029
 ./helpers.c                                           2676
 ./hashtab.c                                           2612
 ./cgroup.c                                            2589
 ./arraymap.c                                          1380
 ./bpf_struct_ops.c                                    1198
 ./devmap.c                                            1171
 ./inode.c                                             1109
 ./trampoline.c                                        1102
 ./task_iter.c                                         1077
 ./memalloc.c                                          1012
 ./bpf_local_storage.c                                  891
 ./log.c                                                876
 ./offload.c                                            873
 ./bpf_iter.c                                           844
 ./cpumap.c                                             791
 ./ringbuf.c                                            789
 ./lpm_trie.c                                           748
 ./bpf_lru_list.c                                       700
 ./stackmap.c                                           691
 ./local_storage.c                                      614
 ./arena.c                                              569
 ./net_namespace.c                                      567
 ./cpumask.c                                            482
 ./mprog.c                                              452
 ./bpf_lsm.c                                            391
 ./bpf_task_storage.c                                   374
 ./disasm.c                                             364
 ./cgroup_iter.c                                        359
 ./reuseport_array.c                                    353
 ./tcx.c                                                346
 ./queue_stack_maps.c                                   299
 ./token.c                                              278
 ./bpf_cgrp_storage.c                                   240
 ./bpf_inode_storage.c                                  237
 ./map_iter.c                                           229
 ./bloom_filter.c                                       219
 ./tnum.c                                               213
 ./percpu_freelist.c                                    200
 ./dispatcher.c                                         170
 ./map_in_map.c                                         150
 ./preload/iterators/iterators.bpf.c                    118
 ./prog_iter.c                                          107
 ./link_iter.c                                          107
 ./preload/bpf_preload_kern.c                            92
 ./sysfs_btf.c                                           45
```

## 需要逐个分析下这个

在 include/uapi/linux/bpf.h 中
- enum bpf_cmd : 俗称 bpf syscall 中执行的各种动作
- enum bpf_map_type
- enum bpf_prog_type
- enum bpf_attach_type
- enum bpf_link_type
- enum bpf_perf_event_type


其中 bpf_prog_type 中存在如下内容:

```txt
BPF_PROG_TYPE_{KPROBE,TRACEPOINT,PERF_EVENT,RAW_TRACEPOINT}
```

的确，实际上，实现的方法完全不同:
```sh
sudo strace -fe bpf  bpftrace -e "kprobe:enqueue_to_backlog { @[kstack] = count(); }"
```
bpf(BPF_PROG_LOAD, {prog_type=BPF_PROG_TYPE_KPROBE, insn_cnt=2


```sh
sudo strace -fe bpf  bpftrace -e "kfunc:enqueue_to_backlog { @[kstack] = count(); }"
```
bpf(BPF_PROG_LOAD, {prog_type=BPF_PROG_TYPE_TRACING, insn_cnt=27


```sh
sudo strace -fe bpf bpftrace -e 'tracepoint:syscalls:sys_enter_nanosleep { printf("%s is sleeping.\n", comm); }'
```
bpf(BPF_PROG_LOAD, {prog_type=BPF_PROG_TYPE_TRACEPOINT, insn_cnt=31


```sh
sudo strace -fe bpf,ioctl,perf_event_open  bpftrace -e "kprobe:enqueue_to_backlog { @[kstack] = count(); }"
```
的输出结果如下:
```txt
bpf(BPF_PROG_LOAD, {prog_type=BPF_PROG_TYPE_KPROBE, insn_cnt=2, insns=0x7fff3d8d4580, license="GPL", log_level=0, log_size=0, log_buf=NULL, kern_version=KERNEL_VERSION(0, 0, 0), prog_flags=0, prog_name="ksys_read", prog_ifindex=0, expected_attach_type=BPF_TRACE_KPROBE_MULTI, prog_btf_fd=0, func_info_rec_size=0, func_info=NULL, func_info_cnt=0, line_info_rec_size=0, line_info=NULL, line_info_cnt=0, attach_btf_id=0, attach_prog_fd=0, fd_array=NULL}, 144) = 8
bpf(BPF_LINK_CREATE, {link_create={prog_fd=8, target_fd=0, attach_type=BPF_TRACE_KPROBE_MULTI, flags=0, kprobe_multi={flags=0, cnt=1, syms=["ksys_read"], addrs=NULL, cookies=NULL}}}, 64) = -1 EOPNOTSUPP (Operation not supported)
bpf(BPF_BTF_LOAD, {btf="\237\353\1\0\30\0\0\0\0\0\0\0L\0\0\0L\0\0\0Z\0\0\0\0\0\0\0\0\0\0\2"..., btf_log_buf=NULL, btf_size=190, btf_log_size=0, btf_log_level=0}, 32) = 9
bpf(BPF_PROG_LOAD, {prog_type=BPF_PROG_TYPE_KPROBE, insn_cnt=27, insns=0x30d2210, license="GPL", log_level=0, log_size=0, log_buf=NULL, kern_version=KERNEL_VERSION(6, 7, 1), prog_flags=0, prog_name="enqueue_to_back", prog_ifindex=0, expected_attach_type=BPF_CGROUP_INET_INGRESS, prog_btf_fd=9, func_info_rec_size=8, func_info=0x30c34e0, func_info_cnt=1, line_info_rec_size=0, line_info=NULL, line_info_cnt=0, attach_btf_id=0, attach_prog_fd=0, fd_array=NULL}, 144) = 10
perf_event_open({type=0x8 /* PERF_TYPE_??? */, size=0x88 /* PERF_ATTR_SIZE_??? */, config=0, sample_period=1, sample_type=0, read_format=0, precise_ip=0 /* arbitrary skid */, ...}, -1, 0, -1, PERF_FLAG_FD_CLOEXEC) = 8
ioctl(8, PERF_EVENT_IOC_SET_BPF, 10)    = 0
ioctl(8, PERF_EVENT_IOC_ENABLE, 0)      = 0
```
使用 perf_event_open 来实现注入 perf event


## 阅读材料
- Safe Programs The Foundation of BPF - Alexei Starovoitov, Facebook - Full Keynote :  https://www.youtube.com/watch?v=AV8xY318rtc
- [ ] https://www.zerodayinitiative.com/blog/2020/4/8/cve-2020-8835-linux-kernel-privilege-escalation-via-improper-ebpf-program-verification
- [ ] https://twitter.com/shunghsiyu/status/1553592644219318272?s=20&t=Tn6g5qhu7pNHCr0-DnKVng

## 如何构建 stackmap
- [Capturing stack traces asynchronously with BPF](https://mp.weixin.qq.com/s/zp_OirD1bpEwxBzPHhfCiw)
  - 分析 stack 是如何构建的，关注 kernel/bpf/stackmap.c 这个文件

忽然意识到，只有 bpf 的工具才可以构建出来 stackcount 的功能

## 有趣，看来 verifier 还是很厉害的
```c
SEC("ksyscall/tgkill")
int BPF_KSYSCALL(tgkill_entry, pid_t tgid, pid_t tid, int sig)
{
        char comm[1];
        __u32 caller_pid = bpf_get_current_pid_tgid() >> 32;

        if (sig == 0) {
                /*
                        If sig is 0, then no signal is sent, but existence and permission
                        checks are still performed; this can be used to check for the
                        existence of a process ID or process group ID that the caller is
                        permitted to signal.
                */
                return 0;
        }

        bpf_get_current_comm(&comm, 100);
        bpf_printk(
                "tgkill syscall called by PID %d (%s) for thread id %d with pid %d and signal %d.",
                caller_pid, comm, tid, tgid, sig);
        return 0;
}
```

加载的过程中可以遇到下面的错误:
```txt
0: R1=ctx() R10=fp0
; int BPF_KSYSCALL(tgkill_entry, pid_t tgid, pid_t tid, int sig) @ ksyscall.bpf.c:9
0: (18) r2 = 0xffffc90000514000       ; R2_w=map_value(map=ksyscal.kconfig,ks=4,vs=1)
2: (71) r2 = *(u8 *)(r2 +0)           ; R2_w=1
3: (15) if r2 == 0x0 goto pc+45       ; R2_w=1
4: (b7) r2 = 112                      ; R2_w=112
5: (79) r6 = *(u64 *)(r1 +112)        ; R1=ctx() R6_w=scalar()
6: (bf) r3 = r6                       ; R3_w=scalar(id=1) R6_w=scalar(id=1)
7: (0f) r3 += r2                      ; R2_w=112 R3_w=scalar()
8: (bf) r1 = r10                      ; R1_w=fp0 R10=fp0
9: (07) r1 += -48                     ; R1_w=fp-48
10: (b7) r2 = 8                       ; R2_w=8
11: (85) call bpf_probe_read_kernel#113       ; R0=scalar() fp-48=mmmmmmmm
12: (b7) r1 = 104                     ; R1_w=104
13: (bf) r3 = r6                      ; R3_w=scalar(id=1) R6=scalar(id=1)
14: (0f) r3 += r1                     ; R1_w=104 R3_w=scalar()
15: (79) r8 = *(u64 *)(r10 -48)       ; R8_w=scalar() R10=fp0 fp-48=mmmmmmmm
16: (bf) r1 = r10                     ; R1_w=fp0 R10=fp0
17: (07) r1 += -48                    ; R1_w=fp-48
18: (b7) r2 = 8                       ; R2_w=8
19: (85) call bpf_probe_read_kernel#113       ; R0_w=scalar() fp-48=mmmmmmmm
20: (b7) r1 = 96                      ; R1_w=96
21: (0f) r6 += r1                     ; R1_w=96 R6_w=scalar()
22: (79) r9 = *(u64 *)(r10 -48)       ; R9_w=scalar() R10=fp0 fp-48=mmmmmmmm
23: (bf) r1 = r10                     ; R1_w=fp0 R10=fp0
24: (07) r1 += -48                    ; R1_w=fp-48
25: (b7) r2 = 8                       ; R2_w=8
26: (bf) r3 = r6                      ; R3_w=scalar(id=2) R6_w=scalar(id=2)
27: (85) call bpf_probe_read_kernel#113       ; R0=scalar() fp-48=mmmmmmmm
28: (79) r7 = *(u64 *)(r10 -48)       ; R7_w=scalar() R10=fp0 fp-48=mmmmmmmm
; __u32 caller_pid = bpf_get_current_pid_tgid() >> 32; @ ksyscall.bpf.c:12
29: (85) call bpf_get_current_pid_tgid#14     ; R0_w=scalar()
30: (bf) r6 = r0                      ; R0_w=scalar(id=3) R6_w=scalar(id=3)
; int BPF_KSYSCALL(tgkill_entry, pid_t tgid, pid_t tid, int sig) @ ksyscall.bpf.c:9
31: (67) r7 <<= 32                    ; R7_w=scalar(smax=0x7fffffff00000000,umax=0xffffffff00000000,smin32=0,smax32=umax32=0,var_off=(0x0; 0xffffffff00000000))
32: (bf) r1 = r7                      ; R1_w=scalar(id=4,smax=0x7fffffff00000000,umax=0xffffffff00000000,smin32=0,smax32=umax32=0,var_off=(0x0; 0xffffffff00000000)) R7_w=scalar(id=4,smax=0x7fffffff00000000,umax=0xffffffff00000000,smin32=0,smax32=umax32=0,var_off=(0x0; 0xffffffff00000000))
33: (77) r1 >>= 32                    ; R1_w=scalar(smin=0,smax=umax=0xffffffff,var_off=(0x0; 0xffffffff))
; if (sig == 0) { @ ksyscall.bpf.c:14
34: (15) if r1 == 0x0 goto pc+47      ; R1_w=scalar(smin=umin=umin32=1,smax=umax=0xffffffff,var_off=(0x0; 0xffffffff))
35: (bf) r1 = r10                     ; R1_w=fp0 R10=fp0
;  @ ksyscall.bpf.c:0
36: (07) r1 += -1                     ; R1_w=fp-1
; bpf_get_current_comm(&comm, 100); @ ksyscall.bpf.c:24
37: (7b) *(u64 *)(r10 -56) = r1       ; R1_w=fp-1 R10=fp0 fp-56_w=fp-1
38: (b7) r2 = 100                     ; R2_w=100
39: (85) call bpf_get_current_comm#16
invalid indirect access to stack R1 off=-1 size=100
processed 39 insns (limit 1000000) max_states_per_insn 0 total_states 2 peak_states 2 mark_read 1
-- END PROG LOAD LOG --
libbpf: prog 'tgkill_entry': failed to load: -22
libbpf: failed to load object 'ksyscall_bpf'
libbpf: failed to load BPF skeleton 'ksyscall_bpf': -22
Failed to open BPF skeleton
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
