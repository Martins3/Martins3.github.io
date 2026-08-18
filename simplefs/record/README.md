# SimpleFS 历史记录

`record/` 保存带时间背景的证据，不承担当前教程职责：

- [xfstests/](xfstests/README.md)：基线、阶段推进、最终全量结果与 NOTRUN 分类；
- [investigations/](investigations/README.md)：独立问题调查和设计记录；
- [archive/](archive/README.md)：早期问答、待办、环境片段和已取代脚本。

最新一次统一全量验收是 2026-07-26：generic/001--787 连续完整运行，PASS 432 /
NOTRUN 355 / FAIL 0 / TIMEOUT 0；787 份日志齐全，NOTRUN 均有原因，dmesg 无非预期
warning/Oops/panic。完整环境和哈希见
[JBD2/Phase 2 记录](xfstests/2026-07-20-phase2-jbd2-progress.md)。

判定规则：

- PASS 只表示用例实际执行、输出匹配且内核健康；
- NOTRUN 必须保留真实的 capability、格式或环境原因；
- TIMEOUT、外部中断、VM reset 不计为 PASS；
- 内核 warning/Oops/panic 即使用例输出匹配也必须调查和重跑；
- 历史 PASS 只覆盖当时的源码、内核、模块、mkfs 和 runner，当前变化要重新回归。

## 环境信息
```nix
with import <nixpkgs> { };

# 各种 C 环境合集都放这里了
pkgs.llvmPackages.stdenv.mkDerivation {
  name = "C test";
  buildInputs = with pkgs; [
    cmake
    libpcap
    liburing
    libtraceevent
    glib
    pkg-config
    fuse3
    libaio
    numactl
    xfsprogs
    util-linux
    acl
    attr
    libcap
    gdbm
    e2fsprogs
    btrfs-progs
    # glibc.static # 可以静态编译
    # 2025-05-09 发现添加上这个，编译运行，程序会直接 crash 的。
  ];
  LD_LIBRARY_PATH = "${lib.makeLibraryPath [ libaio ]}";
}
```
