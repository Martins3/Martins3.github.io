# NVCC `-v` 输出逐行解析

> 命令：`/usr/local/cuda/bin/nvcc -ccbin /usr/bin/g++-14 -std=c++17 -arch=sm_120 -O2 -v chapter_demo.cu -o chapter_demo_v.out`

---

## 阶段 0：NVCC 环境初始化

```
#$ _NVVM_BRANCH_=nvvm
#$ _SPACE_=
#$ _CUDART_=cudart
#$ _HERE_=/usr/local/cuda/bin
#$ _THERE_=/usr/local/cuda/bin
#$ TOP=/usr/local/cuda/bin/..
#$ CICC_PATH=/usr/local/cuda/bin/../nvvm/bin
#$ NVVMIR_LIBRARY_DIR=/usr/local/cuda/bin/../nvvm/libdevice
#$ LD_LIBRARY_PATH=/usr/local/cuda/bin/../lib:
#$ PATH=/usr/local/cuda/bin/../nvvm/bin:/usr/local/cuda/bin:...
#$ INCLUDES="-I/usr/local/cuda/bin/../targets/x86_64-linux/include"
#$ LIBRARIES=  "-L/usr/local/cuda/bin/../targets/x86_64-linux/lib/stubs" "-L/usr/local/cuda/bin/../targets/x86_64-linux/lib"
```

NVCC 设置内部环境变量，包括：
• CICC_PATH：设备代码编译器 cicc 的位置
• NVVMIR_LIBRARY_DIR：NVVM 库设备目录（内含 libdevice.10.bc）
• INCLUDES / LIBRARIES：CUDA 头文件和库路径

────────────────────────────────────────────────────────────────────────────────

## 阶段 1：预处理（Host 路径）

```
#$ "/usr/bin"/g++-14 -std=c++17 ... -E -x c++ -D__CUDACC__ -D__NVCC__ ... -include "cuda_runtime.h" -m64 "chapter_demo.cu" -o "/tmp/...-5_chapter_demo.cpp4.ii"
```

• 工具：g++-14 -E（仅预处理，不编译）
• 输入：chapter_demo.cu
• 输出：.cpp4.ii（预处理后的 C++ 中间文件，包含所有宏展开和头文件内容）
• 关键标志：-D__CUDACC__ / -D__NVCC__ 标识这是 CUDA 编译上下文

────────────────────────────────────────────────────────────────────────────────

## 阶段 2：CUDA 前端分离（cudafe++）

```
#$ cudafe++ --c++17 --gnu_version=140201 ... --gen_c_file_name "/tmp/...-6_chapter_demo.cudafe1.cpp" --stub_file_name "tmpxft_...-6_chapter_demo.cudafe1.stub.c" --gen_module_id_file ... "/tmp/...-5_chapter_demo.cpp4.ii"
```

• 工具：cudafe++（CUDA Frontend）
• 作用：将 .cu 文件中的 Host 代码 和 Device 代码 分离
• 输出：
  • .cudafe1.cpp：分离后的 Host 代码
  • .cudafe1.stub.c：Device 代码存根（供 Host 代码调用）
  • .module_id：模块标识文件

────────────────────────────────────────────────────────────────────────────────

## 阶段 3：预处理（Device 路径）

```
#$ "/usr/bin"/g++-14 -std=c++17 -D__CUDA_ARCH__=610 ... -E -x c++ -DCUDA_DOUBLE_MATH_FUNCTIONS ... "chapter_demo.cu" -o "/tmp/...-9_chapter_demo.cpp1.ii"
```

• 工具：再次调用 g++-14 -E
• 区别：这次带 -D__CUDA_ARCH__=610，为设备代码预处理
• 输出：.cpp1.ii（设备代码专用的预处理结果）

────────────────────────────────────────────────────────────────────────────────

## 阶段 4：设备代码编译（cicc -> PTX）

```
#$ "$CICC_PATH/cicc" --c++17 ... -arch compute_61 -m64 ... --gen_device_file_name "/tmp/...-6_chapter_demo.cudafe1.gpu" "/tmp/...-9_chapter_demo.cpp1.ii" -o "/tmp/...-6_chapter_demo.ptx"
```

• 工具：cicc（CUDA Intermediate Code Compiler，基于 LLVM/NVVM）
• 输入：.cpp1.ii
• 输出：
  • .ptx：PTX 虚拟汇编代码（Parallel Thread Execution，文本格式，具备前向兼容性）
  • .cudafe1.gpu：设备代码中间表示

────────────────────────────────────────────────────────────────────────────────

## 阶段 5：PTX 汇编（ptxas -> Cubin）

```
#$ ptxas -arch=sm_120 -m64 "/tmp/...-6_chapter_demo.ptx" -o "/tmp/...-10_chapter_demo.sm_120.cubin"
```

• 工具：ptxas（PTX Assembler）
• 输入：.ptx
• 输出：.sm_120.cubin（真实 GPU 二进制机器码，针对 sm_120 架构）

────────────────────────────────────────────────────────────────────────────────

## 阶段 6：打包 Fatbin（第一次）

```
#$ fatbinary -64 ... "--image3=kind=elf,sm=120,file=/tmp/...-10_chapter_demo.sm_120.cubin" "--image3=kind=ptx,sm=120,file=/tmp/...-6_chapter_demo.ptx" --embedded-fatbin="/tmp/...-3_chapter_demo.fatbin.c"
#$ rm /tmp/...-3_chapter_demo.fatbin
```

• 工具：fatbinary
• 作用：将 Cubin（elf）和 PTX 打包成 Fatbin
• 输出：.fatbin.c（C 语言数组形式，可直接嵌入到 Host 代码中）

删除临时二进制文件。

────────────────────────────────────────────────────────────────────────────────

## 阶段 7：编译 Host 代码

```
#$ "/usr/bin"/g++-14 -std=c++17 ... -c -x c++ ... "/tmp/...-6_chapter_demo.cudafe1.cpp" -o "/tmp/...-11_chapter_demo.o"
```

• 工具：g++-14 -c（编译，不链接）
• 输入：.cudafe1.cpp（阶段 2 分离出的 Host 代码）
• 输出：.o（Host 目标文件，含 Fatbin 嵌入数据）

────────────────────────────────────────────────────────────────────────────────

## 阶段 8：设备代码链接（nvlink）

```
#$ nvlink -m64 --arch=sm_120 --register-link-binaries="/tmp/...-7_chapter_demo_v_dlink.reg.c" ... "/tmp/...-11_chapter_demo.o" -lcudadevrt -o "/tmp/...-12_chapter_demo_v_dlink.sm_120.cubin" --host-ccbin "/usr/bin/g++-14"
```

• 工具：nvlink（CUDA 设备链接器）
• 作用：链接设备代码中的符号（如跨文件的 __global__ 函数）
• 输出：
  • _dlink.sm_120.cubin：链接后的设备二进制
  • .reg.c：注册的链接信息

────────────────────────────────────────────────────────────────────────────────

## 阶段 9：打包设备链接产物（第二次 fatbinary）

```
#$ fatbinary -64 ... -link "--image3=kind=elf,sm=120,file=/tmp/...-12_chapter_demo_v_dlink.sm_120.cubin" --embedded-fatbin="/tmp/...-8_chapter_demo_v_dlink.fatbin.c"
#$ rm /tmp/...-8_chapter_demo_v_dlink.fatbin
```

再次调用 fatbinary，打包设备链接后的 Cubin。

清理临时文件。

────────────────────────────────────────────────────────────────────────────────

## 阶段 10：编译链接桩文件

```
#$ "/usr/bin"/g++-14 -std=c++17 ... -c -x c++ -DFATBINFILE="\"/tmp/...-8_chapter_demo_v_dlink.fatbin.c\"" -DREGISTERLINKBINARYFILE="\"/tmp/...-7_chapter_demo_v_dlink.reg.c\"" ... "/usr/local/cuda/bin/crt/link.stub" -o "/tmp/...-13_chapter_demo_v_dlink.o"
```

• 工具：g++-14 -c
• 输入：link.stub（CUDA 运行时链接桩）+ 设备链接产物
• 输出：_dlink.o（包含设备代码注册逻辑的目标文件）

────────────────────────────────────────────────────────────────────────────────

## 阶段 11：最终链接

```
#$ "/usr/bin"/g++-14 ... -Wl,--start-group "/tmp/...-13_chapter_demo_v_dlink.o" "/tmp/...-11_chapter_demo.o" ... -lcudadevrt -lcudart_static -lrt -lpthread -ldl -Wl,--end-group -o "chapter_demo_v.out"
```

• 工具：g++-14（链接器）
• 输入：
  • _dlink.o：设备代码注册桩
  • chapter_demo.o：Host 代码目标文件
  • -lcudadevrt：CUDA 设备运行时
  • -lcudart_static：CUDA 运行时（静态链接）
  • -lrt -lpthread -ldl：系统库依赖
• 输出：chapter_demo_v.out（最终可执行文件）

────────────────────────────────────────────────────────────────────────────────

## 流程总结图

```
chapter_demo.cu
      |
      v
[ g++ -E ]  ----> .cpp4.ii
      |
      v
[cudafe++]  ----> .cudafe1.cpp (Host)  +  .cudafe1.stub.c
      |
      +---> [ g++ -E ] ---> .cpp1.ii
                  |
                  v
              [ cicc ] ---> .ptx (PTX 虚拟汇编)
                  |
                  v
              [ptxas] ---> .sm_120.cubin (GPU 机器码)
                  |
                  v
            [fatbinary] ---> .fatbin.c (嵌入 Host)
                  |
      +-----------+
      |
      v
[ g++ -c ] ---> chapter_demo.o (Host 目标文件)
      |
      v
  [nvlink] ---> _dlink.sm_120.cubin (设备链接)
      |
      v
[fatbinary] ---> _dlink.fatbin.c
      |
      v
[ g++ -c ] ---> _dlink.o (链接桩)
      |
      +-------> [ g++ 链接 ] ---> chapter_demo_v.out
```
