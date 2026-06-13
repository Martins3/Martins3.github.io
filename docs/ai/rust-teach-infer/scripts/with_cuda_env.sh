#!/usr/bin/env bash
set -E -e -u -o pipefail

# shellcheck disable=SC2329
function prepend_path() {
	local dir="$1"
	local current="${2:-}"

	if [[ ! -d ${dir} ]]; then
		printf '%s' "${current}"
		return 0
	fi

	case ":${current}:" in
		*":${dir}:"*)
			printf '%s' "${current}"
			;;
		*)
			if [[ -n ${current} ]]; then
				printf '%s:%s' "${dir}" "${current}"
			else
				printf '%s' "${dir}"
			fi
			;;
	esac
}

function script_dir() {
	cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd
}

function project_root() {
	cd -- "$(script_dir)/.." && pwd
}

function ensure_runtime_link() {
	local source_path="$1"
	local target_path="$2"

	if [[ ! -e ${source_path} ]]; then
		echo "[FAIL] missing runtime library: ${source_path}" >&2
		exit 1
	fi

	ln -sfn "${source_path}" "${target_path}"
}

function ensure_cuda_runtime_dir() {
	local root="$1"
	local runtime_dir="${root}/.cuda-runtime"

	mkdir -p "${runtime_dir}"
	ensure_runtime_link "/usr/lib64/libcuda.so.1" "${runtime_dir}/libcuda.so.1"
	ensure_runtime_link \
		"/usr/lib64/libnvidia-ptxjitcompiler.so.1" \
		"${runtime_dir}/libnvidia-ptxjitcompiler.so.1"
	printf '%s' "${runtime_dir}"
}

function find_nix_gcc_lib() {
	# 查找 Nix 提供的 64 位 libstdc++，避免与宿主 glibc 混用导致崩溃
	local gcc_lib
	gcc_lib=$(find /nix/store -maxdepth 1 -name '*gcc-*-lib' -type d 2>/dev/null | while read -r d; do
		if file "${d}/lib/libstdc++.so.6.0.33" 2>/dev/null | grep -q "64-bit"; then
			echo "${d}"
			break
		fi
	done)
	if [[ -z ${gcc_lib} ]]; then
		echo "[FAIL] cannot find 64-bit Nix gcc lib" >&2
		exit 1
	fi
	printf '%s' "${gcc_lib}/lib"
}

function main() {
	if [[ $# -eq 0 ]]; then
		echo "usage: $0 <command> [args...]" >&2
		exit 1
	fi

	local root
	root="$(project_root)"
	local runtime_dir
		runtime_dir="$(ensure_cuda_runtime_dir "${root}")"

	# 自动检测 CUDA 环境（如果 env.sh 不存在）
	if [[ -z ${CUDA_HOME:-} ]]; then
		if [[ -d /usr/local/cuda-13.1 ]]; then
			export CUDA_HOME=/usr/local/cuda-13.1
		elif [[ -d /usr/local/cuda ]]; then
			export CUDA_HOME=/usr/local/cuda
		elif [[ -d /usr/local/cuda-12.8 ]]; then
			export CUDA_HOME=/usr/local/cuda-12.8
		else
			echo "[FAIL] CUDA_HOME not set and no default CUDA installation found" >&2
			exit 1
		fi
	fi

	export PATH="${CUDA_HOME}/bin:${PATH}"
	export NVCC_CCBIN="${NVCC_CCBIN:-/usr/bin/g++-14}"
	export LD_LIBRARY_PATH
	LD_LIBRARY_PATH="$(prepend_path "$(find_nix_gcc_lib)" "${LD_LIBRARY_PATH:-}")"
	LD_LIBRARY_PATH="$(prepend_path "${runtime_dir}" "${LD_LIBRARY_PATH}")"
	LD_LIBRARY_PATH="$(prepend_path "${CUDA_HOME}/lib64" "${LD_LIBRARY_PATH}")"

	echo "[OK] CUDA_HOME=${CUDA_HOME}"
	echo "[OK] CUDA runtime dir: ${runtime_dir}"
	echo "[OK] NVCC_CCBIN=${NVCC_CCBIN}"

	exec "$@"
}

main "$@"
