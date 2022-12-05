# 可能的操作试验

## 非可替代模块
1. 向 page fault 的处理机制中间插入代码，至少
2. 在内核和用户之间添加 TLB flush 操作，然后检查性能下降的程度。

## 可替代
1. 新的文件系统
2. 新的 vma 
3. 写一个 内核驱动，从中间导出各种内核信息

4. 能不能通过访问 /dev/nvme0n1p3 从而实现一个只读的，用户层的文件系统 ?

5. 写一个使用 Kprobe 的使用教程。

## 测试和读取
1. 测试一下内核中间各种类型的 context switch 的数量

https://github.com/mengning/mykernel 很有意思

## 分析类的总结:
1. ./a.out 输出 printf 的全过程 : 分析 strace 中间的每一个函数


## 想法
1. 内核存在内核模块的自动化测试框架吗 ?



感觉 strace 可以作为很好的分析办法!
➜  hack git:(draft) ✗ strace echo "hello world" > ~/wow.md

## 写一个系列的文章
learn linux kernel with BPF

## https://github.com/ivandavidov/minimal
https://github.com/MichielDerhaeg/build-linux
