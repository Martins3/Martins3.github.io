# 如何处理内核性能下降的问题

## 找到内核的 rpm spec
这里描述了 513.5.1 和 513.9.1 提交的修改记录，可供参考。
- https://rockylinux.pkgs.org/8/rockylinux-devel-x86_64/kernel-4.18.0-513.5.1.el8_9.x86_64.rpm.html
- https://rockylinux.pkgs.org/8/rockylinux-baseos-x86_64/kernel-4.18.0-513.9.1.el8_9.x86_64.rpm.html

- https://git.rockylinux.org/staging/rpms/kernel

参考 SPECS/kernel.spec 可以找到构建内核的方
```txt
    %{make} -s ARCH=$Arch V=1 %{?_smp_mflags} KCFLAGS="$KCFLAGS" WITH_GCOV="%{?with_gcov}" $MakeTarget %{?sparse_mflags} %{?kernel_mflags}
    if [ $DoModules -eq 1 ]; then
	%{make} -s ARCH=$Arch V=1 %{?_smp_mflags} KCFLAGS="$KCFLAGS" WITH_GCOV="%{?with_gcov}" modules %{?sparse_mflags} || exit 1
    fi
```

找到对应的 docker 环境:
```sh
docker pull rockylinux:8
```

## 找到
67.96 你可以继续操作（但存储集群重装期间无法跑 fio 测性能）。

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
