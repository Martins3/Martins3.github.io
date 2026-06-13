# aperfmperf
<!-- da7cfef2-9c73-4954-af6f-3d79fffd29b9 -->

```c
#define X86_FEATURE_APERFMPERF		( 3*32+28) /* P-State hardware coordination feedback capability (APERF/MPERF MSRs) */
```

/proc/cpuinfo vs /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq
中获取频率的方法完全是 完全相同的，最后都是走到 arch/x86/kernel/cpu/aperfmperf.c 中的
```txt
@[
        arch_freq_get_on_cpu+5
        show_scaling_cur_freq+30
        show+106
        sysfs_kf_seq_show+206
        seq_read_iter+295
        vfs_read+612
        ksys_read+115
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 32
```

## aperfmperf 是无法透传给 vCPU 的
从 guest 中获取的 /proc/cpuinfo 完全是错的
```txt
➜  ~ cat /proc/cpuinfo | grep MHz
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
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
