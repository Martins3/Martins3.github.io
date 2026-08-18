# CUTLASS Makefile Demo

这个目录放 CUTLASS demo，用 Makefile 验证不同接入方式。Makefile 会自动发现
当前目录下的 `*.cu` 文件，并把每个 demo 编译成 `build/<name>.out`。

- `MODE=local`：默认模式，使用 `~/data/cutlass` 源码和本机已构建的
  `build-cmake-sm120-cuda12.8-tools/tools/library/libcutlass.so`
- `MODE=system`：使用系统安装路径里的 CUTLASS 头文件和 `libcutlass.so`
- `MODE=source`：只使用下载好的 CUTLASS 源码树头文件，不链接 `libcutlass.so`

默认构建并运行：

```bash
make
make run
```

当前已有 demo：

```text
sgemm_demo.cu -> build/sgemm_demo.out
```

运行指定 demo：

```bash
make run DEMO=sgemm_demo
```

传入 demo 参数：

```bash
make run DEMO=sgemm_demo RUN_ARGS="256 256 256"
```

等价显式写法：

```bash
make MODE=local \
  LOCAL_CUTLASS_ROOT=~/data/cutlass \
  LOCAL_CUTLASS_BUILD=~/data/cutlass/build-cmake-sm120-cuda12.8-tools
```

系统安装版示例：

```bash
make clean
make MODE=system CUTLASS_PREFIX=/usr/local
```

如果 `libcutlass.so` 不在 `$CUTLASS_PREFIX/lib64`，显式指定：

```bash
make MODE=system CUTLASS_PREFIX=/opt/cutlass CUTLASS_LIBDIR=/opt/cutlass/lib
```

下载源码树但没有安装 library 时：

```bash
make clean
make MODE=source CUTLASS_ROOT=~/data/cutlass
```

当前 Nix/glibc 环境下，`MODE=source run` 会通过
`LD_PRELOAD=/usr/lib64/libcuda.so.1` 显式加载真实 NVIDIA driver library，避免
CUDA runtime 找不到 driver library 后报 `CUDA driver version is insufficient
for CUDA runtime version`。
