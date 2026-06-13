# 程序员的自我修养 : 动态链接

## 7.1 为什么需要动态链接
好处:
1. 内存和磁盘空间
2. 将程序拆分，升级程序只需要部分升级
3. 增加程序的可扩展(plugin)和兼容(printf)
 

## 7.2 简单的动态链接的例子
1. `-shared`表示产生共享对象
2. `-fPIC` 表示
3. 动态链接库的装载地址不是确定的，装载器会根据当前地址空间的空闲状况动态分配一块虚拟地址空间。

1. `clang -o program2 program2.c ./mylib.so` 不可以改为 `clang -o program2 program2.c mylib.so`



## 7.3 地址无关代码

#### 7.3.1 固定装载地址的困惑
静态共享库 导致　地址冲突，而且会导致模块升级困难

#### 7.3.2 装载时重定位
装载时重定位 用于解决在没有虚拟存储概念时，程序装载到物理内存中间。
它无法处理动态模块被多个进程共享的问题。

#### 7.3.3　地址无关代码
基本思想是: 将指令中间需要修改的部分和数据部分放在一起。

忽然发现:和动态链接库中间的全局变量并不会出现冲突，而是默认使用本地的。而且不同的动态链接库的各种强符号也都不会互相冲突，使用谁的符号取决于谁首先出现。

模块内部调用和跳转看似简单，但是会有共享对象 全局符号介入(interposition)

动态链接库的数据段，每一个进程都是独立拥有自己的备份的。Got段被放置于数据段中间。



P192　为了获取PC值，有很多方法，难道最简单的不是直接访问rip 吗 ?


```
0000000000001139 <bar>:
    1139:	55                   	push   %rbp
    113a:	48 89 e5             	mov    %rsp,%rbp
    113d:	c7 05 01 2f 00 00 01 	movl   $0x1,0x2f01(%rip)        # 4048 <a>
    1144:	00 00 00 
    1147:	48 8b 05 92 2e 00 00 	mov    0x2e92(%rip),%rax        # 3fe0 <b>
    114e:	c7 00 02 00 00 00    	movl   $0x2,(%rax)
    1154:	90                   	nop
    1155:	5d                   	pop    %rbp
    1156:	c3                   	retq   

18 .got          00000028  0000000000003fd8  0000000000003fd8  00002fd8  2**3
                  CONTENTS, ALLOC, LOAD, DATA

0000000000003fe0 R_X86_64_GLOB_DAT  b
```

a是模块内变量,　b的地址在.got中间
```
0x114e + 0x2e92 -0x3fe0 = 0
```
#### 7.3.4 共享模块的全局变量问题
当共享模块的一个文件引用全局变量的时候，它无法确定该全局变量是
该共享模块内的还是外部的。

1. 所用使用这个变量的指令都指向位于可执行文件中的那个副本
2. 即使是模块内部的引用，也会产生跨模块代码。

P198 页非常有问题:
1. 当module.c 是可执行文件中间的一部分的时候，当所有链接都完成，难道还会不知道`global`变量到底是放在`.bss`中间还是外部吗 ?
最多当生成`.o` 的时候无法确定，但是此时无法确定的问题解决方法不是在静态链接的时候已经讨论过吗?
2. 同样的道理，难道当整个动态链接库都生成了，还是没有办法确定一个extern 符号引用的内部还是外部吗 ? 如果最终可以在其他文件中间找到，那么就是模块内，否则模块外
3. 它的说法缺乏证据，b@@Base和 bar@@Base 有什么区别 ?
```
➜  7 git:(master) ✗ objdump -R mylib.so

mylib.so:     file format elf64-x86-64

DYNAMIC RELOCATION RECORDS
OFFSET           TYPE              VALUE 
0000000000003e08 R_X86_64_RELATIVE  *ABS*+0x0000000000001130
0000000000003e10 R_X86_64_RELATIVE  *ABS*+0x00000000000010e0
0000000000004038 R_X86_64_RELATIVE  *ABS*+0x0000000000004038
0000000000003fd8 R_X86_64_GLOB_DAT  _ITM_deregisterTMCloneTable
0000000000003fe0 R_X86_64_GLOB_DAT  b
0000000000003fe8 R_X86_64_GLOB_DAT  __gmon_start__
0000000000003ff0 R_X86_64_GLOB_DAT  _ITM_registerTMCloneTable
0000000000003ff8 R_X86_64_GLOB_DAT  __cxa_finalize@GLIBC_2.2.5
0000000000004018 R_X86_64_JUMP_SLOT  bar@@Base
0000000000004020 R_X86_64_JUMP_SLOT  printf@GLIBC_2.2.5
0000000000004028 R_X86_64_JUMP_SLOT  foo@@Base
0000000000004030 R_X86_64_JUMP_SLOT  ext


➜  7 git:(master) ✗ objdump -R program1.out 

program1.out:     file format elf64-x86-64

DYNAMIC RELOCATION RECORDS
OFFSET           TYPE              VALUE 
0000000000003dd8 R_X86_64_RELATIVE  *ABS*+0x0000000000001150
0000000000003de0 R_X86_64_RELATIVE  *ABS*+0x0000000000001100
0000000000004038 R_X86_64_RELATIVE  *ABS*+0x0000000000004038
0000000000003fd8 R_X86_64_GLOB_DAT  _ITM_deregisterTMCloneTable
0000000000003fe0 R_X86_64_GLOB_DAT  __libc_start_main@GLIBC_2.2.5
0000000000003fe8 R_X86_64_GLOB_DAT  __gmon_start__
0000000000003ff0 R_X86_64_GLOB_DAT  _ITM_registerTMCloneTable
0000000000003ff8 R_X86_64_GLOB_DAT  __cxa_finalize@GLIBC_2.2.5
0000000000004044 R_X86_64_COPY     num
0000000000004018 R_X86_64_JUMP_SLOT  __stack_chk_fail@GLIBC_2.4
0000000000004020 R_X86_64_JUMP_SLOT  foobar
0000000000004028 R_X86_64_JUMP_SLOT  printf@GLIBC_2.2.5
```


#### 7.3.5 数据段的地址无关性


1. 需要补充的知识:
  1. 重定位入口 重定位表 ?
  2. 数据段中间的绝对引用为什么就需要重定位?
  3. 重定位和地址无关的　关系是什么?

## 7.4


#### 7.5.3

有几个问题:
1. `readelf -sD lib.so` 实际输出只有hash 没有symtab 的部分
2. `.symtab` 会被静态链接使用吗
3. 既然`.symtab` 中间包含了　`.dynsym`的内容，为什么还是需要创建`.dynsym` 出来

#### 7.5.4
关于使用PIC前后的区别:
1. 为什么printf 和　sleep 会从`.rel.plt` 被移动到 `.rel.dyn`
2. R_386_RELATIVE 是用来做什么，是不是由于PIC去掉之后，对于glibc 就静态的链接了


## 7.6 动态链接的步骤和实现

#### 7.6.1 动态链接器自举
不能理解为什么，为什么动态链接器　想要使用的自己的全局变量和静态变量都这么复杂。
: 要自己获取自己重定位表　和 符号表, 得到重定位入口

什么是重定位入口 ?





## 问题
1. 使用ld静态链接, 动态链接是使用什么工具 ?
2. 为什么动态链接　/proc/self/maps 下，同一个动态链接库出现多次?
```
7f78f01c6000-7f78f01e8000 r--p 00000000 103:05 2630199                   /usr/lib/libc-2.29.so
7f78f01e8000-7f78f0334000 r-xp 00022000 103:05 2630199                   /usr/lib/libc-2.29.so
7f78f0334000-7f78f0380000 r--p 0016e000 103:05 2630199                   /usr/lib/libc-2.29.so
7f78f0380000-7f78f0381000 ---p 001ba000 103:05 2630199                   /usr/lib/libc-2.29.so
7f78f0381000-7f78f0385000 r--p 001ba000 103:05 2630199                   /usr/lib/libc-2.29.so
7f78f0385000-7f78f0387000 rw-p 001be000 103:05 2630199                   /usr/lib/libc-2.29.so
```

3. 使用`readelf -l `分析任何文件，结果都是显示该文件为
Elf file type is DYN (Shared object file) ?

5. 


