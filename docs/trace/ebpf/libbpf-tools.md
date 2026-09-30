# libbpf-tools
<!-- e2d799d2-e297-45b5-91f8-e7011ce568ce -->

上游源码：<https://github.com/iovisor/bcc/tree/master/libbpf-tools>

## 基本原理

以 `opensnoop` 为例：

- `opensnoop.bpf.c`：内核态 BPF 程序，挂载 `open`、`openat`、`openat2` tracepoint。
- `opensnoop.h`：内核态和用户态共享的 event 结构。
- `opensnoop.c`：解析参数、加载和 attach、读取 event、解析符号并格式化输出。

`libbpf-tools/Makefile` 中相应的构建链是：

```txt
opensnoop.bpf.c -> opensnoop.bpf.o -> opensnoop.skel.h
                                             |
opensnoop.c + opensnoop.h + static libbpf ----+-> opensnoop
```

`bpftool` 可以单独加载简单的 `.bpf.o`，但不会替 `opensnoop.c` 解析参数、选择兼容
hook、轮询 ring/perf buffer 和格式化文件名。因此完整的 libbpf-tool 仍然需要配套的
用户态程序；它不是只有一个可随处加载的 BPF object。

## libbpf-tools 可以平替吗 bcc-tools 吗?

BCC checkout `a3dcb9a53f28` 的机械统计，不把示例、旧工具和 非 Python frontend
算进去：

| 统计项                                                          |      数量 |
| --------------------------------------------------------------- | --------: |
| `tools/*.py` 中的 BCC 工具                                      |       105 |
| `libbpf-tools/*.bpf.c` 中的 libbpf 工具                         |        57 |
| 两边名字完全相同                                                |        47 |
| 加上 `fsdist`、`fsslower` 和 `sigsnoop` 所提供的 BCC 同功能别名 | 约 59/105 |

`fsdist`、`fsslower` 是通用实现，构建系统会创建 `ext4dist`、`xfsdist`、
`nfsdist`、`ext4slower`、`xfsslower` 等别名；`sigsnoop` 也会提供 `killsnoop`
别名。因此只比较同名文件会低估覆盖率。不过即使按功能别名计算，也只有约 56%，
远没有一一对应。这个比例只是目录覆盖率，不代表功能或参数已经完全等价。

### 已覆盖得比较好的领域

- 进程与文件：`execsnoop`、`exitsnoop`、`opensnoop`、`filelife`、`filetop`、
  `mountsnoop`、`statsnoop`、`syncsnoop`。
- 块 I/O：`biolatency`、`biopattern`、`biosnoop`、`biotop`、`bitesize`、
  `biostacks`。
- 调度与 CPU：`offcputime`、`runqlat`、`runqlen`、`runqslower`、`profile`、
  `cpudist`、`hardirqs`、`softirqs`。
- TCP：`tcpconnect`、`tcpconnlat`、`tcplife`、`tcprtt`、`tcpstates`、
  `tcpsynbl`、`tcptop`、`tcptracer`。

### 仍只在 BCC 一侧比较突出的工具

- 通用动态跟踪：`trace`、`argdist`、`funccount`、`funcinterval`、
  `funcslower`、`stackcount`、`tplist`。
- 网络：`tcpaccept`、`tcpcong`、`tcpdrop`、`tcpretrans`、`tcpsubnet`、
  `netqtop`、`rdmaucma`。
- 调度、虚拟化与系统：`offwaketime`、`kvmexit`、`virtiostat`、`wqlat`、
  `cpuunclaimed`、`criticalstat`。
- 应用和协议：`dbslower`、`dbstat`、`mysqld_qslower`、`sslsniff`。
- 文件系统和内存：`cachetop`、`compactsnoop`、`dirtop`、`swapin`、
  `filegone`、`fileslower`。

同名工具也可能有参数差距。例如当前 BCC `execsnoop` 有 PPID、CPU、父进程名等
过滤或输出选项，而本地 libbpf 版本没有完全相同的参数集合。替换生产脚本前必须分别
查看两个版本的 `--help`，不能只替换可执行文件路径。

## 本地构建

https://github.com/iovisor/bcc/tree/master/libbpf-tools
	- 这里提供了一些有用的链接的:

ubuntu 中可以 libbpf-tools ，但是 nixos 中没有
```txt
sudo apt install libbpf-tools
```


```bash
cd /home/martins3/data/bcc
git submodule update --init --recursive

# 构建
cd /home/martins3/data/bcc/libbpf-tools
nix-shell -p elfutils zlib pkg-config
make USE_BLAZESYM=0 BPFTOOL=bpftool -j"$(nproc)"
```

这里的两个显式选项用于简化当前机器上的构建：

- `BPFTOOL=bpftool`：复用 PATH 中已经安装的 bpftool，不再构建内置 bpftool。
- `USE_BLAZESYM=0`：不构建 Rust blazesym；普通工具仍可用。需要更完整的用户态
  符号化，特别是构建 `profile`、`memleak`、`opensnoop` 等 `BZ_APPS` 时，可删除
  此选项并让 Makefile 构建 blazesym。



## bcc 真的会编译一次
经典 BCC 工具确实会在每次启动时编译 BPF C，不过通常不是启动外部 clang 进程，而是在 libbcc 进程内调用 Clang/LLVM
  库。

  以 execsnoop 为例，完整调用链是：

```txt
  tools/execsnoop.py
    BPF(text=bpf_text)
      -> Python BPF.__init__()
      -> bpf_module_create_c_from_string()
      -> BPFModule::load_string()
      -> BPFModule::load_cfile()
      -> ClangLoader::parse()
      -> ClangLoader::do_compile()
      -> CompilerInstance::ExecuteAction()
      -> EmitLLVMOnlyAction
      -> BPFModule::finalize()
      -> LLVM BPF backend 生成 eBPF 指令
```

  关键源码位置：

  - /home/martins3/data/bcc/tools/execsnoop.py 中 BPF(text=bpf_text)
    这是 execsnoop 启动时触发编译的入口。

  - /home/martins3/data/bcc/src/python/bcc/__init__.py 中 BPF.__init__()
    调用：

    lib.bpf_module_create_c_from_string(text, ...)

  - /home/martins3/data/bcc/src/cc/bcc_common.cc 中 bpf_module_create_c_from_string()
    创建 BPFModule，然后调用 BPFModule::load_string()。

  - /home/martins3/data/bcc/src/cc/bpf_module.cc 中 BPFModule::load_string() 和 BPFModule::load_cfile()
    创建 ClangLoader 并执行 ClangLoader::parse()。

  - /home/martins3/data/bcc/src/cc/frontends/clang/loader.cc 中 ClangLoader::do_compile()
    这里是真正的运行时 Clang 编译过程：

    CompilerInstance compiler0;
    compiler0.ExecuteAction(tpact);

    CompilerInstance compiler1;
    compiler1.ExecuteAction(bact);

    CompilerInstance compiler2;
    EmitLLVMOnlyAction ir_act(&*ctx_);
    compiler2.ExecuteAction(ir_act);

  三个阶段分别完成 tracepoint 结构处理、BCC C 源码重写，以及生成 LLVM IR。

  随后 /home/martins3/data/bcc/src/cc/bpf_module.cc 中 BPFModule::finalize() 设置：

  mod->setTargetTriple("bpf-pc-linux");
  builder.setMArch("bpf");
  engine_->finalizeObject();

  LLVM BPF 后端在这里把 LLVM IR 生成最终的 eBPF 指令。

  之所以不产生 clang 子进程，是因为构建时已经将 Clang 库链接进 libbcc：

  - /home/martins3/data/bcc/CMakeLists.txt 中 ENABLE_CLANG_JIT
  - /home/martins3/data/bcc/cmake/clang_libs.cmake 中 clangFrontend、clangDriver、clangCodeGen
  - /home/martins3/data/bcc/src/cc/CMakeLists.txt 中 target_link_libraries(bcc-shared ...)

  源码中真正通过 subprocess 启动外部 clang 的地方只有 /home/martins3/data/bcc/src/python/bcc/__init__.py 中 LoongArch 特殊
  处理：

  subprocess.check_output(["clang", "-print-file-name=include"])

  但它只是查询 Clang include 路径，不是编译。

  因此结论是：

  - execsnoop 等经典 BCC Python 工具：每次运行都会在进程内使用 Clang/LLVM 编译。
  - 不会执行 /usr/bin/clang -target bpf ...，所以 strace -e execve 看不到 clang。
  - 仓库中的 libbpf-tools 才是在 make 阶段显式调用外部 Clang，把 *.bpf.c 提前编译成 *.bpf.o；运行时只加载预编译对象。

## 为什么 bcc 依赖 kheaers ，但是 Libbpf-tools 不需要

- 经典 BCC：在目标机器上，根据当前内核现场编译 BPF C。
- libbpf-tools：提前编译 BPF 对象，运行时通过 CO-RE 适配目标内核。

 阶段                BCC Python 工具                libbpf-tools
━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Clang 编译          每次启动时                     构建工具时
──────────────────  ─────────────────────────────  ───────────────────────────────
 内核类型来源        当前内核 headers               构建期 vmlinux.h
──────────────────  ─────────────────────────────  ───────────────────────────────
 字段偏移适配        编译时确定                     加载时通过目标内核 BTF 重定位
──────────────────  ─────────────────────────────  ───────────────────────────────
 目标机需要          匹配的 kheaders                通常需要内核 BTF
──────────────────  ─────────────────────────────  ───────────────────────────────
 目标机需要 Clang    通过 libbcc 内置 Clang 前端    不需要

经典 execsnoop 的 BPF C 包含：

#include <linux/sched.h>

struct task_struct *task;
task = (struct task_struct *)bpf_get_current_task();
data.ppid = task->real_parent->tgid;

Clang 在编译 task->real_parent->tgid 时必须知道：

- struct task_struct 的完整定义；
- real_parent 和 tgid 在当前内核结构体中的偏移；
- 当前内核配置控制了哪些字段；
- 架构相关类型、宏和 generated headers。

因此 /home/martins3/data/bcc/src/cc/frontends/clang/loader.cc 中 ClangLoader::parse() 会查找：

/lib/modules/$(uname -r)/build
/lib/modules/$(uname -r)/source

找不到时，再由 get_proc_kheaders() 尝试使用：

/sys/kernel/kheaders.tar.xz

/home/martins3/data/bcc/src/cc/frontends/clang/kbuild_helper.cc 中 KBuildHelper::get_flags() 会生成类似内核构建的参
数：

-Iarch/.../include
-Iinclude
-Iinclude/generated/uapi
-include include/linux/kconfig.h
-D__KERNEL__

所以 BCC 依赖的不是几个普通 UAPI 头文件，而是与运行内核匹配的内核构建头文件。

libbpf-tools 的 execsnoop 则写成：

#include <vmlinux.h>
#include <bpf/bpf_core_read.h>

event->ppid = BPF_CORE_READ(task, real_parent, tgid);

源码见 /home/martins3/data/bcc/libbpf-tools/execsnoop.bpf.c。

构建时，/home/martins3/data/bcc/libbpf-tools/Makefile 执行：

clang -target bpf ... -c execsnoop.bpf.c -o execsnoop.bpf.o
bpftool gen skeleton execsnoop.bpf.o

vmlinux.h 只为 Clang 提供一套可以完成编译的内核类型定义。BPF_CORE_READ() 同时在 .bpf.o 的 BTF.ext 中留下类似这样的
CO-RE 重定位信息：

需要读取 struct task_struct.real_parent.tgid

运行到另一台机器时，libbpf读取 /sys/kernel/btf/vmlinux，找到目标内核中这些字段的实际偏移，然后修改 BPF 指令。因此目
标机器不再需要完整 kheaders，也不需要现场运行 Clang。

需要注意，libbpf-tools 并非没有内核信息依赖，而是把依赖从：

目标内核 headers + 运行时编译

换成了：

构建期 vmlinux.h + 运行时目标内核 BTF

如果目标内核没有 BTF，使用 CO-RE 的工具也可能无法运行。这个仓库通过 ensure_core_btf() 和 BTFHub/minimal BTF 提供了一
些外部 BTF 回退方案。

一句话总结：BCC 用当前内核 headers 在现场算字段偏移；libbpf-tools 把程序提前编译，并用目标内核 BTF 在加载时修正字段
偏移。


## ] bpf 实现 profile 如何实现的

- libbpf-bootstrap/examples/c/profile.bpf.c : 根据 CPU 的时钟，周期的触发中断，每次中断的时候执行 bpf ，记录下内核堆栈在那里

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
