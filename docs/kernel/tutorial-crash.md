# 使用 crash 阅读内核

## 获取一个地址属于哪里的
kmem -s 0xffffffff81619500

- 分析下，如何使用 kmem -s 中，虚拟地址找到 slab 的各种信息的

## 补充的操作
- virsh dump --memory-only --live e5fb54af-98ec-46d7-a69b-5a8fb6b52996 g.dump

然后 crash vmlinux g.dump

- foreach bt : 所有进程的 backtrace
- bt -a : 所有的 CPU 的 backtrace
- bt -FF  264 : CPU
  - [ ] -FF 的数据，好吧，需要重新理解 kmalloc 和 stack 的关系
- search sd_fops : 搜索 sd_fops，我靠，根本不能理解为什么这个东西的实现原理啊
- dev : 展示所有的 device
- kmem
  - `-s` : 展示 k

- bt -FF -c 12
- struct hrtimer 0xffff8faa7e095ee0

ptype /o struct task_struct

struct -x o task_struct.group_leader

search


## 模块
mod -s ext2 path/to/ext2.ko.debug

## 问题
- kmem -s 真好用啊，但是如果不是 slub 中的数据，怎么处理?

## https://crash-utility.github.io/crash_whitepaper.html
