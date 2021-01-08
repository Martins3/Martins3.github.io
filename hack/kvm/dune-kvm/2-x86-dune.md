# x86 Dune 

<!-- vim-markdown-toc GitLab -->

- [Bugs](#bugs)
  - [Debug 1](#debug-1)
  - [Debug 2](#debug-2)
  - [问题](#问题)
  - [wedge 的代码分析](#wedge-的代码分析)
  - [sandbox](#sandbox)
- [ref](#ref)

<!-- vim-markdown-toc -->
# Bugs

## Debug 1
为了解决 sched_getcpu 这一个 bug，将 setup_gdt 放到了 create_percpu 中间

```
static struct dune_percpu *create_percpu(void)

  // setup_gdt(percpu);
  // setup_getcpu(percpu);
```
导致 dune_boot 的代码 ltr 出现 vmexit 的 triple fault.

而 thread_local 的理解是存在问题的，lpthread 在 parent 中间初始化一次之后，children 观测的数值也是被修改后的。
lpercpu 的赋值显示，lpercpu 一旦 *dune init* dune_enter 之后，之后任何的 children 都是直接 enter 的。

由于 percpu 的 mmap 分配是 *dune init* 决定的，其实 fork 之后，每一个人都在相同的地址上，拥有自己的 percpu, 而 thread_local 的 lpercpu 每一个 thread 都是持有相同的数值。

共享地址空间 和 不共享地址空间意味着 ?

guest 的空间映射的两种情况:
1. 使用 sthread 方案，每一个 process 都有自己的 page table.
  - 在大多数的情况下保持一致，只有 writable stack tag 重新映射
2. 新的进程只是简单的使用 dune_enter 来处理，由于新进入的利用 map_stack 将自己 stack 添加进去，其余的地址空间不变
  - 无论是否共享地址空间，在 dune 中间的 process 都是无感知，正确的

两者都没有 page table 的竞争问题。

现在的问题是，其实没有任何的数值变化啊!


```c
// now 1100
d7008b 66108 c0067
d70089 66108 c0067
// std 1001
```

使用 debug_setup_gdt 来替换在 dune_boot 中间的 vcpu 那么就可以了.

```c
static void debug_setup_gdt(struct dune_percpu *percpu){
	percpu->gdt[GD_TSS >> 3] = (SEG_TSSA | SEG_P | SEG_A |
				    SEG_BASELO(&percpu->tss) |
				    SEG_LIM(sizeof(struct Tss) - 1));
}
```


## Debug 2

其实 sched_getcpu 的修复方法有问题，其前提是，这种今天修改 gdt 的方法，无法保证 cpu 进行了迁移。

## 问题
- [ ] dune 可以捕获从 gva 到 gpa 之间的 page fault 吗 ?
  - yes, but doesn't work normally yet.
- [ ] 为什么一定需要 static link glibc 才可以
- [ ] 可以使用 gdb 调试在 dune 中间运行的程序吗 ?
  - yes, but in a limited way


## wedge 的代码分析
- [ ] 似乎仅仅分析了其中的 test.c 的入口

test.c

- main
  - sthread_init
    - checkpoint_prepare : 从 /proc/self/mem 中间收集收集各个 vma 的信息，构建 `_segments`
    - dune_init_and_enter : **进入 dune**
    - checkpoint_do : 将其中可写的空间全部放到 checkpoint_do 中间, 遍历 `_segments` 中间的可写的部分
  - test_sthread
    - launch_sthread
      - sthread_create : 如果没有可以 recycle 的
        - create_new_sthread : 从 master thread 中间拷贝一个地址空间
          - dune_vm_clone : 对于 page table tree 拷贝
          - dune_vm_page_walk + walk_protect : 对于所有位置 protect
          - dune_vm_page_walk + walk_remap : 但是对于 checkpoint 的拷贝
      - sthread_join

- [ ] checkpoint_do 的时候 walk_remap 的粒度是 4K，会不会导致 hugepage 无法使用


在现在的测试系统中间：
```c
#include <assert.h> // assert
#include <err.h>
#include <fcntl.h>
#include <limits.h>  // INT_MAX
#include <math.h>    // sqrt
#include <stdbool.h> // bool false true
#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // strcmp ..
#include <sys/mman.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h> // sleep

static void *sys_st(void *arg);

int main(int argc, char *argv[]) {
  char a[10000];
  sys_st(a);

  return 0;
}

static void *sys_st(void *arg) {
  char *buf = arg;
  int fd;
  int rc;

  fd = open("/etc/passwd", O_RDONLY);
  if (fd == -1)
    err(1, "open()");

  rc = read(fd, buf, 1023);
  if (rc <= 0)
    err(1, "read()");

  buf[rc] = 0;
  printf("%s", buf);

  close(fd);

  return NULL;
}
```

glibc 对于 open 函数的封装，实际上调用的是 openat syscall
```
➜  c strace ./open-openat.out
execve("./open-openat.out", ["./open-openat.out"], 0x7fff52b75d70 /* 80 vars */) = 0
brk(NULL)                               = 0x11f3000
arch_prctl(0x3001 /* ARCH_??? */, 0x7ffc040dac90) = -1 EINVAL (Invalid argument)
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=108838, ...}) = 0
mmap(NULL, 108838, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7fb7da221000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\360q\2\0\0\0\0\0"..., 832) = 832
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
pread64(3, "\4\0\0\0\20\0\0\0\5\0\0\0GNU\0\2\0\0\300\4\0\0\0\3\0\0\0\0\0\0\0", 32, 848) = 32
pread64(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0\363\377?\332\200\270\27\304d\245n\355Y\377\t\334"..., 68, 880) = 68
fstat(3, {st_mode=S_IFREG|0755, st_size=2029224, ...}) = 0
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7fb7da21f000
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
pread64(3, "\4\0\0\0\20\0\0\0\5\0\0\0GNU\0\2\0\0\300\4\0\0\0\3\0\0\0\0\0\0\0", 32, 848) = 32
pread64(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0\363\377?\332\200\270\27\304d\245n\355Y\377\t\334"..., 68, 880) = 68
mmap(NULL, 2036952, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7fb7da02d000
mprotect(0x7fb7da052000, 1847296, PROT_NONE) = 0
mmap(0x7fb7da052000, 1540096, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x25000) = 0x7fb7da052000
mmap(0x7fb7da1ca000, 303104, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x19d000) = 0x7fb7da1ca000
mmap(0x7fb7da215000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1e7000) = 0x7fb7da215000
mmap(0x7fb7da21b000, 13528, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7fb7da21b000
close(3)                                = 0
arch_prctl(ARCH_SET_FS, 0x7fb7da220540) = 0
mprotect(0x7fb7da215000, 12288, PROT_READ) = 0
mprotect(0x403000, 4096, PROT_READ)     = 0
mprotect(0x7fb7da269000, 4096, PROT_READ) = 0
munmap(0x7fb7da221000, 108838)          = 0
openat(AT_FDCWD, "/etc/passwd", O_RDONLY) = 3
read(3, "root:x:0:0:root:/root:/bin/bash\n"..., 1023) = 1023
fstat(1, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x5), ...}) = 0
brk(NULL)                               = 0x11f3000
brk(0x1214000)                          = 0x1214000
write(1, "root:x:0:0:root:/root:/bin/bash\n", 32root:x:0:0:root:/root:/bin/bash
) = 32
write(1, "daemon:x:1:1:daemon:/usr/sbin:/u"..., 48daemon:x:1:1:daemon:/usr/sbin:/usr/sbin/nologin
) = 48
write(1, "bin:x:2:2:bin:/bin:/usr/sbin/nol"..., 37bin:x:2:2:bin:/bin:/usr/sbin/nologin
) = 37
write(1, "sys:x:3:3:sys:/dev:/usr/sbin/nol"..., 37sys:x:3:3:sys:/dev:/usr/sbin/nologin
) = 37
write(1, "sync:x:4:65534:sync:/bin:/bin/sy"..., 35sync:x:4:65534:sync:/bin:/bin/sync
) = 35
write(1, "games:x:5:60:games:/usr/games:/u"..., 48games:x:5:60:games:/usr/games:/usr/sbin/nologin
) = 48
write(1, "man:x:6:12:man:/var/cache/man:/u"..., 48man:x:6:12:man:/var/cache/man:/usr/sbin/nologin
) = 48
write(1, "lp:x:7:7:lp:/var/spool/lpd:/usr/"..., 45lp:x:7:7:lp:/var/spool/lpd:/usr/sbin/nologin
) = 45
write(1, "mail:x:8:8:mail:/var/mail:/usr/s"..., 44mail:x:8:8:mail:/var/mail:/usr/sbin/nologin
) = 44
write(1, "news:x:9:9:news:/var/spool/news:"..., 50news:x:9:9:news:/var/spool/news:/usr/sbin/nologin
) = 50
write(1, "uucp:x:10:10:uucp:/var/spool/uuc"..., 52uucp:x:10:10:uucp:/var/spool/uucp:/usr/sbin/nologin
) = 52
write(1, "proxy:x:13:13:proxy:/bin:/usr/sb"..., 43proxy:x:13:13:proxy:/bin:/usr/sbin/nologin
) = 43
write(1, "www-data:x:33:33:www-data:/var/w"..., 53www-data:x:33:33:www-data:/var/www:/usr/sbin/nologin
) = 53
write(1, "backup:x:34:34:backup:/var/backu"..., 53backup:x:34:34:backup:/var/backups:/usr/sbin/nologin
) = 53
write(1, "list:x:38:38:Mailing List Manage"..., 62list:x:38:38:Mailing List Manager:/var/list:/usr/sbin/nologin
) = 62
write(1, "irc:x:39:39:ircd:/var/run/ircd:/"..., 49irc:x:39:39:ircd:/var/run/ircd:/usr/sbin/nologin
) = 49
write(1, "gnats:x:41:41:Gnats Bug-Reportin"..., 82gnats:x:41:41:Gnats Bug-Reporting System (admin):/var/lib/gnats:/usr/sbin/nologin
) = 82
write(1, "nobody:x:65534:65534:nobody:/non"..., 59nobody:x:65534:65534:nobody:/nonexistent:/usr/sbin/nologin
) = 59
write(1, "systemd-network:x:100:102:system"..., 87systemd-network:x:100:102:systemd Network Management,,,:/run/systemd:/usr/sbin/nologin
) = 87
close(3)                                = 0
write(1, "systemd-resolve:x:101:103:system"..., 59systemd-resolve:x:101:103:systemd Resolver,,,:/run/systemd:) = 59
exit_group(0)                           = ?
+++ exited with 0 +++
```

## sandbox
- 只有利用 sandbox 的 pthread 才可以在 dune 的基础上 multiprocess

- syscall_handler
  - syscall_allow
    - `_syscall_monitor` : boxer_register_syscall_monitor
  - syscall_do
    - syscall_do_foreal
      - dune_clone
        - dune_pthread_create

# ref
https://wiki.osdev.org/Paging

```
Bit 0 (P) is the Present flag.
Bit 1 (R/W) is the Read/Write flag.
Bit 2 (U/S) is the User/Supervisor flag.
```

The combination of these flags specify the details of the page fault and indicate what action to take:

```
US RW  P - Description
0  0  0 - Supervisory process tried to read a non-present page entry
0  0  1 - Supervisory process tried to read a page and caused a protection fault
0  1  0 - Supervisory process tried to write to a non-present page entry
0  1  1 - Supervisory process tried to write a page and caused a protection fault
1  0  0 - User process tried to read a non-present page entry
1  0  1 - User process tried to read a page and caused a protection fault
1  1  0 - User process tried to write to a non-present page entry
1  1  1 - User process tried to write a page and caused a protection fault
```
