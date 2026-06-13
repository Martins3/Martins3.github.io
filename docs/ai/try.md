# 值得尝试的 AI 应用方向

1. ebpf fs : 直接修改内核，让用户态来完成加密，压缩，去重
2. kernel-moudle 的依赖关系可视化

3. 疑惑
   1. iouring cmd 机制为什么没有网卡的，这样就干掉 dpdk 就可以了
      - 不过，iouring 为什么要给 fuse 用?
        - 有什么好用的 fuse 现成的程序，用来测试一下内核的 fuse 吗?
   2. Inetstack + io_uring（用户态协议栈 + Linux 异步 I/O）
      - 用户态 netstack 和 io_uring 真的可以搭配吗？
4. rcu

- rcu 不用 ref count 实现?
- rcu statll 的含义解析

4. docs/kernel/mm/mm-gup.md 现在也是可以问问 qwen 了

5. 哪些热插是 ACPI ，哪些热插是 PCIEhp ? 似乎 nvme 的热插是 pciehp 的

直通设备可以热插吗?

6. alpine.sh 的热迁移现在似乎终于可以搞搞了 用 rust 写吧，头都裂开了

7. windows 中基本环境搭建再回顾一次

8. 将 plka 中的所有的东西全部都可以继续调用一下了

9. 实现一个 fuse

10. ceph 重新部署一下吧

https://github.com/qaqcxh/Blogs 就是这了

11. 用户态的 nvme driver + rust 实现


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
