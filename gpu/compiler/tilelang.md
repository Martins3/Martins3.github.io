## tilelang 的基本介绍
TileLang

MSRA 等开发的张量编译器（MSys 相关）：

• 用于生成高性能 GPU/CPU/加速器内核的领域特定语言
• 支持 CUDA、HIP、Apple Metal、WebGPU、华为昇腾等后端
• 相比 Triton 在部分场景可实现 1.1-1.4 倍性能提升
• 已支持 FlashMLA、稀疏矩阵运算等高级功能

TileLang                    张量编译器        MSRA 等                分块调度编译          支持多硬件后端(CUDA/HIP/Metal/昇腾)，MLA 80 行实现


https://zhuanlan.zhihu.com/p/1981042493769413822 : 饭后闲聊 GPU 高性能算子编程新势力：cuTile、Triton 与 TileLang 的深度对话 (1)
	- 基本是对的吧

## 基本操作

没想到的事情是，居然占用显存的就是 python 的:
```txt
pstree -t -p  899087
python3.13(899087)─┬─{cuda-EvtHandlr}(899125)
                   ├─{cuda00001400006}(899120)
                   ├─{python3.13}(899088)
                   ├─{python3.13}(899089)
                   ├─{python3.13}(899090)
                   ├─{python3.13}(899091)
                   ├─{python3.13}(899092)
                   ├─{python3.13}(899093)
                   ├─{python3.13}(899094)
                   ├─{python3.13}(899095)
                   ├─{python3.13}(899096)
                   ├─{python3.13}(899097)
                   ├─{python3.13}(899098)
                   ├─{python3.13}(899099)
                   ├─{python3.13}(899100)
                   ├─{python3.13}(899101)
                   ├─{python3.13}(899102)
                   ├─{python3.13}(899103)
                   ├─{python3.13}(899104)
                   ├─{python3.13}(899105)
                   ├─{python3.13}(899106)
                   ├─{python3.13}(899107)
                   ├─{python3.13}(899108)
                   ├─{python3.13}(899109)
                   ├─{python3.13}(899110)
                   ├─{python3.13}(899111)
                   ├─{python3.13}(899112)
                   ├─{python3.13}(899113)
                   ├─{python3.13}(899114)
                   ├─{python3.13}(899115)
                   ├─{python3.13}(899116)
                   ├─{python3.13}(899117)
                   ├─{python3.13}(899118)
                   └─{python3.13}(899126)
```

## 编译过程

在 TileLang 里，你用 Python 写的 kernel 并不是直接跑在 Python 解释器里的，而是会经过一条完整的编译流水线，最终变成 GPU 上真正执行的机器码。具体来说：
1. 先变成 CUDA C++ 源码
你的 Python 代码（TileLang DSL）首先会被 lower 成 TVM TIR（一种中间表示），然后代码生成器会把它翻译成 CUDA C++ 源代码（.cu 风格，包含 __global__ kernel）。你可以随时把它打印出来看：
```txt
kernel = matmul_relu.compile(a, b)
print(kernel.get_kernel_source())   # 输出 CUDA C++ 代码
```
2. 再编译成 PTX / SASS（最终可执行形式）
生成的 CUDA C++ 源码会由 NVCC 或 NVRTC 进一步编译：
- PTX（Parallel Thread Execution）：NVIDIA GPU 的中间汇编指令。TileLang 也提供了接口可以直接导出：
```txt
  kernel.show_ptx()        # 查看 PTX
  kernel.export_ptx("/tmp/kernel.ptx")
```
- SASS（Streaming Assembler）：PTX 再经过汇编后生成的真正 GPU 机器码。同样可以导出查看：
```txt
  kernel.show_sass()       # 查看 SASS
```
3. 运行时的两种方式
• tvm_ffi / cython backend：TVM 负责把 CUDA C++ 编译成 cubin/PTX，通过自身的 runtime 加载执行。
• nvrtc backend：在 Python 进程内通过 NVRTC 把 CUDA C++ 源码实时编译成 PTX，再用 CUDA Driver API（cuLaunchKernelEx）直接发射到 GPU 上。
总结 Python → TVM TIR → CUDA C++ 源码 → PTX → SASS（GPU 机器码）

所以虽然你写的是 Python，但最终落到 GPU 上的还是和手写 CUDA 一样的路径：先出 C++ 源码，再编译成 PTX/SASS，然后在 NVIDIA 驱动上执行。TileLang 只是帮你自
动完成了前面那些繁琐的优化和代码生成步骤。
