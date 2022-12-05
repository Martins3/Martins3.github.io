划分为两个两个部分，用户态一部分，内核一部分

# 认识 elf
- [ ] 重新回顾一下 程序员的自我修养

https://github.com/adugast/read_elf

https://fasterthanli.me/series

https://github.com/serge1/ELFIO

https://cjting.me/2020/12/10/tiny-x64-helloworld/ : 轻松的教程

[special section](https://lwn.net/Articles/531148/)

[linux 二进制分析，似乎质量还不错](https://book.douban.com/subject/27592738/)

https://cjting.me/2020/12/10/tiny-x64-helloworld/

# binfmt_elf

## kallsyms
1. 如何构建表格的
2. 表格的格式是什么
3. 查询的方法
> 只有 700 行，你值得拥有

# [elf 格式](https://linux-audit.com/elf-binaries-on-linux-understanding-and-analysis/)
博客笔记整理

## 命令
file 帮助我解决了 在 x86 上交叉编译内核，让后 sync 到 3a4000 上去，make install, 这种在 3a4000 上没有办法安装内核模块。

## EXPORT_SYMBO 的工作原理是什么

## [ ] 将内核处理 elf 的笔记放过来
/home/maritns3/core/vn/kernel/plka/syn/fs/binfmt_elf.md

## grub 也是可以加载 elf 的

## [ ]  动态链接库
https://tinylab-1.gitbook.io/cbook/02-chapter4

## 辅助向量

## 工具
- https://github.com/horsicq/XELFViewer
