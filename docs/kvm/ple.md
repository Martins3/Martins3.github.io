# ple windo
<!-- 5368fe04-727f-4f1c-9f09-4baaa0ae3231 -->

/sys/module/kvm_intel/parameters 下有好几个关联的参数:

- ple_gap
- ple_window
- ple_window_grow
- ple_window_max
- ple_window_shrink

```c
	if (vmx->ple_window_dirty) {
		vmx->ple_window_dirty = false;
		vmcs_write32(PLE_WINDOW, vmx->ple_window);
	}
```

```c
/*
 * These 2 parameters are used to config the controls for Pause-Loop Exiting:
 * ple_gap:    upper bound on the amount of time between two successive
 *             executions of PAUSE in a loop. Also indicate if ple enabled.
 *             According to test, this time is usually smaller than 128 cycles.
 * ple_window: upper bound on the amount of time a guest is allowed to execute
 *             in a PAUSE loop. Tests indicate that most spinlocks are held for
 *             less than 2^12 cycles
 * Time is measured based on a counter that runs at the same rate as the TSC,
 * refer SDM volume 3b section 21.6.13 & 22.1.3.
 */
static unsigned int ple_gap = KVM_DEFAULT_PLE_GAP;
module_param(ple_gap, uint, 0444);
```
SDM 的
25.6.13 Controls for PAUSE-Loop Exiting

SDM table C-1 有配套的一个 exit:

PAUSE. Either guest software attempted to execute PAUSE and the “PAUSE exiting” VM-execution control was 1 or
the “PAUSE-loop exiting” VM-execution control was 1 and guest software executed a PAUSE loop with execution
time exceeding PLE_Window (see Section 26.1.3).

> [!NOTE]
> 参考神奇海螺的意见，有待验证

(差不多就是这个意思吧)

它解决的问题是：guest 在抢自旋锁时会反复执行 PAUSE，如果一直在 vCPU 上空转，会白白占着物理 CPU。
所以硬件/KVM 会盯着这种 PAUSE 自旋，一旦判断“这个 guest 已经在 pause loop
里转太久了”，就触发一次 VMEXIT，让宿主机调度别的 vCPU 先跑。

在 KVM/VMX 里：

- ple_gap：两次连续 PAUSE 之间允许的最大间隔，太大就不算“紧密自旋环”
- ple_window：允许 guest 在这个 PAUSE 自旋环里继续跑的总时间上限

这两个量的注释就在 arch/x86/kvm/vmx/vmx.c:169；时间单位不是 wall clock，而
是“和 TSC 同速的计数器周期”。 默认值在 arch/x86/kvm/x86.h:97：

- ple_gap = 128
- ple_window = 4096

KVM 里它还是会动态调节的：

- guest 因 PAUSE 触发 VMEXIT 时，KVM 会把该 vCPU 的 ple_window 往上调，见
  arch/x86/kvm/vmx/vmx.c:6035 和 arch/x86/kvm/vmx/vmx.c:1488
- vCPU 被换出后，又会把 ple_window 缩回来，见 arch/x86/kvm/vmx/vmx.c:1580

所以一句话理解：

ple window 就是“guest 在自旋锁的 pause loop 里，硬件最多再容忍它空转多久；超过这个窗口就退出到 KVM”。

补一句平台差异：

- Intel 叫 PLE_GAP / PLE_WINDOW
- AMD SVM 侧对应的是 pause_filter_count / pause_filter_thresh，本质是同类
  机制，见 arch/x86/kvm/svm/svm.c:83


## TODO
1. 检查一下了 handle_pause 调用，在 6.11 的 kernel 发现了有一些调用，
在 6.12 的 l1 中，没有调用。 是内核版本的区别，还是 l1 虚拟化，还是 kernel config 的不同导致的?

2. 如果使用了 pv spin lock ，那么这个选项还有意义吗?

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
