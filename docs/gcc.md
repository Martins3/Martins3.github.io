# GCC 实战
- https://kristerw.github.io/2021/10/19/fast-math/
  - 分析关于 math 的各种优化

## gcc specs
比如
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

## 一些笔记
- [-m32](https://stackoverflow.com/questions/2426478/when-should-m32-option-of-gcc-be-used) 编译为 32bit 的操作系统的
- -march=i386 : 是用于指定指令集的
