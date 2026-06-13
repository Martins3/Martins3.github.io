## 也许我们对于 streamconfig 的使用是错误的

streamconfig 只是适合做减法，应该首先才系统拷贝一个过来
make localmodconfig 。

现在的做法，只能说，想到于对于内核的掌控更加强吧。

## make oldconfig 的作用

解决源码变化，.config 没有变化的情况

```txt
make oldconfig
*
* Restart config...
*
*
* Virtualization
*
Virtualization (VIRTUALIZATION) [Y/n/?] (NEW) Y
  Kernel-based Virtual Machine (KVM) support (KVM) [N/m/y/?] (NEW) m
    Compile KVM with -Werror (KVM_WERROR) [N/y/?] (NEW) y
    Enable support for KVM software-protected VMs (KVM_SW_PROTECTED_VM) [N/y/?] (NEW) y
    KVM for Intel (and compatible) processors support (KVM_INTEL) [N/m/?] (NEW) m
      Check that guests do not receive #VE exceptions (KVM_INTEL_PROVE_VE) [N/y/?] (NEW) y
    KVM for AMD processors support (KVM_AMD) [N/m/?] (NEW) m
    System Management Mode emulation (KVM_SMM) [Y/n/?] (NEW) Y
    Support for Microsoft Hyper-V emulation (KVM_HYPERV) [Y/n/?] (NEW) Y
    Support for Xen hypercall interface (KVM_XEN) [N/y/?] (NEW)
    Prove KVM MMU correctness (KVM_PROVE_MMU) [N/y/?] (NEW)
  Maximum number of vCPUs per KVM guest (KVM_MAX_NR_VCPUS) [1024] (NEW)
#
# configuration written to .config
```

尤其是，conf 可以加载整个配置空间，来分析

## make V=1 menuconfig 的调试

## 看看 make menuconfig
make V=1 menuconfig 展开的结果是这个

```txt
make --no-print-directory -C /home/martins3/data/kernel/linux-build -f /home/martins3/data/kernel/linux-build/Makefile menuconfig
make -f ./scripts/Makefile.build obj=scripts/basic
make -f ./scripts/Makefile.build obj=scripts/kconfig menuconfig
scripts/kconfig/mconf  Kconfig
```
而且这个很神奇，如何将 make menuconfig 转换为下一个 make file 的
  - 通过 scripts/Makefile.build
的确无法直接运行，似乎丢失了很多上下文

不知道为什么mconf 的结果在 ./scripts/kconfig/mconf-cflags

然后就可以，使用这个来调试
gdb --args ./scripts/kconfig/mconf Kconfig

make --no-print-directory -C /home/martins3/data/kernel/linux-build -f /home/martins3/data/kernel/linux-build/Makefile build_test

替换使用 remake 也是极好的

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
