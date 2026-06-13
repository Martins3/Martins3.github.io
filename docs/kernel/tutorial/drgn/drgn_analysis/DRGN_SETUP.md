# drgn + Nix 使用说明

## 一句话

这个目录现在只保留一条可用路径：

```bash
cd /home/martins3/data/vn/.worktrees/block-scheduler/docs/kernel/tutorial/drgn/drgn_analysis
bash ./drgn-wrapper.sh -c /proc/kcore ./irq_hierarchy.py --list
```

## 保留的文件

- `shell.nix`
- `drgn-wrapper.sh`
- `run.sh`
- `irq_hierarchy.py`
- `kvm_vcpu_dump.py`
- `kvm_vcpu_select_dump.py`
- `kvm_vcpu_full_dump.py`

## 为什么还需要 `shell.nix`

`drgn` 本体在当前目录的 `.venv` 里，但它运行时依赖的动态库来自 Nix：

- `zlib`
- `libstdc++`
- `libelf`
- `xz`
- `bzip2`

`drgn-wrapper.sh` 做的事情已经压到最小：

1. 如果当前不在 `nix-shell`，自动进入 `shell.nix`
2. 如果 `.venv/bin/drgn` 不存在，自动用 `uv` 安装
3. 把 `drgn.libs` 和 Nix 运行库拼到 `LD_LIBRARY_PATH`
4. 只有访问 `/proc/kcore` 时才走 `sudo`

## 当前推荐命令

### 1. 验证 drgn 可启动

```bash
cd /home/martins3/data/vn/.worktrees/block-scheduler/docs/kernel/tutorial/drgn/drgn_analysis
bash ./drgn-wrapper.sh --version
```

### 2. 列出 IRQ 概要

```bash
cd /home/martins3/data/vn/.worktrees/block-scheduler/docs/kernel/tutorial/drgn/drgn_analysis
bash ./drgn-wrapper.sh -c /proc/kcore ./irq_hierarchy.py --list
```

### 3. 分析指定 virq

```bash
cd /home/martins3/data/vn/.worktrees/block-scheduler/docs/kernel/tutorial/drgn/drgn_analysis
bash ./drgn-wrapper.sh -c /proc/kcore ./irq_hierarchy.py 24
```

### 4. 列出 KVM vCPU

```bash
cd /home/martins3/data/vn/.worktrees/block-scheduler/docs/kernel/tutorial/drgn/drgn_analysis
bash ./run.sh
```

### 5. 查看第一个 vCPU 的完整结构

```bash
cd /home/martins3/data/vn/.worktrees/block-scheduler/docs/kernel/tutorial/drgn/drgn_analysis
bash ./drgn-wrapper.sh -c /proc/kcore ./kvm_vcpu_select_dump.py
```

### 6. 查看指定 vCPU 地址

```bash
cd /home/martins3/data/vn/.worktrees/block-scheduler/docs/kernel/tutorial/drgn/drgn_analysis
bash ./drgn-wrapper.sh -c /proc/kcore ./kvm_vcpu_full_dump.py 0xffff888109ab2300
```

### 7. 查看 block mq 关系

```bash
cd /home/martins3/data/vn/.worktrees/block-scheduler/docs/kernel/tutorial/drgn/drgn_analysis
bash ./drgn-wrapper.sh -c /proc/kcore ./blk_mq_relationship.py
```

指定磁盘：

```bash
bash ./drgn-wrapper.sh -c /proc/kcore ./blk_mq_relationship.py vda
```

## 实测结果

2026-03-27 已实际测试：

```bash
bash ./drgn-wrapper.sh --version
bash ./drgn-wrapper.sh -c /proc/kcore ./irq_hierarchy.py --list
```

验证结论：

- `drgn` 可以正常启动
- `/proc/kcore` 分析可以跑通
- 当前环境里的 `sudo` 需要密码，wrapper 已处理
- 仍然会看到 `missing debugging symbols` 警告，但不影响当前脚本使用

## 当前边界

- live kernel 分析本质上还是需要 root
- 如果要获得更完整的类型/符号信息，后续还要补 debuginfo

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
