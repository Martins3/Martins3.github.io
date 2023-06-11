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

先从 help 的输出说起:

## kmem

### 分析一个地址属于那个 slab 的
```txt
crash> kmem -s ffff8040a10b8688
CACHE OBJSIZE ALLOCATED TOTAL SLABS SSIZE NAME
ffff80408001ac00 632 54215 77214 757 64k inode_cache
SLAB MEMORY NODE TOTAL ALLOCATED FREE
ffff7fe0102842c0 ffff8040a10b0000 0 102 12 90
FREE / [ALLOCATED]
[ffff8040a10b8480]
```
我猜测是通过遍历所有的 slub cache 来实现的。

## ps

### 直接使用名称，而且支持正则
注意: 是单引号
```txt
crash> ps 'tmux*'
      PID    PPID  CPU       TASK        ST  %MEM      VSZ      RSS  COMM
     1752    1665  28  ffff88800a8317c0  IN   0.4    23648     3788  tmux: client
     1754       1   0  ffff888012852f80  IN   0.4    24188     4060  tmux: server
```

### ps -y 限制 policy
