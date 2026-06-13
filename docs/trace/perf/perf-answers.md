# Perf 问题解答

## 2. perf-stat -e cycles 的意义

**问题**: 使用 `perf stat -e cycles` 是不是可以测试 CPU 频率？输出有什么意义？

**回答**:
- `cycles` 是 **CPU 时钟周期计数器**，不是直接的频率测量
- 频率 = cycles / time，perf-stat 会显示运行时间，你可以计算平均频率
- 意义在于：
  - 评估代码的 CPU 效率
  - 结合 `instructions` 计算 **IPC (Instructions Per Cycle)**
  - IPC > 1 表示指令级并行性好，< 1 表示有流水线停滞

```bash
perf stat -e cycles,instructions ./your_program
# 输出中的 "insn per cycle" 就是 IPC
```

---

## 3. perf-annotate 的 "执行级别 perf"

**问题**: 用于分析指令级别的代码，但是执行级别的 perf 没太看懂

**回答**:
`perf annotate` 显示的是 **采样级别的热点指令**，不是每条指令的执行时间。原理：
1. perf 定期采样（默认 4000次/秒）PC 寄存器值
2. 统计哪些指令地址出现频率高
3. 反汇编后显示百分比

你看到某条指令占 92%，不是因为它执行慢，而是因为**它被采样到的次数多**（例如它是循环的跳转目标）。配合 `-e cache-misses` 等事件可以看到特定事件下的热点。

---

## 4. strace vs perf trace 性能对比

**问题**: 简单对比下 strace 和 perf trace 的性能差别

**回答**:

| 特性 | strace | perf trace |
|------|--------|------------|
| 实现方式 | ptrace (每次系统调用都触发上下文切换) | perf event (内核 tracepoint，无侵入) |
| 性能开销 | **高** (慢 10-100倍) | **低** (接近原生性能) |
| 精度 | 完整记录每次调用 | 基于采样或事件 |
| 适用场景 | 调试、开发 | 生产环境性能分析 |

**性能测试对比**:
```bash
# strace 会严重拖慢程序
time strace -c ls > /dev/null

# perf trace 开销极小
time sudo perf trace -s ls > /dev/null
```

---

## 5. minor/major page fault 的观测

**问题**: 可以观测 minor/major page fault 吗？minor fault 是通过 pmc 实现的吗？

**回答**:
是的，perf 可以观测：

```bash
# 使用软件事件
perf stat -e page-faults,minor-faults,major-faults ./program

# 使用 tracepoint 查看具体发生在哪里
perf record -e exceptions:page_fault_user -g ./program
```

**关于实现**:
- 缺页是通过 `task_struct` 中的 `min_flt` 和 `maj_flt` 字段统计的

## 7. 关于递归调用和火焰图层次结构

**问题**: 确认下，应该不是每次 stack backtrace 来解决的，不然性能问题太大了，但是这种层次结构是如何实现的?

**回答**:
你的理解**部分正确但有误区**：

1. **确实是基于栈回溯 (stack backtrace)** 实现的
2. **性能开销确实存在**，但 perf 使用 **采样 (sampling)** 而非追踪每个函数调用来降低开销：
   - 默认每秒 4000 次采样（可通过 `-F` 调整）
   - 使用 `frame pointer` 或 `dwarf` 进行栈回溯

3. **递归导致的重复调用**：
   - 火焰图会正确显示递归，但会占用多层栈空间
   - 对于深递归，可能导致栈截断（可用 `--call-graph dwarf,65528` 增加深度）

**验证递归场景**:
```bash
# 创建测试用例验证
cat > /tmp/recursion_test.c << 'EOF'
void recursive(int n) {
    if (n > 0) recursive(n - 1);
}
int main() { recursive(100); return 0; }
EOF
gcc -O0 -g -fno-omit-frame-pointer /tmp/recursion_test.c -o /tmp/recursion_test
perf record -g /tmp/recursion_test
perf report
```

---



---

## 11. 自编译内核缺少符号信息

**问题**: 自己构建的内核，有时候 perf 没有符号信息

**回答**:
可能的原因和解决方案：

1. **缺少 vmlinux**:
```bash
# 显式指定 vmlinux 路径
sudo perf report -k /path/to/vmlinux
```

2. **模块符号未加载**:
```bash
# 确保模块符号在正确位置
sudo mkdir -p /usr/lib/debug/lib/modules/$(uname -r)/
sudo cp /path/to/vmlinux /usr/lib/debug/lib/modules/$(uname -r)/
sudo cp drivers/**/*.ko.debug /usr/lib/debug/lib/modules/$(uname -r)/kernel/
```

3. **内核配置问题**:
```bash
# 确保编译时启用了 debug 信息
CONFIG_DEBUG_INFO=y
CONFIG_DEBUG_INFO_DWARF5=y  # 或 DWARF4
```

4. **kallsyms 被限制**:
```bash
# 检查 kptr_restrict
cat /proc/sys/kernel/kptr_restrict  # 应为 0
sudo sh -c 'echo 0 > /proc/sys/kernel/kptr_restrict'
```

---

## 13. 异构 CPU (P-Core/E-Core) 的问题

**问题**: 13900k 上 `perf record -C 1` 报错 cpu_atom 不支持

**回答**:
这是 Intel 12代+ 混合架构的问题。P-Core (性能核) 和 E-Core (能效核) 有不同的 PMU 能力。

**原因**: CPU 1 是 P-Core，但 perf 尝试使用 `cpu_atom` 事件，这仅适用于 E-Core (16-31)。

**解决方案**:
```bash
# 1. 显式指定使用 cpu_core PMU
sudo perf record -e cpu_core/cycles/P -C 1 -- sleep 10

# 2. 或者让 perf 自动选择正确的 PMU
sudo perf record --no-bpf-event -C 1 -- sleep 10

# 3. 监控所有 P-Core 或 E-Core
sudo perf record -e cycles -C 0-15 -- sleep 10   # P-Core
sudo perf record -e cycles -C 16-31 -- sleep 10  # E-Core
```

## 15. 其他工具说明

### perf-diff
比较两份 perf.data 的差异：
```bash
perf diff perf.old.data perf.new.data
```

### perf-archive
打包 perf.data 及其依赖的符号：
```bash
perf archive perf.data -o perf.tar.gz
```

### perf-test
运行 perf 的自检测试：
```bash
perf test list
perf test 1
```

### perf-buildid-cache
管理构建 ID 缓存：
```bash
perf buildid-cache -a /path/to/lib
perf buildid-list -i perf.data
```

### perf-daemon
后台收集性能数据：
```bash
perf daemon start --config perf.conf
```

### perf-config
配置 perf 默认选项：
```bash
perf config annotate.show_nr_samples=true
```


## 17. perf-list 查看 tracepoint subsys

**问题**: man perf-list(1) 分析了很多内容，没太看懂

**回答**:
可以使用以下方式查看特定子系统的 tracepoint：

```bash
# 列出 kvm 相关的所有 tracepoint
perf list kvm

# 列出 sched 相关的
perf list sched

# 列出所有 tracepoint
perf list 'tracepoint:*'

# 列出所有硬件事件
perf list 'hw:*'

# 列出所有软件事件
perf list 'sw:*'
```

---

## 18. perf-script-perl / perf-script-python

**问题**: perf-script-perl 和 perf-script-python 的用途

**回答**:
perf script 支持使用 Perl 或 Python 脚本处理采样数据：

```bash
# 使用 Python 脚本处理
perf script -g python

# 使用内置的 Python 脚本示例
perf script -s /usr/libexec/perf-core/scripts/python/syscall-counts.py

# 常用的内置脚本
perf script -s /usr/share/perf-core/scripts/python/flamegraph.py
```

这些脚本位于内核源码 `tools/perf/scripts/` 目录下，可以自定义分析逻辑。

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
