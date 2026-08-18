这个目录是关于 GPU 学习的，官方教程的 pdf 本地保存在 ~/data/cuda-programming-guide.pdf

编译 cuda 相关的东西，参考 ~/data/vn/gpu/cuda/tutorial/Makefile

https://docs.nvidia.com/cuda/cuda-programming-guide/index.html

## 代码规范

- 写代码默认使用中文注释
- 代码必须通过 clang-format 格式化后再提交
- 项目中已配置 `.clang-format`，根目录为 4 空格缩进，部分子目录（如 `01-cuda-platform`、`02-asynchronous-execution` 等）为 2 空格缩进
- 格式化命令：
```bash
find . -name "*.cu" -exec clang-format -i {} +
```
