# 为 BMBT 构建一个 mini libc
基于 musl 的版本: 1e4204d522670a1d8b8ab85f1cfefa960547e8af

all code copied from musl except:
- libc/src/bits/
- libc/src/math/sqrt.c
- libc/src/math/sqrtf.c
- strncpy

## TODO
- [ ] I remember QEMU relys on glibc macros
- [ ] compile ulibc as static liba, create a new makefile for it
  - export features.h to the whole project is ugly
- [ ] what if kernel implement the memset too?
- [ ] review bits again, all code copied from loongarch64, maybe compare it with xen0n's implementation
- [ ] test the libc
- [ ] remove the flie related file API
- [ ] implement brk in the baremetal environment
  - https://stackoverflow.com/questions/68123943/advantages-of-mmap-over-sbrk
## header include
./src/include has a higher priority over ./include

## how hidden works
in musl/src/include/stdlib.h, we found:
```c
hidden void *__libc_malloc(size_t);
hidden void *__libc_malloc_impl(size_t);
hidden void *__libc_calloc(size_t, size_t);
hidden void *__libc_realloc(void *, size_t);
hidden void __libc_free(void *);
```
- [ ] create some files to test them

## stdint / limits.h / inttypes.h / stdbool
- [ ] answer this nice questions : musl 为什么需要动态的生成 bits/alltypes.h

```c
obj/include/bits/alltypes.h: $(srcdir)/arch/$(ARCH)/bits/alltypes.h.in $(srcdir)/include/alltypes.h.in $(srcdir)/tools/mkalltypes.sed
	sed -f $(srcdir)/tools/mkalltypes.sed $(srcdir)/arch/$(ARCH)/bits/alltypes.h.in $(srcdir)/include/alltypes.h.in > $@
```

- [ ] 似乎在 float_t 上的确有东西
- [ ] FILE 这个东西是不是定义的有点太随意了啊
- [ ] why NULL is defined every where，虽然很容易处理，但是无法理解

- [ ] 我希望整个 glibc 可以让用户态程序编译出来
  - [ ] 应该是有办法不使用用户态的 lib 然后只是使用系统态的
- [x] 为什么 assert.h 没有 once 来保护
- [ ] 现在 bits 下全部是按照 x86 的，除了 limits.h
  - 其中的 float 相关的需要重点关注一下
  - setjmp 完全就是瞎几把实现的
  - [ ] 需要仔细的比较一下在 types 上, la 和 x86 的实现的差别
- 似乎只是 capstone 使用过 qsort
  - AppPkg/Applications/bmbt/capstone/arch/X86/qsort.h
- [ ] 似乎只是需要补充几个函数，就可以让程序在用户态编译起来
- [ ] strerror 的使用位置非常少，也许直接删除掉?
- [ ] #261 需要逐个分析一下
- [ ] `_GUN_SOURCE` 之前的时候为什么都是默认不需要添加的，现在使用上 musl 的时候，就需要添加上了
  - [ ] 使用 musl 编译测试一下其他程序，看看能不能复现，现在 features.h 中直接添加 `_GUN_SOURCE` 真的过于粗暴

## 为了处理 env
改动:
1. 因为 UEFI 在 MdePkg 中定义了 NULL 的，而 NULL 在很多头文件中都是定义了，所以在 `env/*/uaip/libc.h` 中处理 NULL。
2. 因为 UEFI 的 StdLib 的实现的差别，需要使用 USE_UEFI_LIBC 来构建其中的差别。

细节:
1. swap.h
  - 因为 QEMU 已经实现了 bswap16 / bswap32 / bswap64，最简单的做法还是直接使用 QEMU 的实现，所以 undefined 掉 CONFIG_BYTESWAP_H
  - 因为 UEFI StdLib 总是将这几个函数包含进去了(即使不去直接 include 头文件)，所以使用 USE_UEFI_LIBC 让来包含系统的头文件
2. env/uefi/include/uapi/env.h
  - 因为 UEFI StdLib 的

## 编译选项
-g
-Wall
-Werror
-fno-stack-protector
-mno-red-zone
-nostdinc
-nostdlib
-Wextra
-fno-builtin

关于 redzone 的解释:
- https://os.phil-opp.com/red-zone/
- https://eli.thegreenplace.net/2011/09/06/stack-frame-layout-on-x86-64

## printf 的实现
似乎总体来说，几乎没有很大的挑战

- [ ] 理解一下 wchar_t

## malloc
在这两个位置讨论分析了一下 malloc 的实现:
- https://musl.openwall.narkive.com/J9ymcXt2/what-s-wrong-with-musl-s-malloc
- https://news.ycombinator.com/item?id=23080290

- [ ] 只是将 lock 简化一下，而不要改动本身的
  - [ ] 如果使用一些让人窒息的 lock 怎么办?

在分析 malloc 的原理的时候，我发现当在静态链接的时候，使用 free 与否会导致实际上调用的 malloc 不同
- https://stackoverflow.com/questions/23079997/override-weak-symbols-in-static-library
- https://stackoverflow.com/questions/51656838/attribute-weak-and-static-libraries
- try to use ./week_alias to verify the ideas

- calloc 中的 all_zerop 也是如此

- [ ] 依赖 /home/maritns3/core/musl/src/internal/libc.h 做什么
- calloc.c 为什么需要依赖 dylink.c

- https://www.cs.cmu.edu/afs/cs/academic/class/15213-f12/www/lectures/12-linking.pdf
  - 介绍了三种 interposition 的方法
  - 实际上，如果不怕麻烦，可以定义自己的 malloc 就好了，这里介绍的方法是继续使用原来的技术

- [ ] 在 /home/maritns3/core/musl/src/malloc/mallocng/meta.h 使用了一堆 4096 之类操作，这不是一个问题吗?
- [x] 调查为什么到处判断 pagesize : 应该只是因为
- [ ] 为什么需要重新制造出来一个 assert
  - 感觉没有什么特别强的道理

- [x] 似乎可以很容易的构造出来一个多线程的问题来
```c
#define MT (libc.need_locks)
```
当共享地址空间的时候，才需要上锁的

- 似乎只是 pthread 才会在乎啊，clone 根本不在乎
  - [ ] lite_malloc 的 lock 机制还是不一样的
  - https://stackoverflow.com/questions/855763/is-malloc-thread-safe 中的人都说 malloc 是安全的，但是似乎只是在 pthread 的时候是安全的
  - 从 musl 的库中可以清楚的检查到对于 clone 形成的多线程，malloc 不是安全的，但是对于 glibc 的 malloc 过于复杂，暂时不看

## 浮点
- 似乎是可以参考的 musl 库:
  - https://github.com/xen0n/musl/commit/f8ec0dbd4b08456cda7a38ee4a34924665afa69a
- 参考一下 glibc 的内容
  - [x] 编译验证测试的环境搭建起来
- [ ] 内核环境的重新搭建起来
- 搞一个 QEMU 环境来将之前的内核运行一下什么的啊
- [ ] how floating point exception handle
  - only related with `errno` or related with hardware?
- [ ] actually, we need to port the `memset` `strchr` and related function, but Linux kernel implemented them.

```c
#define _GNU_SOURCE
#include <assert.h>
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>

int main(int argc, char **argv) {
  double x = 1.0;
  double y = 2.0;
  double z = 3.0;
  float g = 1.0;
  // by inline
  printf("%lf\n", fabs(x));
  printf("%f\n", fabsf(g));
  printf("%lf\n", fma(x, y, z));
  printf("%lf\n", fmaf(x, y, z));
  printf("%lf\n", sqrt(x));
  printf("%f\n", sqrtf(g));

  // from musl
  printf("%f\n", pow(x, y));
  printf("%lf", tan(x));
  printf("%lf", rint(x));
  printf("%lf", floor(x));
  printf("%lf", ceil(x));
  printf("%lf", log(x));
  printf("%lf", log10(x));
}
```

### [ ] sqrtf and sqrt
https://stackoverflow.com/questions/57058848/glibc-uses-kernel-functions

```c
>>> br sqrt
Breakpoint 1 at 0x120000a38: file ./w_sqrt_template.c, line 33.
>>> br sqrtf
Breakpoint 2 at 0x120000a6c: file ./w_sqrt_template.c, line 33.
```
actually, they are defined by the macros.

so,  ./w_sqrt_template.c is the source codeelescope in future for configs. adding _ in front helps with duplication of same file name. ￼
```c
   0x0000000120000878 <+84>:    bl      76(0x4c) # 0x1200008c4 <__sqrt>
   0x000000012000087c <+88>:    movfr2gr.d      $r5,$f0
   0x0000000120000880 <+92>:    pcaddu12i       $r4,82(0x52)
   0x0000000120000884 <+96>:    addi.d  $r4,$r4,1528(0x5f8)
   0x0000000120000888 <+100>:   bl      27348(0x6ad4) # 0x12000735c <__printf>
   0x000000012000088c <+104>:   fld.d   $f0,$r22,-24(0xfe8)
   0x0000000120000890 <+108>:   fcvt.s.d        $f0,$f0
   0x0000000120000894 <+112>:   bl      64(0x40) # 0x1200008d4 <__sqrtf>
```

the source code is generated dynamically

### Questions
- [ ] the c language syntax?
  - copy the code directly will cause the 
```c
/* Declare sqrt for use within GLIBC.  Compilers typically inline sqrt as a
   single instruction.  Use an asm to avoid use of PLTs if it doesn't.  */
float (sqrtf) (float) asm ("__ieee754_sqrtf");
double (sqrt) (double) asm ("__ieee754_sqrt");
```
- [ ] i don't know why the dynamic library contains the symbols
- [x] sqrt and sqrtf not found
  - link with -lm

```c
/*
➜  ~/core/glibc git:(master) ✗ make
/usr/bin/ld: /tmp/ccXBnRac.o: in function `L0':
test.c:(.text+0xe8): undefined reference to `sqrt'
/usr/bin/ld: test.c:(.text+0x100): undefined reference to `sqrtf'
collect2: error: ld returned 1 exit status
make: *** [Makefile:12: all] Error 1
```
- [ ]  https://stackoverflow.com/questions/10412684/how-to-compile-my-own-glibc-c-standard-library-from-source-and-use-it#
  - [ ] what's meaning of start file
  - [ ] read the musl's spec files


### fabs
- [ ] I don't know what's `_Float32`
  - [ ] what's the purpose of fabsf32 fabsf64  fabsf32x and fabsf64x

```c
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>

double g(double x) { return __builtin_fabs(x); }

int main(int argc, char **argv) {
  double x = -10;
  double y = 1.0;
  double z = 1.0;
  // inline asm
  printf("%lf\n", fabs(x));
  printf("%f\n", fabsf(x));
  printf("%Lf\n", fabsl(x));

  printf("%lf\n", (double)fabsf32(x));  // 2
  printf("%lf\n", (double)fabsf64(x)); // 1
  printf("%lf\n", (double)fabsf32x(x)); // 1
  printf("%lf\n", (double)fabsf64x(x)); // 3 // doesn't effect me ?
}
```
- actually, ccls is really cool, next time if you can't find the definition of macros, try to define the macro, the compiler will locate it by complaints

## crt0
- crt/crt1.c : used by static library
- crt/rcrt1.c : dynamic library

in `src/env/__libc_start_main.c` we understand how 
[Auxiliary Vector](https://www.gnu.org/software/libc/manual/html_node/Auxiliary-Vector.html)
implemented in libc

## [ ] undefined reference to fwrite
ld: x86tomips-options.c:(.text+0x534): undefined reference to `fwrite'

```c
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define lsassertm(cond, ...)                                                   \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "\033[31m assertion failed in <%s> %s:%d \033[m",        \
              __FUNCTION__, __FILE__, __LINE__);                               \
      fprintf(stderr, __VA_ARGS__);                                            \
      abort();                                                                 \
    }                                                                          \
  } while (0)

int main(int argc, char *argv[]) {
  lsassertm(false, "no");
  return 0;
}
```
```mk
a:
	gcc -c -nostdlib a.c  -o a.o
	ld a.o
```
It seems that fprintf will be optimized into fwrite

## undefined extenddftf2
this can fix by link with /usr/lib/gcc/loongarch64-linux-gnu/8/libgcc.a

https://gcc.gnu.org/onlinedocs/gccint/Soft-float-library-routines.html

```txt
ld: /home/loongson/core/bmbt/build_[loongson]_[]_[]/libc/src/stdio/vfprintf.o: in function `.L5':
vfprintf.c:(.text+0x2e4): undefined reference to `__extenddftf2'
ld: /home/loongson/core/bmbt/build_[loongson]_[]_[]/libc/src/stdio/vfprintf.o: in function `.L54':
vfprintf.c:(.text+0x7fc): undefined reference to `__netf2'
ld: /home/loongson/core/bmbt/build_[loongson]_[]_[]/libc/src/stdio/vfprintf.o: in function `.L52':
vfprintf.c:(.text+0x93c): undefined reference to `__addtf3'
ld: vfprintf.c:(.text+0x96c): undefined reference to `__netf2'
ld: /home/loongson/core/bmbt/build_[loongson]_[]_[]/libc/src/stdio/vfprintf.o: in function `.L69':
vfprintf.c:(.text+0xa24): undefined reference to `__multf3'
ld: /home/loongson/core/bmbt/build_[loongson]_[]_[]/libc/src/stdio/vfprintf.o: in function `.L68':
vfprintf.c:(.text+0xab4): undefined reference to `__subtf3'
ld: vfprintf.c:(.text+0xaf0): undefined reference to `__addtf3'
ld: /home/loongson/core/bmbt/build_[loongson]_[]_[]/libc/src/stdio/vfprintf.o: in function `.L70':
vfprintf.c:(.text+0xb60): undefined reference to `__addtf3'
ld: vfprintf.c:(.text+0xb9c): undefined reference to `__subtf3'
ld: /home/loongson/core/bmbt/build_[loongson]_[]_[]/libc/src/stdio/vfprintf.o: in function `.LBB4':
vfprintf.c:(.text+0xc6c): undefined reference to `__fixtfsi'
ld: vfprintf.c:(.text+0xcbc): undefined reference to `__floatsitf'
ld: vfprintf.c:(.text+0xce4): undefined reference to `__subtf3'
ld: vfprintf.c:(.text+0xd1c): undefined reference to `__multf3'
ld: vfprintf.c:(.text+0xd6c): undefined reference to `__netf2'
ld: /home/loongson/core/bmbt/build_[loongson]_[]_[]/libc/src/stdio/vfprintf.o: in function `.L74':
vfprintf.c:(.text+0xdbc): undefined reference to `__netf2'
ld: /home/loongson/core/bmbt/build_[loongson]_[]_[]/libc/src/stdio/vfprintf.o: in function `.L80':
vfprintf.c:(.text+0x1030): undefined reference to `__netf2'
ld: vfprintf.c:(.text+0x106c): undefined reference to `__multf3'
ld: /home/loongson/core/bmbt/build_[loongson]_[]_[]/libc/src/stdio/vfprintf.o: in function `.L85':
vfprintf.c:(.text+0x1100): undefined reference to `__fixunstfsi'
ld: vfprintf.c:(.text+0x1128): undefined reference to `__floatunsitf'
ld: vfprintf.c:(.text+0x1150): undefined reference to `__subtf3'
ld: vfprintf.c:(.text+0x1188): undefined reference to `__multf3'
ld: vfprintf.c:(.text+0x11b8): undefined reference to `__netf2'
ld: /home/loongson/core/bmbt/build_[loongson]_[]_[]/libc/src/stdio/vfprintf.o: in function `.L116':
vfprintf.c:(.text+0x17b4): undefined reference to `__addtf3'
ld: /home/loongson/core/bmbt/build_[loongson]_[]_[]/libc/src/stdio/vfprintf.o: in function `.L121':
vfprintf.c:(.text+0x18d8): undefined reference to `__addtf3'
ld: vfprintf.c:(.text+0x18f4): undefined reference to `__netf2'
```

objdump -ald build_\[loongson\]_\[\]_\[\]/src/main.o > b.txt

