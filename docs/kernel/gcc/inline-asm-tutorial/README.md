# GCC 内联汇编教程

本教程从简单到复杂，系统性地介绍 GCC 内联汇编的使用方法，包含 12 个循序渐进的示例，**支持 x86_64 和 ARM64 (AArch64) 两种架构**，所有代码均可在对应架构的 Linux 环境下编译运行。

## 支持的架构

| 架构 | 目录 | 说明 |
|------|------|------|
| x86_64 | `x86_64/` | Intel/AMD 64位处理器 |
| ARM64/AArch64 | `aarch64/` | ARM 64位处理器 (包括 Apple Silicon, AWS Graviton 等) |

## 目录结构

```
inline-asm-tutorial/
├── Makefile              # 自动检测架构的主 Makefile
├── README.md             # 本文件
├── x86_64/               # x86_64 架构代码
│   ├── 01_hello_asm.c
│   ├── 02_input_output.c
│   ├── 03_constraints.c
│   ├── 04_clobbers.c
│   ├── 05_memory_barrier.c
│   ├── 06_volatile.c
│   ├── 07_early_clobber.c
│   ├── 08_output_input_modifiers.c
│   ├── 09_atomic_cas.c
│   ├── 10_syscall.c
│   ├── 11_rdpmc.c
│   └── 12_cpuid.c
└── aarch64/              # ARM64 架构代码
    ├── 01_hello_asm.c
    ├── 02_input_output.c
    ├── 03_constraints.c
    ├── 04_clobbers.c
    ├── 05_memory_barrier.c
    ├── 06_volatile.c
    ├── 07_early_clobber.c
    ├── 08_output_input_modifiers.c
    ├── 09_atomic_cas.c
    ├── 10_syscall.c
    ├── 11_timer.c
    └── 12_idreg.c
```

## 快速开始

Makefile 会自动检测当前架构并编译对应的代码：

```bash
# 编译所有示例（自动检测架构）
make all

# 运行所有测试
make test

# 编译并运行单个示例
./x86_64/01_hello_asm.out
# 或
./aarch64/01_hello_asm.out
```

## 架构差异速查

### 寄存器命名

| x86_64 | ARM64 | 说明 |
|--------|-------|------|
| `rax`, `rbx`, ... | `x0`, `x1`, ... | 64位通用寄存器 |
| `eax`, `ebx`, ... | `w0`, `w1`, ... | 32位通用寄存器 |
| `rdi`, `rsi`, `rdx` | `x0`, `x1`, `x2` | 函数/系统调用参数 |
| `rax` | `x0` | 函数/系统调用返回值 |
| `r8`-`r15` | `x8`-`x18` | 额外通用寄存器 |

### 系统调用

| | x86_64 | ARM64 |
|--|--------|-------|
| 指令 | `syscall` | `svc #0` |
| 系统调用号 | `rax` | `x8` |
| 参数1-6 | `rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9` | `x0`-`x5` |
| 返回值 | `rax` | `x0` |
| write 系统调用号 | 1 | 64 |
| getpid 系统调用号 | 39 | 172 |

### 原子操作

| | x86_64 | ARM64 |
|--|--------|-------|
| 原子递增 | `lock; inc` | `LDXR`/`STXR` 循环 |
| CAS | `lock; cmpxchg` | `LDXR` + 比较 + `STXR` |
| 内存屏障 | `mfence` | `dmb sy` |

### 性能计数器

| | x86_64 | ARM64 |
|--|--------|-------|
| 时间戳 | `rdtsc`/`rdtscp` | `mrs cntvct_el0` |
| 频率 | CPU 特定 | `mrs cntfrq_el0` |
| 序列化 | `cpuid` | `isb` |

### 特性检测

| | x86_64 | ARM64 |
|--|--------|-------|
| CPU ID | `cpuid` 指令 | 读取 `MIDR_EL1` |
| 特性位 | `cpuid` 返回值 | `ID_AA64*` 系统寄存器 |
| 缓存信息 | `cpuid` | `CTR_EL0` |

## 学习路径

### 基础篇 (01-04)

#### 01_hello_asm.c - 基本语法
- `__asm__` 和 `asm` 关键字
- `__volatile__` 的作用
- Basic Asm vs Extended Asm
- 系统调用: x86_64 `syscall` / ARM64 `svc #0`

#### 02_input_output.c - 输入输出操作数
- Extended Asm 语法格式
- 输出操作数 (`"=r"`)
- 输入操作数 (`"r"`)
- 操作数编号 (`%0`, `%1`)
- 读写操作数 (`"+r"`)

#### 03_constraints.c - 约束条件
- 通用寄存器约束 (`"r"`)
- x86_64 特定: `"a"`, `"b"`, `"c"`, `"d"`, `"S"`, `"D"`
- ARM64 特定: 通过 `register ... __asm__("xN")`
- 内存约束 (`"m"`)
- 立即数约束 (`"i"`, `"n"`)
- 匹配约束 (`"0"`, `"1"`)

#### 04_clobbers.c - 修改列表
- 寄存器 clobber
- 条件码 clobber (`"cc"`)
- 内存 clobber (`"memory"`)
- 系统调用的 clobber 约定

### 进阶篇 (05-08)

#### 05_memory_barrier.c - 内存屏障
- 编译器屏障 (`"" ::: "memory"`)
- x86_64: `mfence`, `sfence`, `lfence`
- ARM64: `dmb`, `dsb`, `isb`
- 内存序与指令重排序

#### 06_volatile.c - volatile 关键字
- 防止汇编被优化删除
- 副作用与优化
- x86_64: `rdtsc` 读取时间戳
- ARM64: `mrs cntvct_el0` 读取虚拟计数器

#### 07_early_clobber.c - 早期修改
- `&` 修饰符的含义
- 解决寄存器分配冲突
- 输入输出重叠的情况

#### 08_output_input_modifiers.c - 修饰符详解
- `=` 只写
- `+` 读写
- `&` 早期修改
- 数字匹配约束

### 实战篇 (09-12)

#### 09_atomic_cas.c - 原子操作 CAS
- x86_64: `lock; cmpxchg`
- ARM64: `LDXR`/`STXR` 独占加载/存储
- 实现自旋锁
- 多线程测试

#### 10_syscall.c - 系统调用
- x86_64: `syscall` 指令
- ARM64: `svc #0` 指令
- 系统调用号和参数约定
- 错误处理

#### 11_rdpmc.c / 11_timer.c - 性能计数器
- x86_64: `rdtsc`/`rdtscp`
- ARM64: 通用计时器 `CNTVCT_EL0`
- 序列化指令
- 性能测量方法

#### 12_cpuid.c / 12_idreg.c - 特性检测
- x86_64: `cpuid` 指令
- ARM64: ID 寄存器 (`MIDR_EL1`, `ID_AA64*`)
- 缓存信息查询
- 特性位检测

## 核心概念速查

### Extended Asm 语法模板

```c
asm [volatile] (
    "汇编模板"
    : 输出操作数列表    /* 可选 */
    : 输入操作数列表    /* 可选 */
    : 修改列表          /* 可选 */
);
```

### 常用约束

| 约束 | x86_64 | ARM64 | 含义 |
|------|--------|-------|------|
| `r` | 是 | 是 | 任何通用寄存器 |
| `m` | 是 | 是 | 内存操作数 |
| `i` | 是 | 是 | 立即数 |
| `a` | 是 | 否 | `rax`/`eax` |
| `b` | 是 | 否 | `rbx`/`ebx` |
| `c` | 是 | 否 | `rcx`/`ecx` |
| `d` | 是 | 否 | `rdx`/`edx` |
| `S` | 是 | 否 | `rsi`/`esi` |
| `D` | 是 | 否 | `rdi`/`edi` |

### 修饰符

| 修饰符 | 含义 |
|--------|------|
| `=` | 只写 |
| `+` | 读写 |
| `&` | 早期修改 |

### 常用 Clobbers

| Clobber | x86_64 | ARM64 | 含义 |
|---------|--------|-------|------|
| `"memory"` | 是 | 是 | 修改任意内存 |
| `"cc"` | 是 | 是 | 修改条件码/标志位 |
| `"rax"` | 是 | 否 | 修改 `rax` |
| `"x0"` | 否 | 是 | 修改 `x0` |
| `"x8"` | 否 | 是 | 修改 `x8` (syscall) |

## 常见问题

### Q: 为什么我的汇编代码被优化掉了？
A: 必须使用 `__volatile__` 关键字。

### Q: 输入输出使用同一寄存器导致错误？
A: 使用 `&` 修饰符 (early clobber) 或 `+` 修饰符。

### Q: 多线程环境下数据不一致？
A: 添加 `"memory"` clobber 或使用原子指令。

### Q: 如何查看生成的汇编代码？
A: 使用 `make asm` 或 `gcc -S`。

### Q: 如何在 x86_64 上编译 ARM64 代码？
A: 需要交叉编译器，例如 `aarch64-linux-gnu-gcc`。

## 参考资源

### 通用
- [GCC Inline Assembly HOWTO](https://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html)
- [GCC Extended Asm](https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html)

### x86_64
- [Intel SDM](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [AMD APM](https://developer.amd.com/resources/developer-guides-manuals/)

### ARM64
- [ARM Architecture Reference Manual for A-profile](https://developer.arm.com/documentation/ddi0487/latest/)
- [ARM64 System Register Reference](https://developer.arm.com/documentation/ddi0601/latest/)

## 许可

本教程代码供学习和参考使用。

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
