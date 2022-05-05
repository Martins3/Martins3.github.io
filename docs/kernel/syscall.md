# syscall

## syscall 的流程
### 进入 syscall 之前 : glibc
### 进入 syscall 中

### syscall 返回

## syscall 的优化
- [ ] int 0x80 对比 fast system call, 它做了更多的工作， 是什么，为什么要做

- int 0x80 / iret
- 32 bit 下引入的 fast system call：sysenter / sysexit（自 Intel Pentium II 始），syscall / sysret（自 AMD K6 始）4
- 64 bit 下的 syscall / sysret

- [ ] 利用 glibc 确认在现在的机器上，使用的是哪一个技术
- [ ] 使用 asm 分别使用一下在机器上调用一次

### sysenter / sysexit

### vsyscall
- 将 vsyscall 映射到固定的位置为什么存在潜在的风险

```c
#include <stdio.h>
#include <time.h>

typedef time_t (*time_func)(time_t *);

int main(int argc, char *argv[]) {
  time_t tloc;
  int retval = 0;

  time_func func = (time_func)0xffffffffff600000;

  retval = func(&tloc);
  if (retval < 0) {
    perror("time_func");
    return -1;
  }
  printf("%ld\n", tloc);

  return 0;
}
```


### vdso
- [ ] x86 中 gettimeofday 在 vdso 中如何生成的

将 vdso dump 出来
https://kernel.googlesource.com/pub/scm/linux/kernel/git/luto/misc-tests/+/5655bd41ffedc002af69e3a8d1b0a168c22f2549/dump-vdso.c

## 高级话题

### syscall restart
- [ ] https://unix.stackexchange.com/questions/612506/is-it-safe-to-restart-system-calls

### ptrace & seccomp

### 32bit 系统调用

## inside
/home/maritns3/core/vn/kernel/insides/syscall.md 整理一下

## 整理这个 issue
https://github.com/Martins3/Martins3.github.io/issues/8

## 关键参考
- [ ] [x86 架构下 Linux 的系统调用与 vsyscall, vDSO](https://vvl.me/2019/06/linux-syscall-and-vsyscall-vdso-in-x86)
- [ ] https://0xax.gitbooks.io/linux-insides/content/SysCall/linux-syscall-1.html : kernel inside 分析
- [ ] https://lwn.net/Articles/604515/ : Anatomy of a system call, part 2 : 分析 32bit 系统调用
- [ ] https://www.binss.me/blog/the-analysis-of-linux-system-call/ : 分析 32bit 系统调用
- [ ] https://lwn.net/Articles/615809/ : vdso
- [ ] https://www.linuxjournal.com/content/creating-vdso-colonels-other-chicken : 甚至需要自己创建出来一个 vdso
