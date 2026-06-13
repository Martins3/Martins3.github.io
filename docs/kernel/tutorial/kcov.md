# kcov
## 内核代码覆盖率的使用
- https://docs.kernel.org/dev-tools/gcov.html : 分析内核代码本身

## 用户态的 coverage 的基本使用参考:
- https://docs.kernel.org/dev-tools/kcov.html : 用户态
  - https://github.com/SimonKagstrom/kcov
```sh
gcc -fPIC -fprofile-arcs -ftest-coverage -Wall -Werror a.c
./a.out
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory out
```

## 使用 https://docs.kernel.org/dev-tools/gcov.html 中的 Appendix B : gather_on_test.sh 拷贝出来
解压到 linux/kcov 中

## 参考这个项目，基本的执行流程
https://github.com/linux-test-project/lcov/blob/master/README


```txt
🧀  lcov --capture --directory kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov --output-file coverage.info
Capturing coverage data from kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov
Found gcov version: 12.2.0
Using intermediate gcov format
Using JSON module JSON::PP
Scanning kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov for .gcda files ...
Found 2763 data files in kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov
Processing arch/x86/mm/fault.gcda
/home/martins3/core/linux/kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov/arch/x86/mm/fault.gcno:cannot open notes file
/home/martins3/core/linux/kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov/arch/x86/mm/fault.gcda:stamp mismatch with notes file
geninfo: ERROR: GCOV failed for /home/martins3/core/linux/kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov/arch/x86/mm/fault.gcda!
```

```txt
lcov --capture --directory sys/kernel/debug/gcov/home/martins3/data/kernel/linux-kcov/ --output-file coverage.info
```

需要注意 gcc 版本，如何理解?

也就是 linux-kcov 自己的环境，需要用 nix 的 linux 中就可以了:
```txt
default.nix  sys
data/kernel/tmp via impure (yyds-env) 😋
🧀  lcov --capture --directory sys/kernel/debug/gcov/home/martins3/data/kernel/linux-kcov/ --output-file coverage.info
Capturing coverage data from sys/kernel/debug/gcov/home/martins3/data/kernel/linux-kcov/
geninfo cmd: '/nix/store/f2rxnjwz73981pawqmzdbw2i3riwiv87-lcov-2.3.2/bin/geninfo sys/kernel/debug/gcov/home/martins3/data/kernel/linux-kcov/ --toolname .lcov-wrapped --output-filename coverage.info'
Found gcov version: 14.3.0
```

有一些错误，然后:
```txt
lcov --capture \
     --directory sys/kernel/debug/gcov/home/martins3/data/kernel/linux-kcov/ \
     --output-file coverage.info \
     --ignore-errors negative,inconsistent
```

然后:
```sh
genhtml coverage.info --output-directory out
```

但是还是遇到错误，而且整个过程都是很慢的。

## 这是内核的流程?
https://lpc.events/event/11/contributions/1075/attachments/819/1543/Code_Coverage_LPC_Safety_MC.pdf


## 看看这个

net/rds/Makefile 中居然有这个东西

```txt
# for GCOV coverage profiling
ifdef CONFIG_GCOV_PROFILE_RDS
GCOV_PROFILE := y
endif
```
换一句话说，可以实现仅仅一个 kernel module 的 profile 么?

## 分析需求
1. code coverage 分析一下 net 下一个虚拟机中到底会使用那些内容
2. 使用 reset 功能来看具体的功能变化

## 理论基础

• .gcno 文件：编译时生成，在 linux-kcov/ 目录中
• .gcda 文件：运行时生成，通过 debugfs 挂载在 sys/kernel/debug/gcov/... 中


在 /sys/kernel/debug/gcov/home/martins3/data/kernel/linux-kcov
```txt
.
├── arch
│   └── x86
│       ├── entry
│       │   ├── entry_fred.gcda
│       │   ├── entry_fred.gcno -> /home/martins3/data/kernel/linux-kcov/arch/x86/entry/entry_fred.gcno <- 这个在内核构建的时候，就会自动产生的
│       │   ├── syscall_64.gcda
│       │   ├── syscall_64.gcno -> /home/martins3/data/kernel/linux-kcov/arch/x86/entry/syscall_64.gcno
│       │   ├── vdso
│       │   │   ├── extable.gcda
│       │   │   ├── extable.gcno -> /home/martins3/data/kernel/linux-kcov/arch/x86/entry/vdso/extable.gcno
│       │   │   ├── vdso-image-64.gcda
│       │   │   ├── vdso-image-64.gcno -> /home/martins3/data/kernel/linux-kcov/arch/x86/entry/vdso/vdso-image-64.gcno
│       │   │   ├── vma.gcda
│       │   │   └── vma.gcno -> /home/martins3/data/kernel/linux-kcov/arch/x86/entry/vdso/vma.gcno
│       │   └── vsyscall
│       │       ├── vsyscall_64.gcda
│       │       └── vsyscall_64.gcno -> /home/martins3/data/kernel/linux-kcov/arch/x86/entry/vsyscall/vsyscall_64.gcno
│       ├── events
│       │   ├── amd
│       │   │   ├── core.gcda
│       │   │   ├── core.gcno -> /home/martins3/data/kernel/linux-kcov/arch/x86/events/amd/core.gcno
│       │   │   ├── ibs.gcda
│       │   │   ├── ibs.gcno -> /home/martins3/data/kernel/linux-kcov/arch/x86/events/amd/ibs.gcno
│       │   │   ├── iommu.gcda
│       │   │   ├── iommu.gcno -> /home/martins3/data/kernel/linux-kcov/arch/x86/events/amd/iommu.gcno
│       │   │   ├── lbr.gcda
│       │   │   ├── lbr.gcno -> /home/martins3/data/kernel/linux-kcov/arch/x86/events/amd/lbr.gcno
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
