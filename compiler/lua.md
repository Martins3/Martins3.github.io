# lua


## 构建的理念
- checksheet
- 准备知识
  - 主要讲解这一个: https://github.com/nomemory/lc3-vm

## 为什么是 Lua

- vim
- 资料多

## 笔记
- `lua_State` : per thread state

## 问题
- [ ] jit 技术
- [ ] 如何实现 GC 的
- [ ] 如何构建的 tail call 的
- [ ] 如何实现 coroutine 的
- [ ] hash table 在什么地方
- [ ] https://github.com/LuaJIT/LuaJIT
- [x] luaD 和 luaV 中的 D 和 V 都是什么意思
    - ldo.c Stack and Call structure of Lua
    - luaV lvm.c
- [ ] ipairs / setmetatable 这种函数都是在什么地方实现的

## 学习一下 lua 语言
- https://learnxinyminutes.com/docs/lua/ : 大致了解
  - [ ] 从 metatables 构建 Class，无法理解啊
- https://www.lua.org/pil/contents.html : 详细阅读

## 问题
- [ ] 对于 lua 的基本设想，首先，将其编译为自己的中间代码，然后可以解释执行，也可以使用 luajit 编译为程序

- 好家伙，为什么找到和架构耦合的部分啊，代码生成的部分在什么位置啊
- 词法分析器在哪里 lobject lstring
- 虚拟机 lState

- 外部的库是如何引入的？

```
#0  0x000055555556851c in stack_init ()
#1  0x000055555556862e in f_luaopen () // 有趣，进行了多个初始化
#2  0x000055555556043e in luaD_rawrunprotected ()
#3  0x0000555555568a54 in lua_newstate ()
#4  0x0000555555572eb8 in luaL_newstate ()
#5  0x000055555555caeb in main ()
```

`f_luaopen`

```
#0  0x0000555555560f50 in luaD_precall ()
#1  0x000055555556137c in luaD_callnoyield ()
#2  0x000055555555ce57 in f_call ()
#3  0x000055555556043e in luaD_rawrunprotected ()
#4  0x0000555555561600 in luaD_pcall ()
#5  0x000055555555e3c1 in lua_pcallk ()
#6  0x000055555555cb44 in main ()
```

是在 `luaV_execute` 中进行虚拟指令的执行，
```
#0  luaV_execute (L=L@entry=0x5555555972a8, ci=0x555555598af0) at lvm.c:1145
#1  0x0000555555561392 in ccall (inc=65537, nResults=-1, func=0x555555597950, L=0x5555555972a8) at ldo.c:586
#2  luaD_callnoyield (L=L@entry=0x5555555972a8, func=0x555555597950, nResults=-1) at ldo.c:602
#3  0x000055555555ce57 in f_call (L=L@entry=0x5555555972a8, ud=ud@entry=0x7fffffffd200) at lapi.c:955
#4  0x000055555556043e in luaD_rawrunprotected (L=L@entry=0x5555555972a8, f=f@entry=0x55555555ce44 <f_call>, ud=ud@entry=0x7fffffffd200) at ldo.c:144
#5  0x0000555555561600 in luaD_pcall (L=L@entry=0x5555555972a8, func=func@entry=0x55555555ce44 <f_call>, u=u@entry=0x7fffffffd200, old_top=80, ef=<optimized out>) at l
do.c:882
#6  0x000055555555e3c1 in lua_pcallk (L=0x5555555972a8, nargs=<optimized out>, nresults=-1, errfunc=<optimized out>, ctx=<optimized out>, k=<optimized out>) at lapi.c:
979
#7  0x000055555555c091 in docall (L=0x5555555972a8, narg=0, nres=-1) at lua.c:150
#8  0x000055555555c51d in handle_script (L=0x5555555972a8, argv=<optimized out>) at lua.c:239
#9  0x000055555555ca80 in pmain (L=0x5555555972a8) at lua.c:603
#10 0x00005555555610de in precallC (f=0x55555555c900 <pmain>, nresults=1, func=0x555555597910, L=0x5555555972a8) at ldo.c:486
#11 luaD_precall (L=L@entry=0x5555555972a8, func=<optimized out>, func@entry=0x555555597910, nresults=nresults@entry=1) at ldo.c:550
#12 0x000055555556137c in ccall (inc=65537, nResults=1, func=0x555555597910, L=0x5555555972a8) at ldo.c:584
#13 luaD_callnoyield (L=L@entry=0x5555555972a8, func=0x555555597910, nResults=1) at ldo.c:602
#14 0x000055555555ce57 in f_call (L=L@entry=0x5555555972a8, ud=ud@entry=0x7fffffffd480) at lapi.c:955
#15 0x000055555556043e in luaD_rawrunprotected (L=L@entry=0x5555555972a8, f=f@entry=0x55555555ce44 <f_call>, ud=ud@entry=0x7fffffffd480) at ldo.c:144
#16 0x0000555555561600 in luaD_pcall (L=L@entry=0x5555555972a8, func=func@entry=0x55555555ce44 <f_call>, u=u@entry=0x7fffffffd480, old_top=16, ef=<optimized out>) at l
do.c:882
#17 0x000055555555e3c1 in lua_pcallk (L=0x5555555972a8, nargs=<optimized out>, nresults=1, errfunc=<optimized out>, ctx=<optimized out>, k=<optimized out>) at lapi.c:9
79
#18 0x000055555555cb44 in main (argc=2, argv=0x7fffffffd5b8) at lua.c:640
```


通过 parse 可以返回一个 LClosure

```c
    cl = luaY_parser(L, p->z, &p->buff, &p->dyd, p->name, c);
```


## 阅读建议[^3]
Recommended reading order:
lmathlib.c, lstrlib.c: get familiar with the external C API. Don't bother with the pattern matcher though. Just the easy functions.
lapi.c: Check how the API is implemented internally. Only skim this to get a feeling for the code. Cross-reference to lua.h and luaconf.h as needed.
lobject.h: tagged values and object representation. skim through this first. you'll want to keep a window with this file open all the time.
lstate.h: state objects. ditto.
lopcodes.h: bytecode instruction format and opcode definitions. easy.
lvm.c: scroll down to `luaV_execute`, the main interpreter loop. see how all of the instructions are implemented. skip the details for now. reread later.
ldo.c: calls, stacks, exceptions, coroutines. tough read.
lstring.c: string interning. cute, huh?
ltable.c: hash tables and arrays. tricky code.
ltm.c: metamethod handling, reread all of lvm.c now.
You may want to reread lapi.c now.
ldebug.c: surprise waiting for you. abstract interpretation is used to find object names for tracebacks. does bytecode verification, too.
lparser.c, lcode.c: recursive descent parser, targetting a register-based VM. start from chunk() and work your way through. read the expression parser and the code generator parts last.
lgc.c: incremental garbage collector. take your time.
Read all the other files as you see references to them. Don't let your stack get too deep though.
If you're done before X-Mas and understood all of it, you're good. The information density of the code is rather high.

### lmathlib.c

```txt
#0  math_min (L=0x5555555ac2a8) at lmathlib.c:206
#1  0x00005555555643ca in precallC (L=0x5555555ac2a8, func=0x5555555ac970, nresults=0, f=0x55555558efd5 <math_min>) at ldo.c:510
#2  0x00005555555646f2 in luaD_precall (L=0x5555555ac2a8, func=0x5555555ac970, nresults=0) at ldo.c:576
#3  0x000055555557fe70 in luaV_execute (L=0x5555555ac2a8, ci=0x5555555adaf0) at lvm.c:1682
#4  0x000055555556495c in ccall (L=0x5555555ac2a8, func=0x5555555ac950, nResults=-1, inc=65537) at ldo.c:618
#5  0x00005555555649d9 in luaD_callnoyield (L=0x5555555ac2a8, func=0x5555555ac950, nResults=-1) at ldo.c:636
#6  0x000055555555fd32 in f_call (L=0x5555555ac2a8, ud=0x7fffffffd150) at lapi.c:1035
#7  0x000055555556341a in luaD_rawrunprotected (L=0x5555555ac2a8, f=0x55555555fcf9 <f_call>, ud=0x7fffffffd150) at ldo.c:144
#8  0x0000555555565296 in luaD_pcall (L=0x5555555ac2a8, func=0x55555555fcf9 <f_call>, u=0x7fffffffd150, old_top=80, ef=64) at ldo.c:934
#9  0x000055555555fe06 in lua_pcallk (L=0x5555555ac2a8, nargs=0, nresults=-1, errfunc=3, ctx=0, k=0x0) at lapi.c:1061
#10 0x000055555555c081 in docall (L=0x5555555ac2a8, narg=0, nres=-1) at lua.c:160
#11 0x000055555555c4be in handle_script (L=0x5555555ac2a8, argv=0x7fffffffd690) at lua.c:256
#12 0x000055555555d0ff in pmain (L=0x5555555ac2a8) at lua.c:644
#13 0x00005555555643ca in precallC (L=0x5555555ac2a8, func=0x5555555ac910, nresults=1, f=0x55555555cf57 <pmain>) at ldo.c:510
#14 0x00005555555646f2 in luaD_precall (L=0x5555555ac2a8, func=0x5555555ac910, nresults=1) at ldo.c:576
#15 0x0000555555564934 in ccall (L=0x5555555ac2a8, func=0x5555555ac910, nResults=1, inc=65537) at ldo.c:616
#16 0x00005555555649d9 in luaD_callnoyield (L=0x5555555ac2a8, func=0x5555555ac910, nResults=1) at ldo.c:636
#17 0x000055555555fd32 in f_call (L=0x5555555ac2a8, ud=0x7fffffffd530) at lapi.c:1035
#18 0x000055555556341a in luaD_rawrunprotected (L=0x5555555ac2a8, f=0x55555555fcf9 <f_call>, ud=0x7fffffffd530) at ldo.c:144
#19 0x0000555555565296 in luaD_pcall (L=0x5555555ac2a8, func=0x55555555fcf9 <f_call>, u=0x7fffffffd530, old_top=16, ef=0) at ldo.c:934
#20 0x000055555555fe06 in lua_pcallk (L=0x5555555ac2a8, nargs=2, nresults=1, errfunc=0, ctx=0, k=0x0) at lapi.c:1061
#21 0x000055555555d227 in main (argc=2, argv=0x7fffffffd688) at lua.c:671
```
- [ ]  pmain : 中会加载对应的库，但是上面的这个 backtrace 根本看不懂哇
- [ ] `math_min` 会调用 `luaV_equalobj` 之类的，因为需要考虑到 math.min 的被重载了
- [ ] 似乎没有看到任何和 external C API 相关的内容

### lapi.c

### lobject.h / lstate.h / lopcodes.h

### lvm.c
最关键的函数 `luaV_execute`

### ldo.c
call stack coroutines

### ltable.c
```txt
#0  luaH_new (L=0x0) at ltable.c:624
#1  0x00005555555717a7 in init_registry (L=0x5555555ac2a8, g=0x5555555ac370) at lstate.c:218
#2  0x000055555557188f in f_luaopen (L=0x5555555ac2a8, ud=0x0) at lstate.c:235
#3  0x000055555556341a in luaD_rawrunprotected (L=0x5555555ac2a8, f=0x555555571849 <f_luaopen>, ud=0x0) at ldo.c:144
#4  0x00005555555720dc in lua_newstate (f=0x555555583a8b <l_alloc>, ud=0x0) at lstate.c:402
#5  0x0000555555583d4b in luaL_newstate () at lauxlib.c:1089
#6  0x000055555555d195 in main (argc=2, argv=0x7fffffffd688) at lua.c:663
```

```txt
#0  luaH_new (L=0x5555555ac2a8) at ltable.c:624
#1  0x000055555557182f in init_registry (L=0x5555555ac2a8, g=0x5555555ac370) at lstate.c:224
#2  0x000055555557188f in f_luaopen (L=0x5555555ac2a8, ud=0x0) at lstate.c:235
#3  0x000055555556341a in luaD_rawrunprotected (L=0x5555555ac2a8, f=0x555555571849 <f_luaopen>, ud=0x0) at ldo.c:144
#4  0x00005555555720dc in lua_newstate (f=0x555555583a8b <l_alloc>, ud=0x0) at lstate.c:402
#5  0x0000555555583d4b in luaL_newstate () at lauxlib.c:1089
#6  0x000055555555d195 in main (argc=2, argv=0x7fffffffd688) at lua.c:663
```

```txt
#0  luaH_new (L=0x5555555ac2a8) at ltable.c:624
#1  0x000055555555ef7f in lua_createtable (L=0x5555555ac2a8, narray=0, nrec=0) at lapi.c:763
#2  0x00005555555837f2 in luaL_getsubtable (L=0x5555555ac2a8, idx=-1001000, fname=0x55555559abe4 "_LOADED") at lauxlib.c:954
#3  0x0000555555583852 in luaL_requiref (L=0x5555555ac2a8, modname=0x55555559af5e "_G", openf=0x55555558b45b <luaopen_base>, glb=1) at lauxlib.c:970
#4  0x0000555555583e65 in luaL_openlibs (L=0x5555555ac2a8) at linit.c:61
#5  0x000055555555d057 in pmain (L=0x5555555ac2a8) at lua.c:634
#6  0x00005555555643ca in precallC (L=0x5555555ac2a8, func=0x5555555ac910, nresults=1, f=0x55555555cf57 <pmain>) at ldo.c:510
#7  0x00005555555646f2 in luaD_precall (L=0x5555555ac2a8, func=0x5555555ac910, nresults=1) at ldo.c:576
#8  0x0000555555564934 in ccall (L=0x5555555ac2a8, func=0x5555555ac910, nResults=1, inc=65537) at ldo.c:616
#9  0x00005555555649d9 in luaD_callnoyield (L=0x5555555ac2a8, func=0x5555555ac910, nResults=1) at ldo.c:636
#10 0x000055555555fd32 in f_call (L=0x5555555ac2a8, ud=0x7fffffffd530) at lapi.c:1035
#11 0x000055555556341a in luaD_rawrunprotected (L=0x5555555ac2a8, f=0x55555555fcf9 <f_call>, ud=0x7fffffffd530) at ldo.c:144
#12 0x0000555555565296 in luaD_pcall (L=0x5555555ac2a8, func=0x55555555fcf9 <f_call>, u=0x7fffffffd530, old_top=16, ef=0) at ldo.c:934
#13 0x000055555555fe06 in lua_pcallk (L=0x5555555ac2a8, nargs=2, nresults=1, errfunc=0, ctx=0, k=0x0) at lapi.c:1061
#14 0x000055555555d227 in main (argc=2, argv=0x7fffffffd688) at lua.c:671
```
- 我的龟龟，一个 math.min(1,2) 就需要创建多大 20 次

### lstate.c
- [ ] 有趣的 `f_luaopen`

### ltm.c
metamethod, 只有 200 行

- [ ] 所以，例如 `_index` 是如何实现的

### lcode.c
- [ ] addK 的调用

### ldebug.c
The debug library comprises two kinds of functions: introspective functions and hooks

- [ ] ldblib.c 中过来的

### lparser.c
- [ ] 据说一次遍历的，如何实现的

### lgc.c
- 很难的哇

### ldblib.c

## [从零开始实现 Lua 虚拟机 ( UniLua 开发过程 )](https://zhuanlan.zhihu.com/p/22476315)

- VM 部分主要的工作是实现虚拟机的指令分派执行循环( `luaV_execute` )
- Lua 所有类型的对象都用 TValue 结构表示
- Lua 的调用返回地址并不存放在栈上，而是用一个 CallInfo 链来存放
  - `lua_State` 结构中的 stack 是一个指针，指向了一个可以动态增长的数组
- [ ] 在 Lua 的实现里，对 C 函数的引用分为不含 upvalue 的 light C function 和 含 upvalue 的 C closure
- Proto 就是函数的原型，在 Lua 中对应的结构如下
- CallInfo 是一个记录函数调用相关信息的一个结构

## https://www.lua.org/doc/jucs05.pdf
Lua represents values as tagged unions

Each closure has a reference to its corresponding prototype, a reference to its environment (a table
wherein it looks for global variables), and an array of references to upvalues, which are used to access outer local variables.

Lua uses a structure called an upvalue to implement closures.
Any outer local variable is accessed indirectly through an upvalue.
The upvalue originally points to the stack slot wherein the variable lives

- [ ] 让人迷茫的 section 5

When Lua enters a function, it preallocates from the stack an activation record
large enough to hold all the function registers. All local variables are allocated
in registers. As a consequence, access to local variables is specially efficient.

## https://github.com/middlefeng/LuaVMRead
- [ ] `luaD_precall` 和 `lua_pcallk` 有什么关系?

## closure 和 lexical scoping
- 什么是 closure : https://stackoverflow.com/questions/36636/what-is-a-closure

## [ ] 也许首先深入理解一下设计思想: https://www.lua.org/doc/jucs17.pdf

- 这也是一个很不错的分析参考: https://www.zhihu.com/question/20617406

## https://github.com/lichuang/Lua-Source-Internal
- `stack_init` 函数中,这个函数主要就是对Lua栈和CallInfo数组的初始化:
- `lua_State` 结构体成员变量有base,stack,top.stack保存的是数组的初始位置,base会根据每次函数调用的情况发生变化,top指针指向的是当前第一个可用的栈位置,每次向栈中增加/删减元素都要对应的增减top指针.注意到这个数组的成员类型是TValue类型的,从前面的讨论已经知道了TValue这个数据类型是Lua中表示所有数据的通用类型.
- `CallInfo`结构体,是每次有函数调用时都会去初始化的一个结构体,它的成员变量中,也有top,base指针,

- 存放分析Lua代码之后生成相关的opcode,是存放在Proto结构体中,一个Lua文件有一个总的Proto结构体,如果它内部还有定义函数,那么每个函数也有一个对应的Proto结构体.这里仅列列举出与指令执行相关的几个成员:
  - `TValue *k`:存放常量的数组.
  - `Instruction *code`:存放指令的数组.
  - `struct Proto **p`:该Proto内定义的函数相关的Proto数据.

- Lua代码中对不同的指令格式提供了几个函数,`luaK_codeABx`/`luaK_codeABC`等,这几个函数最终都会调用函数`luaK_code`

- `luaT_gettmbyobj` : 根据不同的数据类型,返回元方法的函数,只有在数据类型为Table以及udata的时候,才能拿到对象的metatable表,其他时候是到`global_State`结构体的成员mt来获取的,但是这个对于其他的数据类型而言,一直是空值.
  - 好家伙，有点麻烦，打住
    - [ ] udata 为什么可以有 metatable 的
    - [ ] 为什么

## [ ] 如何和 C 语言交互的
https://www.lua.org/pil/28.1.html

## [ ] 什么是 weak table 的哇


## 后续
- eBPF 的 jit 看一下

[^1]: https://github.com/efrederickson/LuaAssemblyTools/blob/master/etc/ChunkSpy.lua
[^3]: https://www.reddit.com/r/programming/comments/63hth/ask_reddit_which_oss_codebases_out_there_are_so/c02pxbp/
