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
