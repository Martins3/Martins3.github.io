## audit
we can reference it in plka, is it a complex subsystem without too much mention these day ?

## seccomp
https://mp.weixin.qq.com/s/CJXuPGyOHAyBqx5pZNIZ9Q
  - https://lwn.net/Articles/824380/

Windows 程序直接调用系统调用，而不是 Window API，让 wine 没有办法进行拦截,通过 seccomp 可以
实现其中，
> TODO 还分析一下


- [ ] Read the man seccomp(2) before read linux/kernel/seccomp.c

## rlimit
include/uapi/asm-generic/resource.h
