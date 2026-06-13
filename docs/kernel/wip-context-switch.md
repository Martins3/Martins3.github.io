# 上下文切换总结

## 基本的

- ( scheduler 的就先不考虑，只用考虑已经知道应该换到哪一个环境中去)

## CPU 设计的上下文

似乎是如果触发 exception 和中断。

## GPU 的上下文切换


## function level context

函数寄存器

## ucontext

协程

## 内核中的上下文切换

考虑的东西就非常多了

### 需要维护的东西

硬件:
- simp
- fpu
- tlb

软件:
- cgroup ? 这个算吗?

### 场景不同的

kernel -> kernel
userspace - kernel

kernel 或者 userspace 被中断打断
kernel 或者 userspace 被 exception 打断

## 更加通用的概念
session : 各种软件中的概念
sudo su : 切换

## 加速的方法

## 这里讲 fpu 对于上下文切换的影响
docs/kernel/fpu/overview.md

## 这里从虚拟机的切换分析上下文切换
docs/kvm/nested/nested.md

## 上下文切换引入的问题

1. 切换的开销
2. tlb miss 和 cache miss

解决的办法:
1. pasid
	- iommu 中的解决办法: drivers/iommu/amd/pasid.c

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
