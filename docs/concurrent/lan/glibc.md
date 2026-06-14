# glibc

- [glibc 的编译](https://stackoverflow.com/questions/10412684/how-to-compile-my-own-glibc-c-standard-library-from-source-and-use-it)

使用 kernel.nix
```txt
mkdir build
cd build
../configure CC="gcc" CFLAGS="-O3" --prefix=$(pwd)
```

看看 glibc 的实现

例如 nptl/pthread_spin_lock.c 中的 atomic_compare_exchange_weak_acquire

### bits/loongarch/strnlen.S
`sysdeps/x86_64/strnlen.S` 中存在
```c
weak_alias (__strnlen, strnlen);
libc_hidden_builtin_def (strnlen)
```

但是，显然这是 c 语言的语法，为什么会出现在 .S 中，这是
因为 include/libc-symbols.h 对于 weak 分别定义了两种情况。

## 原来 glibc

当
```txt
$ fio
fio: symbol lookup error: /usr/lib64/ceph/libceph-common.so.0: undefined symbol: pthread_cond_clockwait, version GLIBC_2.28
```

原来 ldd 是 glibc 中的功能:
```txt
$ rpm -qf $(which ldd)
glibc-common-2.28-84.oe1.x86_64
```

glibc-common 包含的东西:
```txt
/etc/default
/etc/default/nss
/usr/bin/catchsegv
/usr/bin/gencat
/usr/bin/getconf
/usr/bin/getent
/usr/bin/iconv
/usr/bin/ldd
/usr/bin/locale
/usr/bin/localedef
/usr/bin/makedb
/usr/bin/pldd
/usr/bin/sotruss
/usr/bin/sprof
/usr/bin/tzselect
/usr/lib/build-locale-archive
/usr/sbin/zdump
/usr/sbin/zic
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
