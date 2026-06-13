#!/usr/bin/env bash
# 最小化 drgn 包装：进入 nix-shell，确保本地 .venv 有 drgn，需要 /proc/kcore 时再 sudo

set -E -e -u -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHELL_NIX="${SCRIPT_DIR}/shell.nix"
VENV_DIR="${SCRIPT_DIR}/.venv"
DRGN_BIN="${VENV_DIR}/bin/drgn"

if [[ $# -eq 0 ]]; then
	echo "用法: $0 [drgn 参数]"
	echo "示例: $0 --version"
	echo "示例: $0 -c /proc/kcore ./irq_hierarchy.py --list"
	exit 0
fi

if [[ -z ${IN_NIX_SHELL:-} ]]; then
	exec nix-shell "${SHELL_NIX}" --run "$(printf '%q ' bash "$0" "$@")"
fi

if [[ ! -x ${DRGN_BIN} ]]; then
	echo "[INFO] 初始化 ${VENV_DIR}"
	uv venv "${VENV_DIR}"
	uv pip install --python "${VENV_DIR}/bin/python" drgn
fi

DRGN_LIBS="$(find "${VENV_DIR}/lib" -type d -name drgn.libs -print -quit)"
export LD_LIBRARY_PATH="${DRGN_LIBS}:${DRGN_NIX_LIBRARY_PATH:-}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

if [[ ${1} == "-c" && ${2:-} == "/proc/kcore" && ${EUID} -ne 0 ]]; then
	if sudo -n true 2>/dev/null; then
		exec sudo env LD_LIBRARY_PATH="${LD_LIBRARY_PATH}" "${DRGN_BIN}" "$@"
	fi

	printf 'a\n' | sudo -S env LD_LIBRARY_PATH="${LD_LIBRARY_PATH}" "${DRGN_BIN}" "$@"
	exit 0
fi

# set -x
# exec "${DRGN_BIN}"
# exec "${DRGN_BIN}" -s "$HOME"/vmlinux "$@"
exec "${DRGN_BIN}" "$@"
