# Memory Tracepoints
<!-- 5640bcbc-7daf-420c-87c8-34fe3a7f9cb9 -->

通过这个观察和
1. sudo perf trace -e exceptions:page_fault_kernel

```txt
sudo perf trace -e exceptions:page_fault_kernel

 12458.417 exceptions:page_fault_kernel:address=0x7fe0c0112803s ip=strncpy_from_user error_code=0x0
 12458.543 exceptions:page_fault_kernel:address=0x7fe0c00b0490s ip=strncpy_from_user error_code=0x0
 12459.853 exceptions:page_fault_kernel:address=0x7fe0c012502fs ip=strncpy_from_user error_code=0x0
 12497.294 exceptions:page_fault_kernel:address=0x7ffca5714ec0s ip=x64_setup_rt_frame error_code=0x3
```

```txt
sudo bpftrace -e "tracepoint:exceptions:page_fault_kernel { @[kstack] = count(); }"

@[
    exc_page_fault+315
    exc_page_fault+315
    asm_exc_page_fault+38
    _copy_to_iter+134
    tty_read+228
    vfs_read+662
    ksys_read+111
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 1
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
