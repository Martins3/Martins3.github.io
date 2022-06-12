# lua

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

似乎是在 `luaV_execute` 中进行虚拟指令的执行，
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

## [从零开始实现 Lua 虚拟机 ( UniLua 开发过程 )](https://zhuanlan.zhihu.com/p/22476315)
- https://www.lua.org/doc/jucs05.pdf 的确不错，而且 lua 根本没看懂啊
- 这也是一个很不错的分析参考: https://www.zhihu.com/question/20617406

这个东西介绍了基本的 registr based vm 的实现，也许可以作为一个基础
https://www.andreinc.net/2021/12/01/writing-a-simple-vm-in-less-than-125-lines-of-c

[^1]: https://github.com/efrederickson/LuaAssemblyTools/blob/master/etc/ChunkSpy.lua
[^2]: 核心参考 : https://github.com/lichuang/Lua-Source-Internal
