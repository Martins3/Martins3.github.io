# 阅读 musl 学到的一些东西

- [官方网站](https://www.musl-libc.org/)

<!-- vim-markdown-toc GitLab -->

* [准备](#准备)
* [Makefile](#makefile)
* [memset 的实现和内核有区别吗](#memset-的实现和内核有区别吗)
* [可以不使用 brk 来实现 malloc 吗](#可以不使用-brk-来实现-malloc-吗)
* [`syscall_cp`](#syscall_cp)
* [`__GNU_C__`](#__gnu_c__)
* [wchar](#wchar)
* [gcc attribute](#gcc-attribute)
* [sNaN and qNaN](#snan-and-qnan)
* [初始化结构体](#初始化结构体)
* [头文件 include 是存在优先级的](#头文件-include-是存在优先级的)
* [[ ] hidden 的作用](#-hidden-的作用)
* [[ ] stdint / limits.h / inttypes.h / stdbool](#-stdint-limitsh-inttypesh-stdbool)
  * [`float_t`和 long double](#float_t-和-long-double)
* [redzone](#redzone)
* [malloc](#malloc)
  * [多线程的处理](#多线程的处理)
  * [`week_alias`和静态链接](#week_alias-和静态链接)
  * [`a_ctz_32`](#a_ctz_32)
* [fabs 的定义](#fabs-的定义)
* [当 exit 的时候会发生什么](#当-exit-的时候会发生什么)
* [[ ] musl 如何实现 locks](#-musl-如何实现-locks)
* [`GNU_SOURCE`](#gnu_source)
* [gcc can optimize fprintf to fwrite](#gcc-can-optimize-fprintf-to-fwrite)
* [crt0](#crt0)
* [glibc](#glibc)
  * [bits/loongarch/strnlen.S](#bitsloongarchstrnlens)
* [TODO](#todo)
* [一些奇怪的事情](#一些奇怪的事情)

<!-- vim-markdown-toc -->

## 准备
```sh
git clone https://github.com/bminor/musl
cd musl
mkdir install
./configure --enable-debug --prefix=$(pwd)/install
# 默认安装位置是 /usr/local/musl 将其修改为 musl/install 中
```

- [glibc 的编译](https://stackoverflow.com/questions/10412684/how-to-compile-my-own-glibc-c-standard-library-from-source-and-use-it)

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
但是总体来说，C 库在 Linux 的复杂度面前，还是一个小学生。

## Makefile
实际上，musl 的 Makefile 实际上非常值得一读，可以掌握一些整个工程的架构的。

```mk
obj/crt/Scrt1.o obj/crt/rcrt1.o: CFLAGS_ALL += -fPIC
```

## memset 的实现和内核有区别吗
有，比如，amd64 的实现在 arch/x86/lib/memset_64.S 中

实际上，memset 的实现非常有门道，我们只是需要简单看看 musl 和 glibc 的对比就可以看出来 glibc 的实现要更加高效。
[Fast Memset and Memcpy implementations](https://github.com/nadavrot/memset_benchmark) 中仔细的对比了各种实现。

## 可以不使用 brk 来实现 malloc 吗
可以的。
- musl 用 brk 分配 meta data
- glibc 普通的数据也会使用 brk 分配

总所周知[^1]，brk 实际上是一个简化版的 mmap, 只是调整一个 vma 的上边界，而这个 vma 下边界就是 bss section 结束的位置。
所以 brk 应该比 mmap 更加快一点点。

## `syscall_cp`

将 `src/time/clock_nanosleep.c` 展开为
```c
int __clock_nanosleep(clockid_t clk, int flags, const struct timespec *req, struct timespec *rem)
{
 if (clk == 3) return 22;
 if (clk == 0 && !flags)
  return -(__syscall_cp)(35,((long) (req)),((long) (rem)),0,0,0,0);
 return -(__syscall_cp)(230,((long) (clk)),((long) (flags)),((long) (req)),((long) (rem)),0,0);
}
```

`src/thread/__syscall_cp.c` 在中，通过 `week_alias` 告诉我们当 single thread 的时候 `syscall_cp` 可以退化为普通的 syscall 的。
```c
long __syscall_cp_c(syscall_arg_t nr,
                    syscall_arg_t u, syscall_arg_t v, syscall_arg_t w,
                    syscall_arg_t x, syscall_arg_t y, syscall_arg_t z)
{
  pthread_t self;
  long r;
  int st;

  if ((st=(self=__pthread_self())->canceldisable)
      && (st==PTHREAD_CANCEL_DISABLE || nr==SYS_close))
    return __syscall(nr, u, v, w, x, y, z);

  r = __syscall_cp_asm(&self->cancel, nr, u, v, w, x, y, z);
  if (r==-EINTR && nr!=SYS_close && self->cancel &&
      self->canceldisable != PTHREAD_CANCEL_DISABLE)
    r = __cancel();
  return r;
}
```

在 `__syscall_cp_c` 中，只是将 `self->cancel` 传递到 r11 上了

`src/thread/x86_64/syscall_cp.s` 中:

```asm
.text
.global __cp_begin
.hidden __cp_begin
.global __cp_end
.hidden __cp_end
.global __cp_cancel
.hidden __cp_cancel
.hidden __cancel
.global __syscall_cp_asm
.hidden __syscall_cp_asm
.type   __syscall_cp_asm,@function
__syscall_cp_asm:

__cp_begin:
  mov (%rdi),%eax
  test %eax,%eax
  jnz __cp_cancel
  mov %rdi,%r11
  mov %rsi,%rax
  mov %rdx,%rdi
  mov %rcx,%rsi
  mov %r8,%rdx
  mov %r9,%r10
  mov 8(%rsp),%r8
  mov 16(%rsp),%r9
  mov %r11,8(%rsp)
  syscall
__cp_end:
  ret
__cp_cancel:
  jmp __cancel
```

对比 glibc 的 syscall 是一个很好的理解 x86 syscall 装换:
```asm
ENTRY (syscall)
  movq %rdi, %rax   /* Syscall number -> rax.  */
  movq %rsi, %rdi   /* shift arg1 - arg5.  */
  movq %rdx, %rsi
  movq %rcx, %rdx
  movq %r8, %r10
  movq %r9, %r8
  movq 8(%rsp),%r9  /* arg6 is on the stack.  */
  syscall     /* Do the system call.  */
  cmpq $-4095, %rax /* Check %rax for error.  */
  jae SYSCALL_ERROR_LABEL /* Jump to error handler if error.  */
  ret     /* Return to caller.  */

PSEUDO_END (syscall)
```

参考 kernel inside[^2] 可以找到内核 r11 保存到 `pt_regs->flags`:
```S
  pushq %r11          /* pt_regs->flags */
```

在 lwn 的 [This is why we can't have safe cancellation points](https://lwn.net/Articles/683118/) 有更多的分析。

- [ ] https://tutorialspoint.dev/language/c/pthread_cancel-c-example

## `__GNU_C__`
才知道 `__GNU_C__` [表示为支持 GNU C 扩展](https://stackoverflow.com/questions/19908922/what-is-this-ifdef-gnuc-about)

## wchar
实际上，wchar 和 unicode 根本不是一个东西
[Unicode 编程: C 语言描述](https://begriffs.com/posts/2019-05-23-unicode-icu.html#what-is-a-character)

## gcc attribute
gcc 的一些优化可能将一些函数直接优化掉，可以通过 used 来告诉 gcc 这个符号的确是需要的:
- https://stackoverflow.com/questions/29545191/prevent-gcc-from-removing-an-unused-variable
- https://stackoverflow.com/questions/31637626/whats-the-usecase-of-gccs-used-attribute

使用 -Wall 会导致让一些没有使用的符号被警告，使用 unused 可以告诉 gcc 我知道，请不要警告。

```c
static inline void fp_force_evalf(float x) {
  volatile float y __attribute__((used));
  y = x;
}
```

```txt
[ccls 2] [W] 'used' attribute only applies to variables with non-local storage, functions, and Objective-C methods
```

## sNaN and qNaN
- https://stackoverflow.com/questions/18118408/what-is-the-difference-between-quiet-nan-and-signaling-nan

## 初始化结构体

```c
#include <assert.h>
#include <stdio.h>

struct A {
  int a;
  int b;
  int x[1000];
};
int main(int argc, char *argv[]) {
  struct A a = {0};     // 直接所有的成员初始化为 0
  struct A b = {.a = 1}; // 除了 .a = 1 之外，其余全部都是等于 0
  for (int i = 0; i < 1000; ++i) {
    assert(a.x[i] + b.x[i] == 0);
  }
  return 0;
}
```

## 头文件 include 是存在优先级的
如果工程中，同时存在
- src/include/time.h
- include/time.h

因为在 Makefile 中
```c
CFLAGS_ALL += -D_XOPEN_SOURCE=700 -I$(srcdir)/arch/$(ARCH) -I$(srcdir)/arch/generic -Iobj/src/internal -I$(srcdir)/src/include -I$(srcdir)/src/internal -Iobj/include -I$(srcdir)/include
```
src/include 定义在前，所以优先级更高。

## [ ] hidden 的作用

## [ ] stdint / limits.h / inttypes.h / stdbool
- [ ] answer this nice questions : musl 为什么需要动态的生成 bits/alltypes.h

```c
obj/include/bits/alltypes.h: $(srcdir)/arch/$(ARCH)/bits/alltypes.h.in $(srcdir)/include/alltypes.h.in $(srcdir)/tools/mkalltypes.sed
  sed -f $(srcdir)/tools/mkalltypes.sed $(srcdir)/arch/$(ARCH)/bits/alltypes.h.in $(srcdir)/include/alltypes.h.in > $@
```
- [ ] why NULL is defined every where，虽然很容易处理，但是无法理解
- [ ] FILE 这个东西是不是定义的有点太随意了啊
- [ ] #261 需要逐个分析一下

### `float_t` 和 long double
[`float_t`](https://stackoverflow.com/questions/5390011/whats-the-point-of-float-t-and-when-should-it-be-used)

```c
int main(int argc, char *argv[]) {
  long double a = 1;
  long double b = 1;
  long double c = a + b;
  return c;
}
```
如果上
将这个反汇编一下，就可以知道 long double 为什么需要 gcclib.a 了

## redzone
redzone 是
关于 redzone 的解释:
- https://os.phil-opp.com/red-zone/
- https://eli.thegreenplace.net/2011/09/06/stack-frame-layout-on-x86-64

## malloc
在这两个位置讨论分析了一下 malloc 的实现
- https://musl.openwall.narkive.com/J9ymcXt2/what-s-wrong-with-musl-s-malloc
- https://news.ycombinator.com/item?id=23080290

### 多线程的处理
- [有人说](https://stackoverflow.com/questions/855763/is-malloc-thread-safe) 中的人都说 malloc 总是安全的，但是似乎只是在 pthread 的时候是安全的
```c
#define MT (libc.need_locks)
```
当共享地址空间的时候，才需要上锁的

而且我们构建了一个[clone.c](./malloc_thread_safe/clone.c) [pthread.c](./malloc_thread_safe/pthread.c)

### `week_alias` 和静态链接
在分析 malloc 的原理的时候，我发现当在静态链接的时候，使用 free 与否会导致实际上调用的 malloc 不同
- https://stackoverflow.com/questions/23079997/override-weak-symbols-in-static-library
- https://stackoverflow.com/questions/51656838/attribute-weak-and-static-libraries

其中的原理我们使用了[一个例子](./weak_alias) 来阐述

### `a_ctz_32`
- https://en.wikipedia.org/wiki/De_Bruijn_sequence
- https://www.cnblogs.com/brighthoo/p/10649588.html


## fabs 的定义

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
这个代码编译之后，反汇编之后，我才意识到 fabs 是 gcc 的 built-in 函数，可以直接生成的对应的指令。

## 当 exit 的时候会发生什么
```c
_Noreturn void exit(int code)
{
  __funcs_on_exit();
  __libc_exit_fini();
  __stdio_exit();
  _Exit(code);
}
```
1. [atexit 注册的 hook 执行](https://stackoverflow.com/questions/25115612/whats-the-scenario-to-use-atexit-function)
2. [destructor 被执行](https://stackoverflow.com/questions/6477494/do-global-dtors-aux-and-do-global-ctors-aux)
  - https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/Function-Attributes.html

[exit 和 abort 的区别](https://stackoverflow.com/questions/397075/what-is-the-difference-between-exit-and-abort)
- exit 会执行上面的两种 hook，但是 abort 不会
- abort 是通过发送 SIGABRT 信号的，这意味着你实际上可以给这个信号注册 handler 的

## [ ] musl 如何实现 locks
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

static volatile int lock[1];

static inline void __wake(volatile void *addr, int cnt, int priv)
{
  if (priv) priv = FUTEX_PRIVATE;
  if (cnt<0) cnt = INT_MAX;
  __syscall(SYS_futex, addr, FUTEX_WAKE|priv, cnt) != -ENOSYS ||
  __syscall(SYS_futex, addr, FUTEX_WAKE, cnt);
}
```

## `GNU_SOURCE`
- what's `GNU_SOURCE` : https://stackoverflow.com/questions/5582211/what-does-define-gnu-source-imply/5583764
我不知道为什么正常编译的一个程序的时候，从来不需要考虑 `_GNU_SOURCE` 的问题，但是现在使用 musl 不添加 `_GNU_SOURCE` 几乎就没有可以运行的代码了。

- https://stackoverflow.com/questions/48332332/what-does-define-posix-source-mean

## gcc can optimize fprintf to fwrite

## crt0
- crt/crt1.c : 静态链接的时候使用
- crt/rcrt1.c : 动态链接的时候使用

在 `src/env/__libc_start_main.c` 可以分析出来 [Auxiliary Vector](https://www.gnu.org/software/libc/manual/html_node/Auxiliary-Vector.html) 是如何
在用户态如何构建的。

## glibc

### bits/loongarch/strnlen.S
`sysdeps/x86_64/strnlen.S` 中存在
```c
weak_alias (__strnlen, strnlen);
libc_hidden_builtin_def (strnlen)
```

但是，显然这是 c 语言的语法，为什么会出现在 .S 中，这是
因为 include/libc-symbols.h 对于 weak 分别定义了两种情况。


## TODO
- 实际上，程序员自有修养的很好的分析了一下 crt 相关的工作，值得深入分析。
- 没有在 gcc 中找到将 fprintf 优化为 fwrite 的代码
- 没有仔细分析为什么 Makefile 中处理 .s 和 .S 的区别是什么
- lock 中很多细节
  - 为什么要定义为数组
  - C 中 volatile 的作用
  - 为什么使用 `INT_MIN`
- 在 BMBT 中，我们发现，修改编译选项从 -Ofast 的时候，然后打开所有的警告
```sh
CFLAGS="-Ofast -Wall" ./configure --enable-debug --prefix=$(pwd)/install
```

可以展示这个警告:
```txt
libc/src/stdio/vfprintf.c:561:23: 警告：此函数中的‘estr’在使用前可能未初始化 [-Wmaybe-uninitialized]
     out(f, estr, ebuf - estr);
                  ~~~~~^~~~~~
```
这个警告只有在只有在 -O0 的时候是没有的哦。

- musl 的似乎是通过 spec 文件进行的，那个东西可以简单的分析一下
```sh
➜  obj git:(master) ✗ more musl-gcc
#!/bin/sh
exec "${REALGCC:-gcc}" "$@" -specs "/usr/local/musl/lib/musl-gcc.specs"
```

```txt
➜  lib git:(master) ✗ cat musl-gcc.specs
%rename cpp_options old_cpp_options

*cpp_options:
-nostdinc -isystem /usr/local/musl/include -isystem include%s %(old_cpp_options)

*cc1:
%(cc1_cpu) -nostdinc -isystem /usr/local/musl/include -isystem include%s

*link_libgcc:
-L/usr/local/musl/lib -L .%s

*libgcc:
libgcc.a%s %:if-exists(libgcc_eh.a%s)

*startfile:
%{!shared: /usr/local/musl/lib/Scrt1.o} /usr/local/musl/lib/crti.o crtbeginS.o%s

*endfile:
crtendS.o%s /usr/local/musl/lib/crtn.o

*link:
-dynamic-linker /lib/ld-musl-x86_64.so.1 -nostdlib %{shared:-shared} %{static:-static} %{rdynamic:-export-dynamic}

*esp_link:


*esp_options:


*esp_cpp_options:
```
- [ ] 其中的 startfile 和 endfile 和 crt 有关，而 crt 在程序员的自我修养的时候就没有看懂

## 一些奇怪的事情
- errno 和 signal 的编号都是和架构相关的

[^1]: https://stackoverflow.com/questions/68123943/advantages-of-mmap-over-sbrk
[^2]: https://0xax.gitbooks.io/linux-insides/content/SysCall/linux-syscall-2.html
[^3]: https://gcc.gnu.org/onlinedocs/gccint/Soft-float-library-routines.html

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
