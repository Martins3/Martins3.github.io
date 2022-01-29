# 阅读 musl 学到的一些东西

- [官方网站](https://www.musl-libc.org/)

## 准备工作

```sh
git clone https://github.com/bminor/musl
cd musl
mkdir install
./configure --enable-debug --prefix=$(pwd)/install
# 默认安装位置是 /usr/local/musl 将其修改为 musl/install 中
```

[glibc 的编译](https://stackoverflow.com/questions/10412684/how-to-compile-my-own-glibc-c-standard-library-from-source-and-use-it)


[musl 作者对于几种主流的 C 库比较](https://news.ycombinator.com/item?id=21291893)，我从阅读的角度使用 cloc 比对行数:
musl
```txt
github.com/AlDanial/cloc v 1.82  T=0.95 s (2513.4 files/s, 193017.3 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                             1563           6612           7473          50762
C/C++ Header                   539           4513            285          33124
Assembly                       276            515            641           6532
Bourne Shell                     5            115            169            645
awk                              3             56             74            301
make                             1             68             10            159
sed                              1              0              6              9
-------------------------------------------------------------------------------
SUM:                          2389          11879           8658         162929
-------------------------------------------------------------------------------
```

glibc
```txt
github.com/AlDanial/cloc v 1.82  T=21.03 s (1028.0 files/s, 280970.3 lines/s)
---------------------------------------------------------------------------------------
Language                             files          blank        comment           code
---------------------------------------------------------------------------------------
D                                     4076         820632              0        1313609
DIET                                  2657         449605              0         721526
C                                     9249         115647         175829         682981
JSON                                     3              0              0         493775
C/C++ Header                          3217          40145          66415         369962
Assembly                              1828          39920          93537         234778
PO File                                 43          40870          55813         106389
Bourne Shell                            88           2037           2656          16058
make                                   207           2652           2642          12223
Python                                  52           1363           3812           8792
TeX                                      1            813           3697           7162
m4                                      55            355            167           3445
Windows Module Definition               18            190              0           2937
SWIG                                     3             12              0           2808
Verilog-SystemVerilog                    2              0              0           2748
awk                                     36            266            368           1890
Bourne Again Shell                       6            212            297           1850
C++                                     40            326            456           1382
Perl                                     4            103            165            830
Korn Shell                               1             55             68            435
yacc                                     1             56             36            290
Pascal                                   7             82            326            182
Expect                                   8              0              0             77
sed                                     11              8             15             70
Gencat NLS                               2              0              3             10
---------------------------------------------------------------------------------------
SUM:                                 21615        1515349         406302        3986209
---------------------------------------------------------------------------------------
```

- [x] what if kernel implement the memset too?
  - 内核确定支持，但是实现上存在，
- [x] implement brk in the baremetal environment
  - https://stackoverflow.com/questions/68123943/advantages-of-mmap-over-sbrk

当我们得到 C 库之后:
- [ ] syscallcp 如何操作

## syscall_cp_asm
在 /musl/src/thread/loongarch64/syscall_cp.s 中
- [ ] 没有定义 `__syscall_cp_asm` 啊
- [ ] 为什么需要定义

## syscall cancel
- [ ] 为什么 nanosleep 系统需要实现 syscall_cp
- [ ] 为什么 syscall_cp 的实现是和 pthread 相关的
- [ ] 为什么当 single thread 的时候 syscall_cp 可以退化为普通的

## GNU_C
才知道 `__GNU_C__` 表示为支持 GNU C 扩展啊
https://stackoverflow.com/questions/19908922/what-is-this-ifdef-gnuc-about

## wchar
实际上，wchar 和 unicode 根本不是一个东西
[Unicode 编程: C 语言描述](https://begriffs.com/posts/2019-05-23-unicode-icu.html#what-is-a-character)

## [ ] 无法理解 musl 中 math_invalid 中的实现
```c
double __math_invalid(double x) { return (x - x) / (x - x); }
```

## used
https://stackoverflow.com/questions/29545191/prevent-gcc-from-removing-an-unused-variable

```c
static inline void fp_force_evalf(float x) {
  volatile float y __attribute__((used));
  y = x;
}
```
- [ ] 不知道为什么会出现下面的错误:
```txt
[ccls 2] [W] 'used' attribute only applies to variables with non-local storage, functions, and Objective-C methods
```


## bits/loongarch/strnlen.S
strnlen.S 中为什么可以定义
```c
#ifndef ANDROID_CHANGES
#ifdef _LIBC
weak_alias (__strnlen, strnlen)
libc_hidden_def (strnlen)
libc_hidden_def (__strnlen)
#endif
#endif
```
应为 /home/loongson/ld/caiyinyu/glibc-2.28/include/libc-symbols.h 对于 weak 分别定义了两种情况。
https://begriffs.com/posts/2019-05-23-unicode-icu.html#what-is-a-character

## memset.S 中的 macro
```c
/* This is defined for the compilation of all C library code.  features.h
   tests this to avoid inclusion of stubs.h while compiling the library,
   before stubs.h has been generated.  Some library code that is shared
   with other packages also tests this symbol to see if it is being
   compiled as part of the C library.  We must define this before including
   config.h, because it makes some definitions conditional on whether libc
   itself is being compiled, or just some generator program.  */
#define _LIBC	1
```
看注释应该是编译上的一些技术!

## [ ] a_ctz_32
https://en.wikipedia.org/wiki/De_Bruijn_sequence

## sNaN and qNaN
- https://stackoverflow.com/questions/18118408/what-is-the-difference-between-quiet-nan-and-signaling-nan

## 初始化一个结构体

```c
struct A {
  int a;
  int b;
};
int main(int argc, char *argv[]) {
  struct A a = {0};
  printf("huxueshi:%s %d %d\n", __FUNCTION__, a.a, a.b);
  return 0;
}
```
## a_cas
```c
static inline int a_cas(volatile int *p, int t, int s)
{
	__asm__ __volatile__ (
		"lock ; cmpxchg %3, %1"
		: "=a"(t), "=m"(*p) : "a"(t), "r"(s) : "memory" );
	return t;
}
```
[cmpxchg 指令说明](https://hikalium.github.io/opv86/?q=cmpxchg)

1. t 被加载到寄存器 a 中
2. 指令为 `cmpxchg *p, s` // dest , source
```c
if(*p == t){
  *p = s;
}else{
  t = *p;
}
return t;
```
> Compares the value in the AL, AX, EAX, or RAX register with the first operand (destination operand). If the two
values are equal, the second operand (source operand) is loaded into the destination operand. Otherwise, the
destination operand is loaded into the AL, AX, EAX or RAX register. RAX register is available only in 64-bit mode

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
- [ ] why NULL is defined every where，虽然很容易处理，但是无法理解
- [ ] FILE 这个东西是不是定义的有点太随意了啊
- [ ] #261 需要逐个分析一下

### float_t
https://stackoverflow.com/questions/5390011/whats-the-point-of-float-t-and-when-should-it-be-used

```c
int main(int argc, char *argv[]) {
  long double a = 1;
  long double b = 1;
  long double c = a + b;
  return c;
}
```
将这个反汇编一下，就可以知道 long double 为什么需要 gcclib.a 了

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
- [x] 分析理解 stderr 和 stdout 的区别
  - 主要是 buffer 吧

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

## exit
```c
_Noreturn void exit(int code)
{
	__funcs_on_exit();
	__libc_exit_fini();
	__stdio_exit();
	_Exit(code);
}
```
1. atexit : https://stackoverflow.com/questions/25115612/whats-the-scenario-to-use-atexit-function
2. destructor: https://stackoverflow.com/questions/6477494/do-global-dtors-aux-and-do-global-ctors-aux
  - https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/Function-Attributes.html

关于 exit 和 abort 的区别:
- https://stackoverflow.com/questions/397075/what-is-the-difference-between-exit-and-abort

## locks
```c
void __unlock(volatile int *l)
{
	/* Check l[0] to see if we are multi-threaded. */
	if (l[0] < 0) {
		if (a_fetch_add(l, -(INT_MIN + 1)) != (INT_MIN + 1)) {
			__wake(l, 1, 1);
		}
	}
}
```
- [ ] 本来以为是存在 unlock 的时候，那么前面一定有 lock，实际上不是如此
  - [ ] 这个 unlock 操作没看懂

## GNU_SOURCE
- what's GNU_SOURCE : https://stackoverflow.com/questions/5582211/what-does-define-gnu-source-imply/5583764
我不知道为什么正常编译的一个程序的时候，从来不需要考虑 `_GNU_SOURCE` 的问题，但是现在使用 musl 不添加 `_GNU_SOURCE` 几乎就没有可以运行的代码了。

- https://stackoverflow.com/questions/48332332/what-does-define-posix-source-mean

## errno
我无法理解 errno 居然是架构相关的:
- [ ] 而且 signal 的部分定义 也是架构相关的

## [ ] undefined reference to fwrite
ld: x86tomips-options.c:(.text+0x534): undefined reference to `fwrite`

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

```sh
objdump -ald build_\[loongson\]_\[\]_\[\]/src/main.o > b.txt
```

## asm clobber

```c
#include <assert.h>  // assert
#include <fcntl.h>   // open
#include <limits.h>  // INT_MAX
#include <math.h>    // sqrt
#include <stdbool.h> // bool false true
#include <stdio.h>
#include <stdlib.h> // malloc sort
#include <string.h> // strcmp ..
#include <unistd.h> // sleep

#define __SYSCALL_CLOBBERS                                                     \
  "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "memory"

long hello(long arg0, long arg1, long arg2, long arg3,
                                  long arg4, long arg5, long arg6,
                                  long number) {
  // printf("hello %ld\n", number);
  return 77;
}

long __syscall0(long number) {
  long int _sys_result;

  {
    register long int __a7 asm("$a7") = number;
    register long int __a0 asm("$a0");
    __asm__ volatile("bl hello" "\n\t"
                     : "=r"(__a0)
                     : "r"(__a7)
                     : __SYSCALL_CLOBBERS);
    _sys_result = __a0;
  }
  asm(
    "move $ra, $zero \n\t"
  );
  // printf("huxueshi:%s \n", __FUNCTION__);
  return _sys_result;
}

int main(int argc, char *argv[]) {
  printf("huxueshi:%s %ld\n", __FUNCTION__, __syscall0(12));
  return 0;
}
```

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
