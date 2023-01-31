首先，能不能搞定在自己电脑上进行GPU 编程的难题 ?

## 资源
- [推荐几个不错的CUDA入门教程](https://zhuanlan.zhihu.com/p/346910129)
1. https://news.ycombinator.com/item?id=22880502
2. https://news.ycombinator.com/item?id=22941224 : rust gpu 编程库
3. https://github.com/EmbarkStudios/rust-gpu
- https://github.com/vosen/ZLUDA : ZLUDA is a drop-in replacement for CUDA on Intel GPU.
- https://github.com/Tony-Tan/CUDA_Freshman : 中文 cuda 教程，使用腾讯云做实验
- https://github.com/stackgl/shader-school : 在网页中运行的 GLSL
- https://github.com/parallel101/course : b 站上上课，每节课都是都是存在配套作业，非常有必要学一下
- [2D Graphics on Modern GPU](https://raphlinus.github.io/rust/graphics/gpu/2019/05/08/modern-2d.html)
- [两种 GPU 架构对比](https://www.rastergrid.com/blog/gpu-tech/2021/07/gpu-architecture-types-explained/)

教练，我想写一个 GPU :
- [NyuziProcessor](https://github.com/jbush001/NyuziProcessor)
  - https://www.veripool.org/verilator/

## 似乎存在不少 GPU 封装的语言
- https://github.com/diku-dk/futhark
- https://github.com/halide/Halide

## 问题
- [ ]  想大致的知道 Liunx 和 Windows 的图形栈是什么

## 收集一些材料来写理解图形栈的工作原理
- https://github.com/ocornut/imgui : 这个工作非常有意思，其代码简单，而且支持各种各样的后端，并且在其基础上开发语言的开发库
  - 后端: dearpygui
- https://tauri.app/v1/guides/getting-started/setup/html-css-js
  - 号称是 electorn 的替代品

- https://siboehm.com/articles/22/CUDA-MMM
