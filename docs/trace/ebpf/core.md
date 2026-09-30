# ebpf CO:RE
<!-- 541f504d-5ed1-43bf-8c2d-ab63b67846f7 -->

- design : https://nakryiko.com/posts/bpf-portability-and-co-re/
- reference : https://nakryiko.com/posts/bpf-core-reference-guide/

## 结论

CO:RE 中真正可移植的是带 BTF 和 CO:RE relocation 的 BPF ELF object，也就是
`.bpf.o`。编译时的 `vmlinux.h` 提供本地类型定义；加载时 libbpf 读取目标内核的
BTF，计算字段偏移并修改 BPF 指令，然后把程序交给内核 verifier。

CO:RE 不负责下面这些兼容性：

- 用户态 loader 的 glibc、libbpf、libelf、zlib ABI 和 ELF interpreter；
- 目标内核是否支持程序使用的 map、helper、program type、attach type；
- tracepoint、kprobe 函数或其他 hook 是否存在；
- CPU 架构、字节序以及 `pt_regs` 等架构相关 ABI。

因此 “compile once” 不能理解为“整个程序随便复制到任何 Linux 都能运行”。

## 实验

这个 demo 只有 `code/core.bpf.c`，没有自定义用户态 loader。`record_target_layout()`
通过 `bpf_core_field_exists()` 和 `bpf_core_field_offset()` 为
`task_struct.tgid` 生成 CO:RE relocation，并把目标内核解析出的结果写入 `results`
map。加载、挂载和读取 map 全部使用 bpftool。

### 构建唯一的 `.bpf.o`

```bash
cd /home/martins3/data/vn/docs/trace/ebpf/code
make core
file .output/core.bpf.o
readelf -SW .output/core.bpf.o | grep -E 'cgroup_skb|\.maps|\.BTF'
```

`make core` 只生成 `.output/core.bpf.o`，不会生成 `core.out`。这个 object 在宿主机
7.1 内核生成的 `vmlinux.h` 上编译，local `task_struct.tgid` 偏移是 2868 bytes。
`.BTF.ext` 的 `core_relo_len` 为 44，包含 field-exists 和 field-byte-offset 两条
relocation。


### 操作

我利用 collei virtme 和物理机中操作，是完全一样的操作:

```bash
cd /home/martins3/data/vn/docs/trace/ebpf/code

sudo ./bpftool.out prog load \
  .output/core.bpf.o \
  /sys/fs/bpf/core-demo-prog \
  pinmaps /sys/fs/bpf/core-demo-maps

sudo ./bpftool.out cgroup attach \
  /sys/fs/cgroup \
  cgroup_inet_egress \
  pinned /sys/fs/bpf/core-demo-prog

ping -c 1 127.0.0.1

sudo ./bpftool.out -j -p map dump \
  pinned /sys/fs/bpf/core-demo-maps/results
```

期望 `formatted.value` 为：

```json
{
  "tgid_exists": 1,
  "tgid_offset": 2772
}
```

清理：

```bash
sudo ./bpftool.out cgroup detach \
  /sys/fs/cgroup \
  cgroup_inet_egress \
  pinned /sys/fs/bpf/core-demo-prog
sudo unlink /sys/fs/bpf/core-demo-prog
sudo unlink /sys/fs/bpf/core-demo-maps/results
sudo rmdir /sys/fs/bpf/core-demo-maps
```

## 问题记录

### bpftrace 对 CO:RE 有感知吗

对 BTF 有感知，但不能据此认为 bpftrace 产生的是可跨内核复用的 CO:RE object。

在 `oe2403` 的 bpftrace 0.19.1 和 `yyds-fs` 的 bpftrace 0.23.5 上执行：

```bash
sudo bpftrace --emit-elf /tmp/core-bpftrace.bpf.o \
  -e 'kprobe:do_sys_open { @comm = ((struct task_struct *)curtask)->comm; }'
```

结果：

- 0.19.1 生成的 ELF 没有 `.BTF` 和 `.BTF.ext`；
- 0.23.5 有 `.BTF` 和 `.BTF.ext`，但 `.BTF.ext` header 中
  `core_relo_len` 为 0；
- `task_struct.comm` 在两个目标内核上的偏移分别是 3152 和 3312 bytes。

这说明这次脚本中 bpftrace 在目标机运行时读取 BTF 并按目标布局生成代码，而不是
生成待另一个目标上的 libbpf 再重定位的 object。此结论限定于上述版本和脚本；
判断其他版本时应直接检查生成 ELF 的 `.BTF.ext` 中是否存在 CO:RE relocation。

### CO:RE 是否需要 LLVM 支持

需要。Clang/LLVM 负责识别 `preserve_access_index` 及
`__builtin_preserve_*` builtins，并在 `.BTF.ext` 中发出 CO:RE relocation；
libbpf 负责在加载时消费 relocation。

bpftool 构建输出中的：

```text
clang-bpf-co-re: [ OFF ]
```

是 bpftool 编译时的 feature test 结果，控制 bpftool 自身可选 BPF skeleton 的
构建，不代表已经构建出的 bpftool 不能加载 CO:RE object。

### libbpf 和 bpftool 的关系

libbpf 是 C library/loader：解析 BPF ELF、创建 maps、执行 CO:RE relocation、
加载程序和建立 links。bpftool 是面向人和脚本的通用 CLI，本身使用 libbpf：

- `bpftool prog load` 通过 libbpf 加载 object；
- `bpftool gen skeleton` 把 `.bpf.o` 嵌入生成的 C header；
- skeleton 的 `open/load/attach/destroy` 最终仍调用 libbpf；
- `bpftool btf dump` 和 `bpftool gen min_core_btf` 是检查或生成 BTF 的工具。

所以 bpftool 不是 libbpf 的替代 loader，而是 libbpf 的一个 CLI 用户和配套的
构建/诊断工具。bpftool 的官方仓库也把 libbpf 作为 submodule。

### BTFHub 能否解决 4.19/3.10 没有内核 BTF 的问题

能解决“目标 BTF 缺失”这一项，不能补齐旧内核缺少的 BPF 功能。

BTFHub 为具体的发行版、版本、架构和 kernel release 提供一一对应的外部 BTF。
loader 通过 `bpf_object_open_opts.btf_custom_path` 把它交给 libbpf。bcc/libbpf-tools
使用 `ENABLE_MIN_CORE_BTFS=1` 时，则由 `bpftool gen min_core_btf` 只保留应用需要的
types，把多个目标的最小 BTF 打包进 binary；`ensure_core_btf()` 只在目标没有
native BTF 时选择精确匹配的文件。

BTFHub 的 supported-distros 表中出现 3.10，只表示有对应 BTF archive。该表中早期
CentOS 7 的 `BPF` 列仍是 `-`，后期 3.10 也明确注明可用 eBPF 功能很有限。因此
仍要逐项检查应用所需 program type、map、helper 和 attach point。

本次 collei 环境不能验证 BTFHub 路径：现有 5.4、6.6、6.19 guest 都启用了
`CONFIG_DEBUG_INFO_BTF=y`。所以“4.19 + external BTF 可运行”在这里仍是上游能力，
不是本次实验结果。后续需要准备一台 `/sys/kernel/btf/vmlinux` 不存在且 BTFHub
恰好包含其完整 kernel release 的 guest 再验证。

### eunomia-bpf 解决什么问题

eunomia-bpf 在 libbpf/CO:RE 之上提供编译工具链和动态运行时，主要减少用户态
loader 样板代码，并把 BPF 程序、参数和数据导出描述打包成 JSON、Wasm 或 OCI
artifact，方便构建、分发和运行。

它解决的是开发与交付层问题，不改变 CO:RE 的底层边界：目标仍需兼容所用 BPF
功能，也仍需 native 或 external BTF 来完成 relocation。


### “BPF CO:RE” 指的是 `.bpf.o` 可移植吗

是，这是最准确的理解。但还要加两个限定：

1. object 必须包含 local BTF 和对应的 CO:RE relocation；仅有 `.BTF.ext` section
   不够，还要确认 `core_relo_len` 非零；
2. target 必须提供匹配 BTF，并支持 object 使用的其他 BPF 能力。

## human
1. bpftrace 对于 CORE 有感知吗?
2. CO:RE 需要 llvm 的支持
```txt
make: Entering directory '/root/bpftool/rpmbuild/BUILD/bpftool-6.8.0/src'
...                        libbfd: [ on  ]
...        disassembler-four-args: [ OFF ]
...                          zlib: [ on  ]
...                        libcap: [ on  ]
...               clang-bpf-co-re: [ OFF ]
```


## 其实这个是一个正规的项目
https://github.com/aquasecurity/btfhub

什么 libbpf 是 loader 而不是 bpftool ，两者到地什么关系

> This is achieved by the libbpf loader, a component within the eBPF's loader
> and verification architecture. The libbpf loader arranges the necessary
> infrastructure for an eBPF object, including eBPF map creation, code
> relocation, setting up eBPF probes, managing links, handling their
> attachments, among others.

## 真的可以 用 btfhub 解决 4.19 的 kernel libbpf 的使用问题!

而且从 https://github.com/aquasecurity/btfhub/blob/main/docs/supported-distros.md
看，连 3.10 kernel 都是支持的。

## 这个库解决了什么问题?
https://github.com/eunomia-bpf/eunomia-bpf


## bpf core 到指的是 bpf.o 可以移植吧？

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
