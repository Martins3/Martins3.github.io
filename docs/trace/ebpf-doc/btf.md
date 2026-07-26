## btf
- https://www.ebpf.top/post/kernel_btf/
- https://nakryiko.com/posts/btf-dedup/

## 问题
- btf 在那里，有具体的文件吗?
  - 难道是 : /sys/kernel/btf
- 如何生成的?

## pahole
https://mp.weixin.qq.com/s/uTGRjky09PuQej566P2HVA : 似乎 btf 是从 dwarf 中生成的

https://stackoverflow.com/questions/70093863/linux-btf-bpftool-failed-to-get-ehdr-from-sys-kernel-btf-vmlinux

## vmlinux.h 如何生成的，作用如何?
参考 : https://www.grant.pizza/blog/vmlinux-header/

![](https://www.grant.pizza/libbpf/vmlinux.png)

> Since the vmlinux.h file is generated from your installed kernel, your bpf program could break if you try to run it on another machine without recompiling if it’s running a different kernel version.
> This is because, from version to version, definitions of internal structs change within the linux source code.
>
> However, by using libbpf, you can enable something called “CO:RE” or “Compile once, run everywhere”.
> There are macros defined in libbpf (such as BPF_CORE_READ) that will analyze what fields you’re trying to access in the types that are defined in your vmlinux.h.
> If the field you want to access has been moved within the struct definition that the running kernel uses,
> the macro/helpers will find it for you. It doesn’t matter if you compile your bpf program with the vmlinux.h file you generated from your own kernel and then ran on a different one.

简单来说，构建 bpf 程序的时候，需要

- [ ] 如何解决 vmlinux.h 如何在不同的 kernel 版本的，就是 CO:RE ，但是 libbpf 如何解决的，是个迷?
  - 猜测，libbpf 运行的时候，还是需要知道运行的 kernel 的 header 是什么?

## /sys/kernel/btf
https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-kernel-btf

> Contains BTF type information and related data for kernel and kernel modules.

包含了当前 kernel 的 btf 的信息 : /sys/kernel/btf

- https://github.com/aquasecurity/btfhub/blob/main/docs/how-to-use-pahole.md
  - 不明觉厉啊

```sh
bpftool btf dump file /sys/kernel/btf/vmlinux format raw
```

```sh
pahole /sys/kernel/btf/vmlinux
```
- [ ] 这个生成的结果 vmlinux.h 啥关系

### /sys/kernel/btf 难道和 vmlinux.h 中的内容不是重复的吗?

是的，在 guest 中
```sh
bpftool btf dump file /sys/kernel/btf/vmlinux format c
```
生成的结果和
```sh
pahole /sys/kernel/btf/vmlinux
```
的内容相同。

### [ ] 那么问题是，为什么 libbpf-bootstrap 一个 vmlinux.h ，不可以直接现场生成吗?


https://lwn.net/Articles/818714/


## Skipping BTF generation for virtio/virtio-dummy.ko due to unavailability of vmlinux
<!-- 3b20834f-f926-48d1-a3e1-dc2c4543a2f8 -->

在 m/ 下构建驱动的时候，总是发现这个警告:
```txt
Skipping BTF generation for virtio/virtio-dummy.ko due to unavailability of vmlinux
  BTF [M] mini/mini.ko
Skipping BTF generation for mini/mini.ko due to unavailability of vmlinux
  BTF [M] simplefs/simplefs.ko
Skipping BTF generation for simplefs/simplefs.ko due to unavailability of vmlinux
  BTF [M] martins3.ko
Skipping BTF generation for martins3.ko due to unavailability of vmlinux
```

### codex 的解答
是的，debug-btf 可以用来测试，而且我已经验证过了。

原理是这样的：外部模块最终链接成 .ko 后，如果内核配置开了
CONFIG_DEBUG_INFO_BTF_MODULES=y，Kbuild 会尝试给模块生成 BTF。逻辑在
scripts/Makefile.modfinal:41：

if [ ! -f $(objtree)/vmlinux ]; then
        printf "Skipping BTF generation ..."
else
        scripts/gen-btf.sh --btf_base $(objtree)/vmlinux $@
fi

BTF 是从 DWARF 调试信息用 pahole 生成的类型信息。模块 BTF 需要 vmlinux 作
为 --btf_base，因为模块里的很多类型来自内核本体，比如 struct module、
struct task_struct、trace/BPF 相关类型等。模块 BTF 通常不是完整复制所有内
核类型，而是生成 split BTF：模块自己的 .BTF 加上对内核基础 BTF 的引用/去重
信息，所以需要完整的、带 BTF/DWARF 上下文的 vmlinux。

你经常看到这个提示，是因为 /home/martins3/data/vn/m/Makefile 里默认：

NORMAL=1
KERNEL_SOURCE := /lib/modules/$(uname -r)/build

而发行版的 kernel-devel 目录通常有 headers、auto.conf、Module.symvers，但
没有完整 vmlinux。配置又开了 CONFIG_DEBUG_INFO_BTF_MODULES=y，于是 Kbuild
想生成模块 BTF，但找不到 $(objtree)/vmlinux，就跳过。

我在 debug-btf 里验证的可用命令是：

cd /home/martins3/data/vn/m
make clean NORMAL=0 KERNEL_SOURCE=/home/martins3/data/kernel/linux-drm
make NORMAL=0 KERNEL_SOURCE=/home/martins3/data/kernel/linux-drm \
  PAHOLE=/home/martins3/.nix-profile/bin/pahole

注意 VM 里 /usr/local/bin/pahole 是 0 字节且不可执行，所以要显式指定 nix
profile 里的 pahole。

验证结果：

readelf -S martins3.ko | grep -E '\.BTF|debug_info'

能看到：

.debug_info
.BTF
.BTF.base

并且我用 sudo 密码 a 在 VM 里做了 insmod/rmmod，模块加载和卸载都成功，
dmesg 里有 greeter_init 和 greeter_exit。debug-btf VM 已经停掉了。

这个东西怎么用：

readelf -S martins3.ko | grep BTF
bpftool btf dump file martins3.ko format raw

实际运行时你不需要手动“使用”它；模块加载时内核会解析/校验模块 BTF。它主要
服务于 BPF/CO-RE、类型 introspection、tracing/fentry 等场景。普通 insmod
测试模块功能时，带不带 BTF 通常没有行为差异。

### 内核模块有了 btf 可以有什么不同，当前系统中加载的模块是有 btf 的吗？

1. BPF 可移植性（CO-RE）：BPF 程序可以通过 BTF 了解模块里的结构体、函数签名，编译一次就能在不同内核版本上运行，而不必为 每个版本重新编译。
2. 更安全的 BPF 辅助函数使用：BPF 验证器能看到模块导出函数的真实参数类型和返回值，减少误用。
3. 更丰富的动态追踪：ftrace、perf、tracepoint、kprobe 等能利用 BTF 解析参数、结构体字段，输出更可读的火焰图和跟踪日志。
4. 调试与崩溃分析：drgn、crash、gdb 等调试工具可以借助 BTF 还原模块内部数据结构，即使没有调试符号也能定位问题。
5. 内核热补丁/实时补丁：基于 BTF 的元数据可以更安全地匹配符号和类型，降低热补丁出错风险。

(**我这里想要确认下，drgn 真的可以使用 btf 吗?**)

从输出看：
1. `/sys/kernel/btf/` 目录下包含了很多模块名命名的文件，包括
... (23 more lines, ctrl+o to expand)

当前系统是 Fedora 44，内核版本 7.0.10-201.fc44.x86_64。从
/sys/kernel/btf/ 目录可以看到：

- 有 vmlinux 文件（内核本身的 BTF）
- 还有大量以模块名命名的 BTF 文件，例如 zram、lz4_compress、vfio、vhost_net、tap、i915、kvm、xfs、xe 等等

这说明当前系统中已加载的内核模块基本都携带了 BTF。

检查方式

最直观的方式就是看 /sys/kernel/btf/：

```bash
  ls /sys/kernel/btf/
```

如果某个加载的模块在该目录下有同名文件，就表示它有 BTF。例如：

```bash
  ls -lh /sys/kernel/btf/zram /sys/kernel/btf/vmlinux
```

如果想看模块 ELF 里有没有 .BTF section，可以：

```bash
  readelf -S /lib/modules/$(uname -r)/kernel/drivers/block/zram/zram.ko.xz | grep -i btf
```

（注意 Fedora 的 ko 文件通常是 xz 压缩的，需要先 unxz -c 再传给 readelf。）

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
