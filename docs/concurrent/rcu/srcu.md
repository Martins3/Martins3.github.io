## srcu

- blk_mq_run_dispatch_ops 总是在区分 srcu 和 rcu

介绍的简单易懂:
https://liujunming.top/2023/08/06/Linux-kernel-SRCU-usage/


## SRCU

e.g., `kvm_mmu_notifier_invalidate_range_start`

sleepable rcu

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
