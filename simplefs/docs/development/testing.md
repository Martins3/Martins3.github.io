# 构建与测试环境

## 构建边界

代码位于 NFS 共享的 `~/data`。内核模块必须在物理机以普通用户构建；guest root 在
NFS 上编译会受到 root-squash 和构建环境影响。

```bash
cd /home/martins3/data/vn/simplefs
./build.sh
```

`build.sh` 同时生成 `mkfs.simplefs.out` 和 `simplefs.ko`。两者必须来自同一份当前
源码。测试前可检查：

```bash
stat -c '%y %n' mkfs.simplefs.out simplefs.ko
modinfo -F vermagic simplefs.ko
```

vermagic 必须匹配 guest `uname -r`。旧 mkfs 可能写出当前模块无法恢复的 journal，
所以只重建 ko 不足以建立可信环境。

## yyds-fs 中执行

单用例：

```bash
ssh -p 51404 root@localhost \
  "cd /home/martins3/data/vn/simplefs && ./xfstests-full.sh 001"
```

范围与全量：

```bash
./xfstests-full.sh 100-200
./xfstests-full.sh
```

结果和逐例日志位于：

```text
/var/tmp/simplefs-xfstests/xfstests_full_results.txt
/var/tmp/simplefs-xfstests/xfstests_full_logs/
```

## 全量环境前置条件

除了 `simplefs.ko` 和 mkfs，guest 还需要匹配当前内核构建配置的
`dm-flakey`、`dm-thin-pool`、`dm-log-writes`、`scsi_debug` 等模块，以及 xfstests
用户、组和工具。模块缺失会使相关用例 NOTRUN；模块与运行内核配置不一致甚至可能
制造与 SimpleFS 无关的 warning。

全量验收前必须确认这些模块能 `modprobe`，并记录 `uname -r`、模块 vermagic、
SimpleFS/mkfs 哈希。历史统一环境的完整证据见
[2026-07-26 记录](../../record/xfstests/2026-07-20-phase2-jbd2-progress.md)。

## VM 警告与恢复

发现 WARNING、Oops、panic 或持续异常后先保存日志：

```bash
./collei/scripts/collei-action.py -a log -n yyds-fs
```

保存现场后使用快速重启，不在 guest 中执行完整 reboot：

```bash
./collei/scripts/collei-action.py -a force_reboot -n yyds-fs
```

如果替换了 `-kernel` 指向的内核镜像，`force_reboot` 不会让 QEMU 重新读取它，必须
按 collei 规则 kill 再 run。

## 每次变更的检查

```bash
shellcheck -x xfstests-full.sh tests/xfstests/prepare.sh
shfmt -d -ci -s -bn xfstests-full.sh tests/xfstests/prepare.sh
python3 -m py_compile tests/xfstests/patch.py
git diff --check
```

Python 编译检查在物理机执行，不要让 guest root 在 NFS 目录生成 `__pycache__`。

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
