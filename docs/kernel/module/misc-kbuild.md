# 内核的构建系统

## 基本的构建流程

1. .config

## 文档阅读

### [ ]  https://stackoverflow.com/questions/29231876/how-does-kbuild-actually-work

### https://docs.kernel.org/kbuild/makefiles.html
> scripts/Makefile.* contains all the definitions/rules etc. that are used to build the kernel based on the kbuild makefiles.

- [ ] scripts/Makefile :

> Kbuild compiles all the $(obj-y) files. It then calls “$(AR) rcSTP” to merge these files into one built-in.a file. This is a thin archive without a symbol table. It will be later linked into vmlinux by scripts/link-vmlinux.sh

> Link order is significant, because certain functions (module_init() / `__initcall`) will be called during boot in the order they appear.

### https://qemu.readthedocs.io/en/latest/devel/kconfig.html
分析 QEMU 的 Kconfig 语言。

## 迷茫的语法
- always-y
- subdir

## 如何编译 out-of-tree 的内核模块
- [ ] 为什么需要指定 build 目录

## 各种小问题

### make compile_commands.json 是如何生成的
https://stackoverflow.com/questions/23774582/in-kernel-makefile-call-cmd-tags-what-is-the-cmd-here-refers-to

```txt
🧀  rm compile_commands.json && make compile_commands.json -j
  DESCEND objtool
  CALL    scripts/checksyscalls.sh
  GEN     compile_commands.json
linux on  master [!+?] via C v11.3.0-gcc via ❄️  impure (kernel)
🧀  rm compile_commands.json && V=1 make compile_commands.json -j
mkdir -p /home/martins3/core/linux/tools/objtool && make O=/home/martins3/core/linux subdir=tools/objtool --no-print-directory -C objtool
make -f /home/martins3/core/linux/tools/build/Makefile.build dir=. obj=fixdep
make -f /home/martins3/core/linux/tools/build/Makefile.build dir=./arch/x86 obj=objtool
  CALL    scripts/checksyscalls.sh
make -f /home/martins3/core/linux/tools/build/Makefile.build dir=. obj=fixdep
  GEN     compile_commands.json
```

为什么可以自动将 cmd 忽视掉
```txt
quiet_cmd_gen_good = GEN     $@
      cmd_gen_good = echo "good"
```

- [ ] 3.12 Command change detection : 进一步的补充说明，但是不是本质。
  - https://docs.kernel.org/kbuild/makefiles.html

```sh
quiet_cmd_tags = GEN     $@
      cmd_tags = $(BASH) $(srctree)/scripts/tags.sh $@

tags TAGS cscope gtags: FORCE
	$(call cmd,tags)
```


```sh
make -p | grep -B1 -E '^cmd '
```
得到
```c
cmd = @set -e; $(echo-cmd) $($(quiet)redirect) $(delete-on-interrupt) $(cmd_$(1))
```
其实就是 echo 命令，并且执行命令，stackoverflow 上总结的很对

### What are the `some_name.o.cmd` files?
- https://unix.stackexchange.com/questions/186577/what-are-the-some-name-o-cmd-files

### Makefile 和 Kbuild 是什么关系?

- 暂时只是知道，会首先选择 Kbuild 的，而且 Kbuild 的语法和 Makefile 相同的。

# build system
[各种 make defconfig 生成的过程 .config 的过程是什么?](https://stackoverflow.com/questions/41885015/what-exactly-does-linux-kernels-make-defconfig-do)
简单来说，每一个 config 项都是默认项目的，如果在 /arch/x86/configs/x86_64_defconfig 中间存在这个选项，那么就使用该选项，否则使用默认选项。

[make olddefconfig 的作用](https://lore.kernel.org/patchwork/patch/267098/)
将想要的选项放到 .config，比如 virtio 的，然后将 make olddefconfig ，其其余的选项都是自动采用默认选项.
这里存在一个很诡异的地方是 : 在 .config 放下面的语句, 会让配置变为 32bit x86
```plain
# CONFIG_64BIT is not set
```

- [ ] CONFIG_VIRTIO_BLK 之类的存在依赖，make olddefconfig 可以自动处理吗?
  - [ ] 比如 B 依赖 A, 如果 CONFIG_B=Y, 那么 CONFIG_A=Y 会被自动配置
  - [ ] 比如 B 依赖 A, C 要求 A 不能打开，同时配置 CONFIG_B=Y CONFIG_C=Y 会怎么样?


[vmlinux bzImage zImage 的关系是什么?](https://unix.stackexchange.com/questions/5518/what-is-the-difference-between-the-following-kernel-makefile-terms-vmlinux-vml)
1. vmlinux 将内核编译为静态的 ELF 格式，可以用于调试
2. vmlinux.bin : 将 vmlinux 中的所有符号和重定向信息去掉
3. vmlinuz : 压缩版本
4. zImage 和 bzImage : By adding further boot and decompression capabilities to vmlinuz, the image can be used to boot a system with the vmlinux kernel.
  - 其中 zImage 和 bzImage 在于 b 也就是 big，大小是否大于 512KB


- [ ] 通过这种方法了解一下 Kconfig 的使用方法 : 在内核的 source tree 中间，添加一个 hello world 的程序，然后加以编译执行。
- [ ]  如果存在部分 module 是单独分开安装的，那么，在 Ubuntu 的 img 重新指定任意的版本的内核就应该是不可能的事情了
## make modules

- [^2] make menuconfig 存在的两个框框 [] <>，前者只能选择为 y 或者 n，后者还多出了一个 m，在 .config 中间也是存在对应的描述 =y =m，被注释掉


## compiler
https://lwn.net/Articles/512548/ : 函数前 `__visible` 的作用

## 有没有类似这种 config
- kernel/configs/hardening.config

这是基于同一个问题吗? https://github.com/a13xp0p0v/kernel-hardening-checker

[^1]: https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html
[^2]: https://unix.stackexchange.com/questions/20864/what-happens-in-each-step-of-the-linux-kernel-building-process

## [linux 的编译系统](https://www.linuxjournal.com/content/kbuild-linux-kernel-build-system)

查资料查到了，但是有必要将内核的编译，链接，装载过程搞清楚

## 内核的 build 系统到底如何工作的
1. modules.buildin 文件是做什么用的
2. autoconf.h 是如何生成的 ?
3. scripts 文件夹下有大量好用的工具，比如生成 tag　的 tags.sh

## 一个项目，将 kernel 中的构建系统专门扣出来
https://github.com/netoptimizer/prototype-kernel

## d2lang 是一个好东西
https://github.com/ravsii/tree-sitter-d2?tab=readme-ov-file

## -kernel vmlinux 可以启动虚拟机吗?

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
