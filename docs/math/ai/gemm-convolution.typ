#set heading(numbering: "1.")
#set text(font: "Noto Sans CJK SC", size: 11pt)
#set math.equation(numbering: "(1)")

#align(center)[#text(size: 20pt, weight: "bold")[GEMM Convolution]]

GEMM convolution 的核心说法是对的：

#quote[
把卷积中的大量 patch 与 filter 的点积，重新排列成一次矩阵乘法。
]

这里需要先说明一个小细节：深度学习框架里常说的 convolution，通常实际计算的是 cross-correlation，也就是不翻转卷积核。下面沿用深度学习里的习惯，仍然叫 convolution。

= 一个数值例子

这个例子只有一张输入图片、一个输入通道，所以可以先把输入写成一个二维矩阵。对应到完整符号里，就是 $N = 1, H = 4, W = 4, C = 1$。

令输入为：

$ I =
  mat(
    1, 2, 3, 4;
    5, 6, 7, 8;
    9, 10, 11, 12;
    13, 14, 15, 16
  ) $

先只看一个输出通道，也就是 $K = 1$。令 filter 为：

$ F =
  mat(
    1, 0, -1;
    1, 0, -1;
    1, 0, -1
  ) $

左上角输出位置 $(p=0,q=0)$ 对应的 input patch 是：

$ X =
  mat(
    1, 2, 3;
    5, 6, 7;
    9, 10, 11
  ) $

这个输出点就是 patch 和 filter 的点积：

$ O_(0,0,0,0)
  = 1 dot 1 + 2 dot 0 + 3 dot (-1)
  + 5 dot 1 + 6 dot 0 + 7 dot (-1)
  + 9 dot 1 + 10 dot 0 + 11 dot (-1)
  = -6 $

如果用 GEMM 表达，先把这个 patch 展成 $A$ 的第一行：

$ A_(0,:) = [1, 2, 3, 5, 6, 7, 9, 10, 11] $

把 filter 展成 $B$ 的第一列：

$ B_(:,0) = [1, 0, -1, 1, 0, -1, 1, 0, -1]^T $

于是：

$ G_(0,0) = A_(0,:) B_(:,0) = -6 $

第二个输出元素对应 $(p=0,q=1)$，也就是窗口向右移动一格：

$ X_1 =
  mat(
    2, 3, 4;
    6, 7, 8;
    10, 11, 12
  ) $

所以：

$ O_(0,0,1,0)
  = 2 dot 1 + 3 dot 0 + 4 dot (-1)
  + 6 dot 1 + 7 dot 0 + 8 dot (-1)
  + 10 dot 1 + 11 dot 0 + 12 dot (-1)
  = -6 $

用 GEMM 看，就是 $A$ 的第二行乘以 $B$：

$ A_(1,:) = [2, 3, 4, 6, 7, 8, 10, 11, 12] $

$ G_(1,0) = A_(1,:) B_(:,0) = -6 $

这个例子中一共有 $2 times 2 = 4$ 个输出空间位置，所以完整的 $A$ 是：

$ A =
  mat(
    1, 2, 3, 5, 6, 7, 9, 10, 11;
    2, 3, 4, 6, 7, 8, 10, 11, 12;
    5, 6, 7, 9, 10, 11, 13, 14, 15;
    6, 7, 8, 10, 11, 12, 14, 15, 16
  ) $

完整的 $B$ 是：

$ B =
  mat(
    1;
    0;
    -1;
    1;
    0;
    -1;
    1;
    0;
    -1
  ) $

于是完整的 GEMM 是：

$ G = A B =
  mat(
    1, 2, 3, 5, 6, 7, 9, 10, 11;
    2, 3, 4, 6, 7, 8, 10, 11, 12;
    5, 6, 7, 9, 10, 11, 13, 14, 15;
    6, 7, 8, 10, 11, 12, 14, 15, 16
  )
  mat(
    1;
    0;
    -1;
    1;
    0;
    -1;
    1;
    0;
    -1
  )
  =
  mat(
    -6;
    -6;
    -6;
    -6
  ) $

最后把 $G$ 的 4 行还原成输出的 $(p,q)$ 空间位置：

$ O_(0,:,:,0) =
  mat(
    -6, -6;
    -6, -6
  ) $

其他输出位置也是同样的点积，只是换成不同的 input patch。如果有多个输出通道 $K$，就是让 $B$ 有多列；一次矩阵乘法同时算出多个 filter 的结果。


= 卷积公式

假设输入采用 NHWC 布局：

$ I in RR^(N times H times W times C) $

NHWC 表示输入张量的四个维度顺序是：

- $N$：batch size，也就是一次处理多少张图片
- $H$：input height，输入特征图高度
- $W$：input width，输入特征图宽度
- $C$：input channels，输入通道数

所以 $I_(n,h,w,c)$ 表示第 $n$ 个样本、第 $h$ 行、第 $w$ 列、第 $c$ 个输入通道的值。

卷积核采用 RSCK 布局：

$ F in RR^(R times S times C times K) $

RSCK 表示 filter 张量的四个维度顺序是：

- $R$：filter height，卷积核高度
- $S$：filter width，卷积核宽度
- $C$：input channels，和输入张量的 $C$ 对齐
- $K$：output channels，也就是卷积核个数

所以 $F_(r,s,c,k)$ 表示第 $k$ 个 filter 在位置 $(r,s)$、输入通道 $c$ 上的权重。

输出为：

$ O in RR^(N times P times Q times K) $

这里可以把输出布局记成 NPQK：

- $N$：batch size，和输入的 $N$ 一样
- $P$：output height，输出特征图高度
- $Q$：output width，输出特征图宽度
- $K$：output channels，和 filter 的 $K$ 一样

所以 $O_(n,p,q,k)$ 表示第 $n$ 个样本、第 $p$ 行、第 $q$ 列、第 $k$ 个输出通道的值。

如果 stride 为 $u, v$，padding 为 $a_h, a_w$，dilation 为 $d_h, d_w$，那么输出空间大小是：

$ P = floor((H + 2 a_h - d_h (R - 1) - 1) / u) + 1 $

$ Q = floor((W + 2 a_w - d_w (S - 1) - 1) / v) + 1 $

每个输出元素是：

$ O_(n,p,q,k) =
  sum_(r=0)^(R - 1)
  sum_(s=0)^(S - 1)
  sum_(c=0)^(C - 1)
  I_(n, p u + r d_h - a_h, q v + s d_w - a_w, c)
  F_(r,s,c,k) $

其中越过输入边界的位置按 padding 规则处理，最常见的是补 0。

这个公式表达的是：固定一个输出位置 $(n,p,q,k)$ 后，把输入中的一个局部 patch 和第 $k$ 个 filter 做点积。

= 从卷积到矩阵乘法

GEMM 是通用矩阵乘法：

$ C = A B $

要把卷积改写成 GEMM，可以把输出的前三个维度 $(n,p,q)$ 展平成矩阵行，把卷积核内部的三个维度 $(r,s,c)$ 展平成矩阵的归约维度。

定义：

$ m = (n P + p) Q + q $

$ ell = (r S + s) C + c $

于是逻辑矩阵 $A$、$B$、$G$ 的 shape 是：

$ A in RR^((N P Q) times (R S C)) $

$ B in RR^((R S C) times K) $

$ G in RR^((N P Q) times K) $

它们的元素对应关系是：

$ A_(m,ell) =
  I_(n, p u + r d_h - a_h, q v + s d_w - a_w, c) $

$ B_(ell,k) = F_(r,s,c,k) $

$ G_(m,k) = O_(n,p,q,k) $

把这些代入矩阵乘法：

$ G_(m,k) =
  sum_(ell=0)^(R S C - 1) A_(m,ell) B_(ell,k) $

再把 $ell$ 还原成 $(r,s,c)$，就得到前面的卷积公式：

$ G_(m,k) =
  sum_(r=0)^(R - 1)
  sum_(s=0)^(S - 1)
  sum_(c=0)^(C - 1)
  I_(n, p u + r d_h - a_h, q v + s d_w - a_w, c)
  F_(r,s,c,k)
  = O_(n,p,q,k) $

所以，GEMM convolution 并不是改变了数学结果，而是改变了计算组织方式。

= im2col 与 implicit GEMM

如果显式生成 $A$，这个过程通常叫 im2col：

```text
input tensor -> im2col matrix A -> GEMM -> output tensor
```

这样做的好处是实现简单，可以直接调用成熟的矩阵乘法库。问题是 $A$ 可能非常大，因为相邻 patch 会重复包含大量相同输入元素。

implicit GEMM convolution 的做法是：

```text
input tensor -> kernel 内按 A(m, ell) 的映射取数 -> GEMM 风格计算 -> output tensor
```

也就是说，$A$ 是一个逻辑矩阵，而不是一个提前写入内存的真实大矩阵。kernel 内部按 GEMM 的 tile、warp、thread 组织方式工作，需要 $A_(m,ell)$ 时，再根据 $m$ 和 $ell$ 算出原始 input tensor 中的坐标去加载。

所以：

- GEMM convolution：把卷积写成 $C = A B$
- explicit GEMM convolution：先真的生成 $A$，再做 GEMM
- implicit GEMM convolution：不生成完整 $A$，只在 kernel 内隐式访问 $A_(m,ell)$

= 一个形状例子

假设：

$ N = 1, H = 4, W = 4, C = 1 $

$ R = 3, S = 3, K = 2 $

stride 为 1，padding 为 0，dilation 为 1。

那么：

$ P = 4 - 3 + 1 = 2 $

$ Q = 4 - 3 + 1 = 2 $

输出 shape 是：

$ O in RR^(1 times 2 times 2 times 2) $

GEMM 中三个矩阵的 shape 是：

$ A: (N P Q) times (R S C) = 4 times 9 $

$ B: (R S C) times K = 9 times 2 $

$ G: (N P Q) times K = 4 times 2 $

其中 $A$ 的 4 行分别对应 4 个输出空间位置：

```text
(n=0, p=0, q=0)
(n=0, p=0, q=1)
(n=0, p=1, q=0)
(n=0, p=1, q=1)
```

每一行有 9 个元素，对应一个 $3 times 3 times 1$ 的输入 patch。

= 一句话总结

卷积可以看成大量 patch-filter 点积。把每个输出位置对应的 patch 放到矩阵 $A$ 的一行，把每个 filter 放到矩阵 $B$ 的一列，卷积就变成了 $C = A B$。implicit GEMM convolution 保留这个 GEMM 视角，但不显式生成完整 im2col 矩阵，而是在 kernel 中按映射关系从原始 tensor 取数。
