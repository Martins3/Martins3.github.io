## debugfs

global
```txt
369544-17
371710-19
377880-19
blocking
exits
hvc_exit_stat
mmio_exit_kernel
mmio_exit_user
remote_tlb_flush
remote_tlb_flush_requests
signal_exits
wfe_exit_stat
wfi_exit_stat
```

per vm
```txt
blocking # 多少个 vcpu 进入到 block 的状态
exits
hvc_exit_stat
mmio_exit_kernel
mmio_exit_user
remote_tlb_flush
remote_tlb_flush_requests
signal_exits
wfe_exit_stat
wfi_exit_stat


idregs
vgic-its-state@8080000
vgic-state
```
也就是额外的多出来了


```txt
idregs
vgic-its-state@8080000
vgic-state
```

## 基本分析

```txt
mmio_exit_kernel:25080
mmio_exit_user:175246
remote_tlb_flush:0
remote_tlb_flush_requests:0
signal_exits:10607
```

```c
struct kvm_vcpu_stat {
	struct kvm_vcpu_stat_generic generic;
	u64 hvc_exit_stat;
	u64 wfe_exit_stat;
	u64 wfi_exit_stat;
	u64 mmio_exit_user;
	u64 mmio_exit_kernel;
	u64 signal_exits;
	u64 exits;
};
```

remote_tlb_flush 居然是 0 ，有一点，似乎也没有实现
pvtlbflush ?

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
