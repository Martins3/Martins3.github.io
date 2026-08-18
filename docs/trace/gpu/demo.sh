#!/usr/bin/env bash
set -E -e -u -o pipefail

root_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
readonly root_dir
readonly cuda_bin=/usr/local/cuda-13.1/bin
readonly report_dir="$root_dir/reports"
readonly source_file="$root_dir/nvsys.cu"
readonly use_green_context=${USE_GREEN_CONTEXT:-1}
kernel="regex:bench_.*"

readonly binary="$root_dir/gpu_debug_demo.out"

readonly -a cuda_env=(env -u LD_LIBRARY_PATH -u LIBRARY_PATH -u NIX_LDFLAGS
	-u NIX_CFLAGS_COMPILE -u NIX_ENFORCE_NO_NATIVE
	"PATH=$cuda_bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin")

function run_nsys() {
	"${cuda_env[@]}" "$cuda_bin/nsys" profile --force-overwrite=true \
		--trace=cuda,nvtx,osrt --sample=none --cpuctxsw=none \
		--output="$report_dir/gpu-debug-demo" "$binary"
	"${cuda_env[@]}" "$cuda_bin/nsys" stats --force-export=true \
		--report=cuda_gpu_kern_sum "$report_dir/gpu-debug-demo.nsys-rep"
}

function run_ncu() {
	local tmp_dir

	tmp_dir=$(mktemp -d /tmp/ncu-demo.XXXXXX)
	sudo "${cuda_env[@]}" "$cuda_bin/ncu" --force-overwrite --set=basic \
		--kernel-name="$kernel" --launch-count=5 \
		--export="$tmp_dir/gpu-debug-demo" "$binary"
	sudo chown "$(id -u):$(id -g)" "$tmp_dir/gpu-debug-demo.ncu-rep"
	mv -f "$tmp_dir/gpu-debug-demo.ncu-rep" "$report_dir/gpu-debug-demo.ncu-rep"
	rmdir "$tmp_dir"
	"${cuda_env[@]}" "$cuda_bin/ncu" \
		--import "$report_dir/gpu-debug-demo.ncu-rep" --page=details
}

mkdir -p "$report_dir"
cd "$root_dir"
"${cuda_env[@]}" "$cuda_bin/nvcc" -ccbin=/usr/bin/g++ -O2 -lineinfo \
	-arch=sm_120 -DUSE_GREEN_CONTEXT="$use_green_context" "$source_file" -o "$binary"
# run_nsys
run_ncu
