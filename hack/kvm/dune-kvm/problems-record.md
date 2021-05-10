# problem record

## 使用 cross compile 内核导致模块无法编译 

编译一个很小的模块，得到如下的错误:
```
make -C /lib/modules/`uname -r`/build M=`pwd`
make[1]: Entering directory '/home/loongson/loongson/cross'
  CC [M]  /home/loongson/loongson/dune/mm/mm.o
/bin/sh: scripts/basic/fixdep: cannot execute binary file: Exec format error
```

```
➜  mm git:(main) ✗ file /lib/modules/4.19.73+/build/scripts/basic/fixdep
/lib/modules/4.19.73+/build/scripts/basic/fixdep: ELF 64-bit LSB shared object, x86-64, version 1 (SYSV), dynamically linked (uses shared libs), BuildID[sha1]=9ebfa9fab4526922f5ff187411283f5b89b5963c, for GNU/Linux 3.2.0, not stripped
```

之前，为了测试交叉编译的正确性，在 x86 的电脑上编译了内核，并且 sync 到 4000 上，机器可以正确运行，但是
