https://mp.weixin.qq.com/s/3hjLlB8Qr2FQWoJbP0OE4w

https://fosdem.org/2025/schedule/event/fosdem-2025-5471-ngnfs-a-distributed-file-system-using-block-granular-consistency/

不明觉厉

__insert_inode_hash 不知道为什么也一般调用不到

get_next_ino 基本都是给 vfs 使用的
```txt
@[
        get_next_ino+5
        proc_pid_make_inode+41
        proc_pid_make_base_inode.constprop.0+19
        proc_pid_instantiate+28
        proc_pid_lookup+135
        proc_root_lookup+31
        __lookup_slow+133
        walk_component+219
        link_path_walk.part.0.constprop.0+456
        path_openat+157
        do_filp_open+215
        do_sys_openat2+138
        __x64_sys_openat+84
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 29
```

- ilookup 和 ilookup5 的调用不容易触发

mount 在文件查询的过程 : follow_up 和 follow_down

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
