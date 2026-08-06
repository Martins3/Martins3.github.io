# AGENTS.md - Linux 内核模块开发项目

严禁在物理机中 insmod ！
## 在物理机中构建

### 内核源码配置

构建系统默认使用 `~/data/kernel/default` 的内核源码。可通过以下方式覆盖：
- `NORMAL=1` - 使用 `/lib/modules/$(uname -r)/build`
- `VERSION=x.y.z` - 使用特定内核版本
- `NIXOS=1` - 使用 NixOS 内核路径

如果运行在 hyperv 中，将 default kernel 设置为 $HOME/data/kernel/linux-hyperv
然后继续调试。

### 构建设置（config.h）

`config.h` 文件控制哪些测试被编译。通过取消注释其定义来启用测试：

```c
#define CONFIG_TEST_TIF 1        // 当前已启用
// #define CONFIG_TEST_SOFTIRQ 1 // 当前已禁用
```

### 构建命令

```bash
# 构建所有模块
make

# 清理构建产物
make clean

# 生成 rust-analyzer 数据（用于 Rust 实验）
make rs
```

### 创建新测试
不要手动创建，手动创建容易漏掉东西，而是需要使用 `ktest.sh` 脚本搭建新测试框架：

```bash
# 创建新测试（交互式）
./ktest.sh newtest

# 删除测试
./ktest.sh -r newtest
```

这将：
1. 创建测试源文件
2. 添加条目到 `internal.h`、`main.c` 和 `config.h`
3. 可选创建用户空间测试程序

## 在虚拟机中测试
如何启动虚拟机和使用虚拟机，检查本项目中的 collei/AGENTS.md

可以使用 yyfs-fs 虚拟机测试:

不要手动的加载，而是使用脚本
```bash
# 使用动作 1 测试 workqueue
./mod.sh workqueue 1

# 测试 RCU
./mod.sh rcupdate 0

# 测试内存模型
./mod.sh memory_model 1

# 重新加载模块
./mod.sh -r
```

## 代码风格指南

### 格式化
- 使用提供的 `.clang-format` 配置

### 注释

- 代码库中全部使用中文注释
- 注释解释"为什么"而不仅仅是"做什么"
- 相关时引用内核源文件

## 测试策略

### 单元测试

每个测试文件专注于特定的内核 API 或机制：
- 测试相互独立，可单独运行
- 动作参数选择特定测试用例
- 测试使用 `pr_info()` 输出

### 与用户空间集成

某些测试包含用户空间组件：
- `*-user.c` 文件用于用户空间测试
- `user.mk` 用于构建用户空间程序
- `*.sh` 脚本用于复杂测试场景


## 开发工作流

1. **在 config.h 中启用测试**：取消注释 `CONFIG_TEST_*` 行
2. **重新生成对象**：运行 `./config.sh`
3. **构建**：运行 `make`

## 常见问题

### 模块版本不匹配

insmod: ERROR: could not insert module martins3.ko: Invalid module format
dmesg: martins3: version magic '6.19.0-...' should be '6.18.8-100.fc42.x86_64'

原因：默认使用 ~/data/kernel/default（6.19.0）编译，但运行系统是 6.18.8

vn/build 重新构建内核，kill 掉虚拟机，重新拉起虚拟机测试

### Sysfs 未出现

模块可能加载失败。使用 `dmesg` 检查错误。

可能原因：config.h 中这些测试被注释掉了

修复：启用需要的测试：

### liburing.h 找不到

iouring-user.c:13:10: fatal error: liburing.h: No such file or directory

原因：系统没有安装 liburing 库，应该加载 ./default.nix 在 nix 环境中构建

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
