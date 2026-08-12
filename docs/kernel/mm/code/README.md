# 测试内容记录

## mmap 之后 close(fd)

```sh
./mmap-close-fd.out      # MAP_SHARED + close(fd)
./mmap-close-fd.out 2    # MAP_PRIVATE + close(fd)
./mmap-close-fd.out 3    # mmap + unlink + close(fd)
```

结论: close(fd) 不会撤销映射, 映射依然可以正常读写。

原因: mmap 时内核执行 `vma->vm_file = get_file(file)` (mm/mmap.c 的 `mmap_region()`),
VMA 自己持有 struct file 的引用; close(fd) 只释放 fd 的引用, 只有 munmap 时
`remove_vma()` 里的 `fput(vma->vm_file)` 才释放最后一个引用。

实验观察:
1. close 之后 /proc/self/maps 中映射还在
2. close 之后 fd 号被复用 (new open got fd = 3)
3. MAP_SHARED 下 close 之后写入, 通过新 fd 打开文件依然能看到
4. MAP_PRIVATE 下 close 之后写入, 文件内容不变 (写时复制)
5. mmap + unlink + close 之后, 映射依然可用, maps 中显示 `(deleted)`,
   inode 因为 VMA 引用而存活

## memfd

```sh
./memfd.out         # 未 ftruncate 的 memfd, mmap 后访问触发 SIGBUS
./memfd.out 1       # 20G memfd: touch -> close(fd) -> munmap
./memfd.out 2       # close(fd) 后 touch 映射区域, 正常, 不会 segfault
```

test 1 的 getchar() 需要交互输入才能继续, 直接重定向 /dev/null 会立即跳过
中间状态。用 FIFO 控制放行时机可以观察到:

```sh
mkfifo /tmp/f
sleep 1000 > /tmp/f &   # 保持写端打开, 让读端 open 不阻塞
./memfd.out 1 < /tmp/f &
# 轮询 VmRSS 到 20G 后, 此时 close(fd) 已执行:
#   /proc/PID/fd 中已无 memfd, 但 maps 中映射还在, RSS 保持 20G
#   -> close(fd) 不释放内存
echo x > /tmp/f          # 放行 getchar -> munmap
#   映射消失, RSS 释放到几 MB
```

## rmap
```sh
make && rm -f us_xfr_v2_uds_lib && ./rmap.out
```
输入 `1 fork` 让 root 构建 fork 出来 2

`2 munmap` 可以观察到 avc 解散的过程:

```txt
@[
    unlink_anon_vmas+0
    unmap_region+272
    do_vmi_align_munmap+796
    do_vmi_munmap+184
    __vm_munmap+172
    __arm64_sys_munmap+40
    invoke_syscall+116
    el0_svc_common.constprop.0+72
    do_el0_svc+36
    el0_svc+60
    el0t_64_sync_handler+288
    el0t_64_sync+404
]: 1
```

在一个 terminal 中观察:
```sh
viddy pstree 1007409 -p
```

## userfaultfd 性能测量

```txt
total :latencies(ns):0
latencies(ns):0 121107
1 13226
2 8812
3 8097
4 7621
5 8279
6 15493
7 8510
8 7443
9 7227
10 11014
11 7998
12 7659
13 7416
14 7464
15 7841
16 7701
17 7958
18 7304
19 7755
20 21
21 44
22 17
23 16
24 17
25 17
26 16
27 16
28 17
29 16
```
1. 不知道第一个 page 为什么那么大的延迟
2. 发现拷贝一个 page 也就是多几百 ns 的样子

## 真的很奔溃
抄的两个例子都有这个问题 : UFFD_EVENT_PAGEFAULT

##
sudo sysctl -w vm.overcommit_memory=1
echo 0 | sudo tee /proc/sys/vm/watermark_boost_factor

## memfd 的代码来自于
https://github.com/a-darwish/memfd-examples

库里面应该补充一下，uds 的发送和接受 fd 的操作

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
