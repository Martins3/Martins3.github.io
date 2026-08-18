#!/usr/bin/env bash
set -E -e -u -o pipefail

PROGDIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
# ============================================================
# TileLang Demo 一键运行脚本 (NixOS + GTX 1060 适配版)
# 用法: bash /home/martins3/data/vn/a.sh
# 功能: 自动建 venv、装依赖、修 bug、设环境变量、跑 demo
# ============================================================

PROJECT_ROOT="$PROGDIR"
VENV_PATH="$PROJECT_ROOT/.venv"

function setup_env() {

	# --------------------------------------------------
	# 0. 先设置 LD_LIBRARY_PATH (NixOS 必须，否则 torch 无法导入)
	# --------------------------------------------------
	CUDA_LIB_DIR="/tmp/cuda_lib"
	if [[ ! -d $CUDA_LIB_DIR ]]; then
		echo "[0/6] 创建 CUDA 驱动库软链接..."
		mkdir -p "$CUDA_LIB_DIR"
		ln -sf /lib64/libcuda.so.1 "$CUDA_LIB_DIR/" 2>/dev/null || true
		ln -sf /lib64/libcuda.so.580.142 "$CUDA_LIB_DIR/" 2>/dev/null || true
		ln -sf /usr/lib64/libnvidia-ptxjitcompiler.so.1 "$CUDA_LIB_DIR/" 2>/dev/null || true
		ln -sf /usr/lib64/libnvidia-ptxjitcompiler.so.580.142 "$CUDA_LIB_DIR/" 2>/dev/null || true
	else
		echo "[0/6] CUDA 驱动库软链接已存在，跳过"
	fi

	GCC_LIB_PATH=$(find /nix/store -maxdepth 1 -name '*gcc-10.3.0-lib' -type d 2>/dev/null | head -1)
	if [[ -z $GCC_LIB_PATH ]]; then
		GCC_LIB_PATH=$(find /nix/store -maxdepth 1 -name '*gcc-*-lib' -type d 2>/dev/null | head -1)
	fi
	if [[ -z $GCC_LIB_PATH ]]; then
		echo "错误: 找不到 Nix store 中的 gcc lib 路径"
		exit 1
	fi
	export LD_LIBRARY_PATH="$GCC_LIB_PATH/lib:$CUDA_LIB_DIR:${LD_LIBRARY_PATH:-}"

	# --------------------------------------------------
	# 1. 创建虚拟环境
	# --------------------------------------------------
	if [[ ! -d $VENV_PATH ]]; then
		echo "[1/6] 创建虚拟环境..."
		python3 -m venv "$VENV_PATH"
		"$VENV_PATH/bin/pip" install --upgrade pip
	else
		echo "[1/6] 虚拟环境已存在，跳过"
	fi

	source "$VENV_PATH/bin/activate"

	# --------------------------------------------------
	# 2. 安装 PyTorch (cu118 版本，适配 sm_61/GTX 1060)
	# --------------------------------------------------
	# 必须在 /tmp 下 import，避免项目根目录的 tilelang/ 干扰
	TORCH_OK=$(cd /tmp && python -c "import torch; v=torch.__version__; print('OK' if 'cu118' in v else v)" 2>/dev/null || echo "NO")
	if [[ $TORCH_OK != "OK" ]]; then
		echo "[2/6] 安装 PyTorch 2.5.1+cu118..."
		pip install torch==2.5.1+cu118 --index-url https://download.pytorch.org/whl/cu118
	else
		echo "[2/6] PyTorch cu118 已安装，跳过"
	fi

	# --------------------------------------------------
	# 3. 安装 tilelang
	# --------------------------------------------------
	TILELANG_OK=$(cd /tmp && python -c "import tilelang; print('OK')" 2>/dev/null || echo "NO")
	if [[ $TILELANG_OK != "OK" ]]; then
		echo "[3/6] 安装 tilelang..."
		pip install tilelang
	else
		echo "[3/6] tilelang 已安装，跳过"
	fi

	# --------------------------------------------------
	# 4. 修复 tilelang bfloat16 编译 bug (sm_61 必需)
	# --------------------------------------------------
	SITE=$(cd /tmp && python -c "import tilelang; print(tilelang.__path__[0])" 2>/dev/null)
	COMMON_H="$SITE/src/tl_templates/cuda/common.h"
	echo 0

	if [[ -f $COMMON_H ]] && grep -q "TL_DEVICE __nv_bfloat162 fma2" "$COMMON_H"; then
		# 检查是否已经修复过
		if ! grep -q "#if defined(__CUDA_ARCH__) && (__CUDA_ARCH__ >= 800)" "$COMMON_H"; then
			echo "[4/6] 修复 tilelang bfloat16 编译 bug..."
			sed -i 's/TL_DEVICE __nv_bfloat162 fma2(__nv_bfloat162 a, __nv_bfloat162 b,/#if defined(__CUDA_ARCH__) \&\& (__CUDA_ARCH__ >= 800)\nTL_DEVICE __nv_bfloat162 fma2(__nv_bfloat162 a, __nv_bfloat162 b,/' "$COMMON_H"
			sed -i 's/  return __nv_bfloat162{__hfma(a.x, b.x, c.x), __hfma(a.y, b.y, c.y)};/  return __hfma2(a, b, c);\n}\n#endif/' "$COMMON_H"
		else
			echo "[4/6] bfloat16 bug 已修复，跳过"
		fi
	else
		echo "[4/6] 未找到 common.h 或已更新，跳过"
	fi

	# --------------------------------------------------
	# 5. 验证 torch + CUDA
	# --------------------------------------------------
	echo "[5/6] 验证 torch CUDA 可用性..."
	cd /tmp
	python -c "import torch; assert torch.cuda.is_available(), 'CUDA 不可用'; print('torch:', torch.__version__, '| CUDA:', torch.cuda.is_available())"
}

function run_demo() {
	# --------------------------------------------------
	# 6. 运行 Demo
	# --------------------------------------------------
	echo "[6/6] 开始运行 TileLang Demo"
	echo ""

	# Demo 1: 逐元素加法 (elementwise_add)
	cp "$PROGDIR"/elementwise_demo.py /tmp/elementwise_demo.py
	python elementwise_demo.py

	# Demo 2: Online Softmax
	cp "$PROGDIR/softmax_demo.py" /tmp/softmax_demo.py
	python softmax_demo.py
}

setup_env
run_demo
