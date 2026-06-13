# LKMM Litmus Tests

本目录包含可运行的 litmus tests 和测试脚本，用于验证 Linux Kernel Memory Model。

## 目录结构

```
lkmm-tests/
|-- README.md              # 本文件
|-- run.sh                 # 一键运行脚本
|-- tests/                 # litmus test 文件
|   |-- mp.litmus         # Message Passing
|   |-- sb.litmus         # Store Buffering
|   |-- lb.litmus         # Load Buffering
|   |-- iriw.litmus       # Independent Reads Independent Writes
|   |-- wrc.litmus        # Write to Read Causality
|-- results/               # 测试结果输出
```

## 前置依赖

```bash
# 安装 herdtools7
opam install herdtools7

# 验证安装
herd7 --version
```

## 快速开始

```bash
# 进入本目录
cd docs/concurrent/lkmm-tests

# 运行所有测试
./run.sh

# 运行单个测试
./run.sh tests/mp.litmus
```

## 手动运行

```bash
# 设置内核 memory-model 路径
export LKMM_PATH=/path/to/kernel/tools/memory-model

# 运行单个测试
herd7 -conf $LKMM_PATH/linux-kernel.cfg \
      -I $(herd7 --version 2>&1 | head -1 | xargs dirname)/../share/herdtools7/herd \
      tests/mp.litmus
```

## 测试结果说明

| 结果 | 含义 |
|------|------|
| `Never` | 断言的结果永远不会出现（模型保证正确性） |
| `Sometimes` | 断言的结果可能出现（允许的行为） |
| `Always` | 断言的结果总是出现 |

## 测试列表

| 测试 | 模式 | 无屏障 | 有屏障 |
|------|------|--------|--------|
| MP | Message Passing | Sometimes | Never (release/acquire) |
| SB | Store Buffering | Sometimes | Never (smp_mb) |
| LB | Load Buffering | Sometimes | Never (smp_mb) |
| IRIW | Independent Reads | Sometimes | Never (smp_mb) |
| WRC | Write-Read Causality | Sometimes | Never (smp_mb) |


## 附录：快速参考

### 常用 herd7 命令

```bash
# 运行单个测试
herd7 -conf linux-kernel.cfg test.litmus

# 显示所有状态
herd7 -conf linux-kernel.cfg -show all test.litmus

# 生成图形化执行图
herd7 -conf linux-kernel.cfg -show exec test.litmus

# 指定内存模型版本
herd7 -variant lkmmv2 -conf linux-kernel.cfg test.litmus
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
