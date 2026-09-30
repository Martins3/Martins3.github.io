# 不如直接用 tsc

## 为什么 kvmclock 在 firecracker 是默认打开的
因为 QEMU 默认开启 kvmclock，但通常不向 guest 暴露 invtsc，Linux 就不会把 kvm-clock 的优先级降到 TSC 以下。 我之前说“稳定时”不够精确：
判断依据是 guest 可见的 CPU 特性和 guest 内核状态。

源码证据可以连起来看：

1. QEMU 默认提供 kvmclock。

   本地 /home/martins3/data/qemu/target/i386/kvm/kvm-cpu.c:175：

   static PropValue kvm_default_props[] = {
       { "kvmclock", "on" },

   这表示提供该功能，最终使用哪个 clocksource 由 guest Linux 选择。

2. QEMU 默认的可迁移特性过滤会排除 invtsc。

   QEMU 把 CPUID.80000007H:EDX[8] 命名为 invtsc，并标记为：

   .unmigratable_flags = CPUID_APM_INVTSC,

   host/max CPU 的 migratable 默认开启，自动获取宿主机特性时会进行过滤。因此，即使使用 -cpu host，也不能直接认为 guest 拥有
   invtsc。显式设置 tsc-khz 有相应例外。QEMU v9.2 源码 (https://github.com/qemu/qemu/blob/v9.2.0/target/i386/cpu.c#L1842-L1863)

3. Linux 只有满足以下条件，才降低 kvm-clock 的评分。

   Linux v5.10：kvmclock_init() (https://github.com/torvalds/linux/blob/v5.10/arch/x86/kernel/kvmclock.c#L362-L375)：

   if (boot_cpu_has(X86_FEATURE_CONSTANT_TSC) &&
       boot_cpu_has(X86_FEATURE_NONSTOP_TSC) &&
       !check_tsc_unstable())
       kvm_clock.rating = 299;

   kvm-clock 初始评分为 400；tsc 的评分为 300
   (https://github.com/torvalds/linux/blob/v5.10/arch/x86/kernel/tsc.c#L1147-L1161)。没有显式覆盖时，clocksource
   核心选择评分最高的可用源 (https://github.com/torvalds/linux/blob/v5.10/kernel/time/clocksource.c#L710-L728)：

    Guest 条件                kvm-clock 评分         tsc 评分    默认结果
   ━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━  ━━━━━━━━━━━
    缺少上述 TSC 特性                    400    300（若可用）    kvm-clock
   ────────────────────────  ────────────────  ───────────────  ───────────
    满足上述条件，TSC 可用               299              300    tsc

另外纠正版本边界：这项 Linux 策略从主线 v5.1 就存在，并非始于 v5.10。 我在本地 Git 历史中定位到提交 7539b174aef4
(https://github.com/torvalds/linux/commit/7539b174aef405d9d57db48c58390ba360c91312)，首次包含于 v5.1-rc1。Firecracker 文档的“≥
5.10”不能作为该机制首次引入的版本。


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
