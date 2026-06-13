
## selinux 到底是什么个原理
```txt
🧀  sudo perf ftrace -G ksys_mmap_pgoff ls | grep selinux
[sudo] password for martins3:
 12)               |        selinux_mmap_file() {
 12)   0.057 us    |            selinux_mmap_addr();
 12)               |            selinux_vm_enough_memory() {
```

## kernel security
https://github.com/Arinerron/CVE-2022-0847-DirtyPipe-Exploit
https://www.hackthebox.com/blog/Dirty-Pipe-Explained-CVE-2022-0847
https://github.com/Lissy93/personal-security-checklist
https://github.com/CTFd/CTFd
https://ctfd.io/

## kernel security 问题分析
https://github.com/ZJU-SEC/Readings/blob/main/README.md

## 这个目录
security/yama/

（如 kptr_restrict、dmesg_restrict ，增加偏移推测难度。
函数粒度随机化 : https://www.phoronix.com/news/Linux-FGKASLR-2022

## audit 是个什么原理?

## 几个经典错误

https://copy.fail/
https://github.com/V4bel/dirtyfrag
https://dirtycow.ninja/

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
