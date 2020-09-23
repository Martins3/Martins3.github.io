---
title: ulmBLAS
date: 2018-04-19 18:09:37
tags: ku
---


# What's
[blas](https://en.wikipedia.org/wiki/Basic_Linear_Algebra_Subprograms)
in High Performance Computing we are interested in realizing numerical
algorithms in such a way that we get as close as possible to the **theoretical
peak performance** of the underlying hardware
如何计算理论值。

can we apply the tech used here to other project ?
**Intel Math Kernel** Library is a library of optimized math routines for science, engineering, and financial applications. Core math functions include BLAS, LAPACK, ScaLAPACK, sparse solvers, fast Fourier transforms, and vector math.[4] The routines in MKL are hand-optimized specifically for Intel processors.

That is because accessing the data from the memory becomes a bottleneck. Most of the time the CPU will be waiting for doing actual computations.


## what's it

## why we need it

## where we apply it

## which papers involved it

# Run it in your computer

# read the code

## points
makefile 实现对于多个模块的编译
[git 版本之间的切换](https://www.atlassian.com/git/tutorials/using-branches/git-checkout)
nm

格式化代码的工具

[ATLAS](http://math-atlas.sourceforge.net/) 是一个BLAS库
[Netlib](http://netlib.org/) is a collection of mathematical software, papers, and databases. 用于提供测试框架的

不是局部文件的头文件使用 "" 吗 ?


## key

## difficutl
1. 为什么需要编译两个文件出来

## new tech used

# summary

## why not 100%

## sort the techs by effiency order

# time costed !


# 遇到的问题
1. 编译出错, gcc版本不对, 应该没有必要使用gcc-4.8

# demo-pure-c
[gch](https://stackoverflow.com/questions/1241399/what-is-a-h-gch-file)
[dgemm](dgemm - perform one of the matrix-matrix operations    C  :=
     alpha*op( A )*op( B ) + beta*C)
dgemm为什么叫这一个名字

为什么必须保证对于 行列的维度是整除的关系 ？

level1 和 level3 分别表示什么 ?

非常奇怪的函数定义方法 ?
level3 中dgemm的两个同名函数

**All matrices can have arbitrary row and column stride**
Note that the pure C implementation works as long as mr is a divisor or mc and nr a divisor of nc.

*编译的过程中间，文件是如何流动的 ？*

写一个文件对于测试生成的链接库

在写一个智障版本的用于测试，就是简单的使用for 循环，然后进行调用，　查看一下分块的效果是什么

https://www.rapidtables.com/code/linux/gcc/gcc-i.html
[gcc mess](https://wiki.gentoo.org/wiki/GCC_optimization#-msse.2C_-msse2.2C_-msse3.2C_-mmmx.2C_-m3dnow)

有一种错觉，　那就是level1中间的内容完全和level3中间没有任何的联系，还有level2的文件夹在哪里?

为什么所有的位置都是需要添加incRow和incCol的，采用的原理是什么?

**C <- beta*C + alpha*A*B****

核心代码400行，其余的代码做什么的?

为什么需要padding, 如果根据之前的了解只有一个函数被头文件对外提供，那么是不是只要该dgemm\__nn没有使用到的函数都是做什么的?

pack_\_a 和　pack　b　需要创建出来两个函数出来。

# demo-naive-sse-with-intrisinc
发现使用clang编译居然会发生错误

处理micro kernel的处理，分块已经完成，加快矩阵乘法的计算。

```
    for (l=0; l<kc; ++l) {
      for (j=0; j<NR; ++j) {
          for (i=0; i<MR; ++i) {
              AB[i+j*MR] += A[i]*B[j];
          }
      }
      A += MR;
      B += NR;
    }
```

SSE 是什么?

把所有的版本全部的测试结果合并下来。

似乎用到了寄存器的东西(如果本电脑不支持怎么办)
｀#include <emmintrin.h>｀检查一下这一个头文件是什么情况
