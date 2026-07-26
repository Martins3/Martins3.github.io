#!/usr/bin/env bash
set -E -e -u -o pipefail

# 禁止 root 用户运行
if [[ $EUID -eq 0 ]]; then
	echo "[ERROR] 此脚本不允许以 root 用户运行"
	echo "由于当前运行在 nfs ，使用 root 编译会遇到问题"
	echo "请使用普通用户运行，脚本会自动使用 sudo 执行需要 root 权限的操作"
	exit 1
fi

PROGDIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
cd "$PROGDIR"

/usr/bin/gcc -B/usr/bin -o mkfs.simplefs.out user_mkfs.c mkfs_common.c -Wall
make -C /home/martins3/data/kernel/linux-drm M="$(pwd)" modules

# 测试记录：
# 一共有从 0 到 787 个测试
# 100 - ✗ 失败（输出文件缺失）
# 101 - ✓ 通过
# 102 - ✓ 通过
# 103 - ✗ 失败
# 104 - ✓ 通过
# 105 - ✗ 失败
# 106 - ✓ 通过
# 107 - ✓ 通过
# 108 - ✗ 失败（输出文件缺失）
# 109 - ✗ 错误（设备忙）
# 110 - ✗ 错误
# 111 - ✓ 通过
# 112 - ✓ 通过
# 113 - ✓ 通过
# 114 - ✓ 通过
# 115 - ✓ 通过
# 116 - ✓ 通过
# 117 - x 内核宕机
# 118 - ✓ 通过
