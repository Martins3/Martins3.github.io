## 基本探索
https://mlir.llvm.org/ : middle level for accelerator programming language
https://www.zhihu.com/question/435109274/answer/3585914452 : MLIR 一些资源总结
	- https://zhuanlan.zhihu.com/p/708819963 : 从零开始教你写一个MLIR Pass
	- https://mlir.llvm.org/docs/Tutorials/Toy/ : 官方教程，直接看这个就没问题了


https://hackmd.io/lJZSI0pHSwGXzzL2z5vrbw : TileLang 完整解析：當 80 行 Python 打敗 500 行 CUDA，GPU 編程的新革命
https://zhuanlan.zhihu.com/p/1981042493769413822 :

## 基本操作

```
mlir/examples/toy/
├── Ch1/   # 第一章：Toy 语言 AST 与解析器
├── Ch2/   # 第二章：定义 Toy Dialect
├── Ch3/   # 第三章：IR 构建与验证
├── Ch4/   # 第四章：优化 Pass（形状推导等）
├── Ch5/   # 第五章：Lowering 到 Affine/LLVM
├── Ch6/   # 第六章：JIT 编译与执行
├── Ch7/   # 第七章：自定义类型与高级特性
└── CMakeLists.txt
```

llvm-project 中执行:

```bash
cmake -G Ninja -S llvm -B build \
  -DLLVM_ENABLE_PROJECTS="mlir" \
  -DLLVM_BUILD_EXAMPLES=ON \
  -DLLVM_TARGETS_TO_BUILD="Native" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON
```

```bash
ninja -C build toyc-ch1 toyc-ch2 toyc-ch3 toyc-ch4 toyc-ch5 toyc-ch6 toyc-ch7
```


编译产物位于 `build/bin/`。创建一个测试文件：

```toy
# test.toy
def main() {
  var a = [1, 2, 3];
  var b = [4, 5, 6];
  var c = a + b;
  print(c);
}
```

```bash
# Ch1: 输出 AST
./build/bin/toyc-ch1 test.toy --emit=ast

# Ch2: 输出 MLIR
./build/bin/toyc-ch2 test.toy --emit=mlir

# Ch3-Ch5: 输出 MLIR（支持更多 lowering 选项）
./build/bin/toyc-ch5 test.toy --emit=mlir

# Ch6/Ch7: JIT 执行（输出计算结果）
./build/bin/toyc-ch6 test.toy --emit=jit
# => 5.000000 7.000000 9.000000

./build/bin/toyc-ch7 test.toy --emit=jit
# => 5.000000 7.000000 9.000000
```

如果需要构建完整的 MLIR 工具链（包含 `mlir-opt`、`mlir-translate` 等）：

```bash
ninja -C build
```


## MLIR 与 nvcc 的关系
<!-- 3b457321-fa66-4dae-8cdd-94299afc2a01 -->

MLIR 和 `nvcc` 是**不同层次、不同定位**的编译器组件，没有直接的隶属关系，但可以通过 LLVM 后端间接协作。




### 定位对比

| | **MLIR** | **nvcc** |
|---|---|---|
| **本质** | 中间表示（IR）框架 + 编译器基础设施 | NVIDIA CUDA 编译器驱动程序 |
| **输入** | 各种 Dialect（如 `linalg`、`scf`、`llvm`） | CUDA C/C++（`.cu` 文件） |
| **输出** | 更低层的 Dialect 或 LLVM IR | PTX（GPU 汇编）或 SASS（机器码） |
| **归属** | LLVM 开源项目 | NVIDIA 闭源/开源混合（基于 LLVM） |

---

### 两者如何产生关联

1. 共同基于 LLVM 后端

`nvcc` 底层使用 **LLVM/NVPTX 后端** 将 LLVM IR 编译为 PTX。
MLIR 也可以通过 `LLVM Dialect` → LLVM IR → NVPTX 后端，**最终同样生成 PTX**。

```
CUDA C/C++ → nvcc → LLVM IR → NVPTX → PTX → SASS
                        ↑
MLIR IR → LLVM Dialect ─┘
```

2. MLIR 生态中的 CUDA/ GPU 编译路径

MLIR 不直接调用 `nvcc`，但它有自己的 GPU 编译流水线：

| 组件 | 作用 |
|------|------|
| `GPU Dialect` | 表示 host + device 代码结构（`gpu.launch` 等） |
| `NVVM Dialect` | 表示 NVIDIA GPU 特定的 intrinsic |
| `ROCDL Dialect` | AMD GPU 对应物 |
| `mlir-translate` | 将 IR 导出为 LLVM IR |
| `llc` / `clang` | 用 LLVM 工具链生成 PTX/二进制 |

**典型流程**：

```mlir
// 高层 MLIR (如 linalg.matmul)
↓ lowering
gpu.func @kernel(...) { ... }   // GPU Dialect
↓
nvvm.kernel @kernel { ... }     // NVVM Dialect
↓ translate
LLVM IR
↓ llc -mcpu=sm_80
PTX 代码
```

3. NVIDIA 内部也在探索 MLIR

NVIDIA 参与了 MLIR 社区（如 `NVVM Dialect` 的维护），在一些内部项目中使用 MLIR 技术栈，但 **`nvcc` 本身目前仍是传统的 Clang/LLVM 前端驱动，不直接暴露 MLIR 接口**。

---

### 一个实际的工业场景

以 **Triton**（OpenAI 的 GPU 内核生成语言）为例：

1. Python Triton 代码 → Triton IR（基于 MLIR 风格设计）
2. 经过 MLIR 风格的优化 Pass
3. 生成 LLVM IR
4. 调用 LLVM NVPTX 后端生成 PTX
5. **可选**：用 `ptxas`（NVIDIA 汇编器）生成 SASS，或让驱动 JIT 编译

这个过程中：
- 用了 MLIR 做高层优化和 IR 变换
- 最终通过 LLVM 生成 PTX（和 `nvcc` 的后半段路径重合）
- **不需要也不调用 `nvcc`**

### 总结

| 问题 | 答案 |
|------|------|
| MLIR 能替代 nvcc 吗？ | 不能直接替代。MLIR 是 IR 框架，`nvcc` 是完整的前端驱动。但 MLIR 可以绕过 `nvcc`，直接生成 GPU 代码 |
| 它们会竞争吗？ | 在 AI 编译器领域，**IREE/Torch-MLIR/Triton** 等 MLIR 项目正在减少开发者对手写 CUDA + `nvcc` 的依赖 |
| 关系最准确的描述？ | **间接协作关系**：MLIR 负责高层 IR 优化，最终通过 LLVM NVPTX 后端生成代码，而 `nvcc` 是另一条独立的、从 C++ 出发的路径 |

简单来说：**`nvcc` 是 NVIDIA 的 C++ 编译器，MLIR 是编译器构建框架**——MLIR 可以生成 `nvcc` 最终也能生成的 PTX，但走的是完全不同的技术路径。
