# QEMU 中的信号机制
<!-- 4c4de4b0-ca6a-470a-8354-dbadf0b51835 -->

1. QEMU 默认屏蔽 sigpipe 的操作
2. qemu 默认使用 signalfd
3. Host 遇到了 SIGBUS 信号，那么错误首先会注入给 Guest 一下，最终 QEMU 会因为 SIGBUS 信号而挂掉。

```txt
- qemu_init_sigbus
  - sigbus_handler
    - kvm_on_sigbus_vcpu
      - kvm_on_sigbus
        - kvm_arch_on_sigbus_vcpu
      - sigbus_reraise
```
## 不是使用 signalfd 的情况
vCPU 之间的 ipi

kvm_init_cpu_signals 中初始化这个为 signal handler ，只有这样才可以打断
掉 vCPU 的执行:

```c
static void kvm_ipi_signal(int sig)
{
    if (current_cpu) {
        assert(kvm_immediate_exit);
        kvm_cpu_kick(current_cpu);
    }
}

static void kvm_cpu_kick(CPUState *cpu)
{
    qatomic_set(&cpu->kvm_run->immediate_exit, 1);
}
```

而内核 kvm_arch_vcpu_ioctl_run 中，会
```c
		if (signal_pending(current)) {
			r = -EINTR;
			kvm_run->exit_reason = KVM_EXIT_INTR;
			++vcpu->stat.signal_exits;
		}
```

## QEMU 是如何处理 SIGPIPE 的

简而言之，QEMU 也是采用类似上面的说法的:

qemu-img 之类的工具:
```c
	signal(SIGPIPE, SIG_IGN);
```

对于 qemu 自身，调用的是
os_setup_early_signal_handling
```c
void os_setup_early_signal_handling(void)
{
    struct sigaction act;
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL);
}
```

两者有细微的区别的，应该只是理论上的差别吧。

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
