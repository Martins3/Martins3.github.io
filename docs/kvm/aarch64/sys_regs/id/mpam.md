## aarch64 mpam
<!-- 09ae7eb8-f063-44fe-ad96-9a167525a425 -->

commit 09e6b306f3ba ("arm64: cpufeature: discover CPU support for MPAM")

```txt
commit 09e6b306f3bad803a9743e40da6a644d66d19928
Author: James Morse <james.morse@arm.com>
Date:   Wed Oct 30 16:03:13 2024 +0000

    arm64: cpufeature: discover CPU support for MPAM

    ARMv8.4 adds support for 'Memory Partitioning And Monitoring' (MPAM)
    which describes an interface to cache and bandwidth controls wherever
    they appear in the system.

    Add support to detect MPAM. Like SVE, MPAM has an extra id register that
    describes some more properties, including the virtualisation support,
    which is optional. Detect this separately so we can detect
    mismatched/insane systems, but still use MPAM on the host even if the
    virtualisation support is missing.

    MPAM needs enabling at the highest implemented exception level, otherwise
    the register accesses trap. The 'enabled' flag is accessible to lower
    exception levels, but its in a register that traps when MPAM isn't enabled.
    The cpufeature 'matches' hook is extended to test this on one of the
    CPUs, so that firmware can emulate MPAM as disabled if it is reserved
    for use by secure world.

    Secondary CPUs that appear late could trip cpufeature's 'lower safe'
    behaviour after the MPAM properties have been advertised to user-space.
    Add a verify call to ensure late secondaries match the existing CPUs.

    (If you have a boot failure that bisects here its likely your CPUs
    advertise MPAM in the id registers, but firmware failed to either enable
    or MPAM, or emulate the trap as if it were disabled)
```

这是一个非常有意思的特性，通过这个来控制，
- https://aijishu.com/a/1060000000393432
- https://mp.weixin.qq.com/s/_GQpRwYmEjiO2zed2p_e9Q
	- Android 使用 MPAM 来协调 CPU 和 system level cache
	- 本文介绍一下高通的Yiwei Huang在LPC 2025上面分享的一个主题

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
