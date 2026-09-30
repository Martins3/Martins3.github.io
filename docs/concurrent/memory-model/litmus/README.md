# memory model : litmus 测试工具工作原理
<!-- fef61692-2264-42da-9c42-6989fa93ff54 -->

本目录同时包含 C/LKMM 测试和 X86 指令测试。C 测试既交给
herd7 在 LKMM 上做静态分析，也交给 litmus7 生成 x86 用户态
binary；X86 指令测试既交给 herd7 在 x86 TSO 模型上分析，
也由 litmus7 生成并在真实硬件上运行。
本文解释这两条工具链的职责边界和结果含义。

## 前置依赖

```bash
# 安装 herdtools7
opam install herdtools7

# 验证安装
herd7 --version
```


## 2. 两条工具链

herdtools7 提供三个工具，对应验证内存模型的三种方法：

| 工具      | 做什么                                      | 本质                           |
| --------- | ------------------------------------------- | ------------------------------ |
| `herd7`   | 在抽象内存模型（`.cat` 文件）上模拟测试     | 数学验证，静态分析，不需要真机 |
| `litmus7` | 把测试编译成 C 可执行程序，在真实硬件上运行 | 实证，真机运行                 |
| `diy7`    | 根据约束自动生成 litmus 测试                | 测试生成                       |

## 5. 本目录用法

```sh
make check      # 用 herd7 + LKMM/x86 TSO 分析所有测试
make graph      # 用 herd7 为所有测试生成执行图
make compile    # 用 litmus7 生成并编译所有 C/X86 测试
make            # 同时执行 check 和 compile
make run        # 展示全部模型结果并运行所有真机测试
make clean      # 清理 build/
```

Makefile 细节：
- 按文件首行自动区分 `C` 和 `X86`。
- C 测试通过 `linux-kernel.cfg` 加载 LKMM，分析结果保存在
  `build/herd/lkmm/*.out`。
- X86 测试通过 `x86tso.cat` 分析，结果保存在
  `build/herd/x86/*.out`。
- `make graph` 默认为每个测试生成一个执行图，保存在
  `build/herd/graphs/{lkmm,x86}/<test>/`。
  图目录中的 `summary.out` 只统计被 `HERD_GRAPH_NSHOW` 选中的执行，
  不能代替 `make check` 的完整模型结果。
- 所有 C/X86 测试都由 litmus7 生成到 `build/litmus/`，每个测试
  都会生成一个 `.exe`。
- C binary 通过 `linux-tools-compat.h` 复用 `KERNEL_TREE` 下的
  `tools/include` 和 `tools/arch/x86/include`。该兼容头只解决 litmus7
  生成代码与内核 `noinline` 宏的命名冲突。
- `KERNEL_TREE` 默认指向 `/home/martins3/data/kernel/linux-drm`；
  `LKMM_PATH` 默认是其中的 `tools/memory-model`，
  其中包含 `linux-kernel.cfg`、`linux-kernel.def`、
  `linux-kernel.bell` 和 `linux-kernel.cat`。
- 本机 herd7/litmus7 的默认 libdir 不正确，Makefile 通过
  `HERD_LIBDIR` 和 `LITMUS_LIBDIR` 显式指定安装目录。
- 运行 `litmus7` 时 `-s`/`-r` 可调样本规模，如
  `make run LITMUS7_EXTRA="-s 100000 -r 10"`

## 6. 参考

- herdtools7 官方文档: https://diy.inria.fr/doc/index.html
- 源码: https://github.com/herd/herdtools7
- 内核内存模型文档: 内核源码 `tools/memory-model/Documentation/`
- 本目录测试对应的内核 litmus 测试集合:
  `tools/memory-model/litmus-tests/`

## 两种文件格式为什么不同

`C-MP+o-mb-o+o-mb-o.litmus` 的首行是 `C`，测试体使用 Linux
内核风格原语，可用于 LKMM 模型分析，也可在提供具体原语实现后
编译成用户态 binary。`x86-sb.litmus` 的首行是
`X86`，测试体直接描述 x86 指令，用于 x86 模型或真机测试。
它们表达的层次不同，因此格式本来就不应相同。

## 手动运行 LKMM 测试

`LKMM_PATH` 不是 LKMM 宏，只是内核 memory-model 目录的 shell
变量。herd7 需要通过该目录找到配置引用的 `.def`、`.bell`
和 `.cat` 文件：

```bash
LKMM_PATH=/path/to/linux/tools/memory-model
HERD_LIBDIR=/path/to/share/herdtools7/herd

herd7 -set-libdir "${HERD_LIBDIR}" \
	-I "${LKMM_PATH}" \
	-conf "${LKMM_PATH}/linux-kernel.cfg" \
	mp.litmus
```

## 手动运行 X86 模型测试

X86 指令测试使用 herdtools7 自带的 `x86tso.cat`：

```bash
HERD_LIBDIR=/path/to/share/herdtools7/herd

herd7 -set-libdir "${HERD_LIBDIR}" \
	-model x86tso.cat \
	x86-sb.litmus
```

## 测试结果说明

| 结果 | 含义 |
|------|------|
| `Never` | 断言的结果永远不会出现（模型保证正确性） |
| `Sometimes` | 断言的结果可能出现（允许的行为） |
| `Always` | 断言的结果总是出现 |

## 测试列表

| 文件 | 模式 | 分析模型 | 排序原语 | 模型结果 |
|------|------|----------|----------|----------|
| `C-MP+o-mb-o+o-mb-o.litmus` | Message Passing | LKMM | `smp_mb()` | Never |
| `iriw.litmus` | Independent Reads | LKMM | 无屏障 | Sometimes |
| `lb.litmus` | Load Buffering | LKMM | 无屏障 | Sometimes |
| `mp.litmus` | Message Passing | LKMM | release/acquire | Never |
| `sb+mb.litmus` | Store Buffering | LKMM | `smp_mb()` | Never |
| `sb.litmus` | Store Buffering | LKMM | 无屏障 | Sometimes |
| `wrc.litmus` | Write-Read Causality | LKMM | 无屏障 | Sometimes |
| `x86-sb.litmus` | Store Buffering | x86 TSO | 无屏障 | Sometimes |


## 附录：快速参考

### 常用 herd7 命令

```bash
make check

# 每个测试生成一个执行图
make graph

# 只为满足 exists 条件的执行生成图，每个测试最多两张
make graph HERD_GRAPH_SHOW=prop HERD_GRAPH_NSHOW=2

# 指定内存模型版本
make check LKMM_HERD7_EXTRA="-variant lkmmv2"
```

### 常用 klitmus7 命令

```bash
# 生成 C 代码
klitmus7 -o output_dir test.litmus

# 指定运行次数
klitmus7 -o output_dir -n 100000 test.litmus

# 指定线程数
klitmus7 -o output_dir -a 2 test.litmus
```

## 这个问题
herd7 还是什么之类的，简单看看就可以了:
	- 下一个问题就是这个了:
	- docs/concurrent/lkmm/other-tools.md
	- /home/martins3/data/vn/docs/concurrent/1-litmus.md

## 其他 Memory Model 测试工具

Linux
内核生态和学术界还有多种工具用于验证、测试和分析内存模型。这些工具从不同角度（形式化验证、动态检测、压力测试、硬件测试）来保障并发代码的正确性。

## 1. herdtools7 套件中的其他工具

herdtools7 是一个完整的工具链，除了 herd7 之外，还包括以下工具：

### 1.1 klitmus7 — 内核模块生成器

**功能**: 将 litmus test 编译成可加载的内核模块（.ko），在真实硬件上运行测试。

**与 herd7 的区别**:

- herd7: 在模拟器中运行，基于 LKMM 的公理验证所有可能的执行路径
- klitmus7: 在真实 CPU 上运行，通过大量重复执行来观察实际硬件行为

**使用方法**:

```bash
# 生成内核模块源代码
klitmus7 -o klitmus_output/ test.litmus

# 指定运行参数
klitmus7 -o klitmus_output/ -n 100000 -a 2 test.litmus
# -n: 每个测试运行次数
# -a: 可用 CPU 数

# 编译并加载（在生成的目录中）
cd klitmus_output/
make
insmod test.ko
```

**兼容性**: klitmus7 生成的内核模块与内核版本和 herdtools7
版本都有绑定关系。详见 `tools/memory-model/README` 中的兼容性表格。

### 1.2 litmus7 — 用户态硬件测试

**功能**: 在用户态直接运行 litmus test，测试真实 CPU
的内存序行为，无需编译内核模块。

**与 klitmus7 的区别**:

- litmus7: 用户态程序，更容易运行，但受限于用户态内存访问
- klitmus7: 内核模块，可以测试内核特有的原语（如 RCU、spinlock）

**使用方法**:

```bash
# 生成并编译所有 C/X86 litmus test
make compile

# 指定运行参数
make run LITMUS7_EXTRA="-s 100000 -r 10"
```

litmus7 会保留 C 测试中的 LKMM 原语。本目录通过内核 tools
头文件为它们提供 x86 用户态实现。需要测试真正的内核态实现、
RCU 或 spinlock 等原语时，应使用 klitmus7。

### 1.3 diy7 — Litmus Test 生成器

**功能**: 根据指定的模式自动生成 litmus tests。

**使用方法**:

```bash
# 生成 Message Passing 模式的测试
diy7 -arch Linux MP

# 生成 Store Buffering 模式的测试
diy7 -arch Linux SB

# 生成所有经典模式
diy7 -arch Linux -mode uni
```

diy7 可以生成大量测试用例，配合 herd7
批量验证，是发现内存模型边界情况的有力工具。

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
