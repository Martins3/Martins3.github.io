#import "@preview/ctheorems:1.1.3": *
#show: thmrules

#set heading(numbering: "1.")
#set text(font: "Noto Serif CJK SC", size: 11pt)
#set math.equation(numbering: "(1)")

// 定理环境配置
#let definition = thmbox("definition", "定义", fill: rgb("#e8f0fe"))
#let theorem = thmbox("theorem", "定理", fill: rgb("#fef3e8"))
#let proof = thmproof("proof", "证明")
#let example = thmbox("example", "例", fill: rgb("#e8f8e8"))

#align(center)[#text(size: 20pt, weight: "bold")[有趣的数学分析主题]]

本笔记将探讨几个相互关联的深刻数学主题：从经典的级数理论，到超越数的概念，再到五次方程的不可解性，以及复变函数在实积分计算中的应用。

= 调和级数

#definition[
  *调和级数*是指如下无穷级数：
  $ H_n = sum_(n=1)^oo 1/n = 1 + 1/2 + 1/3 + 1/4 + dots $
]

调和级数是人类最早研究的级数之一。尽管其通项 $1/n$ 趋于 0，但这个级数却是发散的。

#theorem[
  调和级数发散：$display(sum_(n=1)^oo 1/n = +oo)$
]

#proof[
  这里给出经典的*柯西凝聚判别法*证明，以及更直观的*分组证明*。

  *方法一：分组证明*

  我们将级数按如下方式分组：
  $ sum_(n=1)^oo 1/n &= 1 + 1/2 + (1/3 + 1/4) + (1/5 + 1/6 + 1/7 + 1/8) + dots.c $

  注意到：
  $ 1/3 + 1/4 &> 1/4 + 1/4 = 1/2 $
  $ 1/5 + 1/6 + 1/7 + 1/8 &> 4 times 1/8 = 1/2 $

  一般地，第 $k$ 组（包含 $2^k$ 项）满足：
  $ sum_(n=2^k+1)^(2^(k+1)) 1/n > 2^k times 1/(2^(k+1)) = 1/2 $

  因此：
  $ sum_(n=1)^oo 1/n > 1 + 1/2 + 1/2 + 1/2 + dots.c = +oo $

  *方法二：积分判别法*

  考虑函数 $f(x) = 1/x$，它是正的、连续且递减的。由于：
  $ integral_1^oo 1/x dif x = lim_(b->oo) ln b = +oo $

  由积分判别法，调和级数发散。
]

#example[
  调和级数的部分和与对数函数有密切关系：
  $ lim_(n->oo) (sum_(k=1)^n 1/k - ln n) = gamma $
  其中 $gamma approx 0.5772$ 称为*欧拉-马歇罗尼常数*。这是一个著名的未解决问题：$gamma$ 是否是有理数？目前尚不知道。
]

= 自然常数 $e$ 及其导数

== $e$ 的多种定义

自然常数 $e$ 是数学中最重要的常数之一，可以通过多种等价的方式定义：

#definition[
  自然常数 $e$ 的等价定义：

  1. *极限定义*：$display(e = lim_(n->oo) (1 + 1/n)^n)$

  2. *级数定义*：$display(e = sum_(n=0)^oo 1/(n!))$

  3. *积分定义*：$display(e^x)$ 是满足 $f'(x) = f(x)$ 且 $f(0) = 1$ 的唯一函数在 $x=1$ 处的值。

  4. *对数定义*：$e$ 是满足 $display(integral_1^e 1/t dif t = 1)$ 的唯一正实数。
]

这些定义的等价性可以通过分析学的方法加以证明。其中级数定义特别有用，因为它直接展示了 $e$ 是无理数（事实上是超越数）。

== $e^x$ 导数的证明

#theorem[
  设 $f(x) = e^x$，则 $f'(x) = e^x$，即 $(e^x)' = e^x$。
]

#proof[
  我们使用 $e$ 的级数定义来证明这一性质。

  首先，$e^x$ 可以通过幂级数定义：
  $ e^x = sum_(n=0)^oo x^n/(n!) = 1 + x + x^2/(2!) + x^3/(3!) + dots.c $

  该幂级数的收敛半径为 $+oo$（由比值判别法），因此可以逐项求导。

  对两边求导：
  $ dif/(dif x) e^x &= dif/(dif x) sum_(n=0)^oo x^n/(n!) $
  $ &= sum_(n=0)^oo dif/(dif x) (x^n/(n!)) $
  $ &= sum_(n=1)^oo (n x^(n-1))/(n!) $
  $ &= sum_(n=1)^oo x^(n-1)/((n-1)!) $
  $ &= sum_(m=0)^oo x^m/(m!) quad (m = n-1) $
  $ &= e^x $

  证毕。
]

#example[
  *利用极限定义的替代证明*：

  从极限定义出发：
  $ f'(x) = lim_(h->0) (e^(x+h) - e^x)/h = e^x dot lim_(h->0) (e^h - 1)/h $

  关键步骤是证明 $display(lim_(h->0) (e^h - 1)/h = 1)$。

  利用 $e^h = display(lim_(n->oo) (1 + h/n)^n)$，对小的 $h$ 有：
  $ (e^h - 1)/h approx ((1 + h/n)^n - 1)/h $

  当 $n$ 足够大时，$(1 + h/n)^n approx 1 + h + O(h^2/n)$，因此：
  $ lim_(h->0) (e^h - 1)/h = 1 $
]

= 超越数

== 代数数与超越数

#definition[
  一个复数 $alpha$ 称为*代数数*，如果存在非零整系数多项式 $P(x) in ZZ[x]$，使得 $P(alpha) = 0$。

  不是代数数的复数称为*超越数*。
]

#example[
  - 所有有理数 $p/q$ 都是代数数（是 $q x - p = 0$ 的根）
  - $sqrt(2)$ 是代数数（是 $x^2 - 2 = 0$ 的根）
  - $root(3, 5)$ 是代数数
  - $i = sqrt(-1)$ 是代数数（是 $x^2 + 1 = 0$ 的根）
]

== 著名超越数

#theorem[
  1. *(林德曼-魏尔斯特拉斯定理, 1885)*：若 $alpha_1, alpha_2, dots, alpha_n$ 是互不相同的代数数，则 $e^(alpha_1), e^(alpha_2), dots, e^(alpha_n)$ 在代数数域上线性无关。

  2. 特别地，$e$ 和 $pi$ 都是超越数。
]

#example[
  $pi$ 的超越性证明了古希腊"化圆为方"问题用尺规作图是不可能的。因为如果可作，则 $sqrt(pi)$ 将是代数数，从而 $pi$ 也是代数数。
]

= 五次方程的不可解性

这是代数学史上最深刻的结果之一，由伽罗瓦（Évariste Galois）在19世纪初创立的理论所揭示。

== 根式可解的定义

#definition[
  一个多项式方程 $P(x) = 0$ 称为*根式可解*，如果它的根可以通过有限次加、减、乘、除和开 $n$ 次方运算（从方程的系数出发）表示出来。
]

#example[
  - 一次方程：$a x + b = 0$，解为 $x = -b/a$
  - 二次方程：$a x^2 + b x + c = 0$，解为 $x = (-b plus.minus sqrt(b^2 - 4 a c))/(2 a)$
  - 三次和四次方程也有类似的求根公式（但更为复杂）
]

== 伽罗瓦理论的核心思想

伽罗瓦的关键洞察是：将方程的根的对称性与*伽罗瓦群*的结构联系起来。

#definition[
  设 $P(x) in QQ[x]$ 是不可约多项式，$K$ 是其分裂域。$P$ 的*伽罗瓦群* $"Gal"(K/QQ)$ 是保持 $QQ$ 不动的 $K$ 的自同构群。
]

#theorem[
  (伽罗瓦判别法) 多项式 $P(x)$ 根式可解当且仅当其伽罗瓦群是可解群。
]

== $S_5$ 的不可解性

#theorem[
  一般五次方程 $x^5 + a_1 x^4 + a_2 x^3 + a_3 x^2 + a_4 x + a_5 = 0$ 不是根式可解的。
]

#proof[
  证明概要：

  1. 一般五次方程的伽罗瓦群是对称群 $S_5$。

  2. 对于 $n >= 5$，交错群 $A_n$ 是单群（没有非平凡的正规子群）。

  3. $S_5$ 的合成列为：$\{e\} triangle.stroked.l A_5 triangle.stroked.l S_5$

  4. 由于 $A_5$ 是单群且非阿贝尔群，它不是循环群。

  5. 因此 $S_5$ 不是可解群（可解群要求所有合成因子都是素数阶循环群）。

  6. 由伽罗瓦判别法，一般五次方程不是根式可解的。

  具体而言，$A_5$ 的阶为 $5!/2 = 60 = 2^2 dot 3 dot 5$，它是一个非阿贝尔单群。这意味着不存在正规子群列使得每个商群都是阿贝尔群。
]

#example[
  方程 $x^5 - x + 1 = 0$ 就是一个具体的五次方程，其伽罗瓦群是 $S_5$，因此不能用根式求解。
]

= 复变函数方法计算实积分

复变函数论为计算某些困难的实积分提供了强大的工具，特别是*留数定理*的应用。

== 问题陈述

计算积分：
$ I = integral_(-oo)^(+oo) 1/(1 + x^2) dif x $

== 留数定理回顾

#theorem[
  (留数定理) 设 $f(z)$ 在简单闭曲线 $C$ 及其内部除有限个孤立奇点 $z_1, z_2, dots, z_n$ 外解析，则：
  $ integral_C f(z) dif z = 2pi i sum_(k=1)^n "Res"(f, z_k) $
]

== 积分计算

#theorem[
  $display(integral_(-oo)^(+oo) 1/(1 + x^2) dif x = pi)$
]

#proof[
  考虑复变函数 $f(z) = 1/(1 + z^2)$。该函数在复平面上有两个一阶极点：
  $ z^2 + 1 = 0 arrow.long.double z = plus.minus i $

  构造积分围道：以原点为中心，半径为 $R > 1$ 的上半圆。围道由两部分组成：
  - $C_R$：上半圆弧 $z = R e^(i theta)$，$theta in [0, pi]$
  - $[-R, R]$：实轴上的线段

  由留数定理：
  $ integral_([-R,R] union C_R) f(z) dif z = 2pi i dot "Res"(f, i) $

  计算在 $z = i$ 处的留数：
  $ "Res"(f, i) = lim_(z->i) (z - i) dot 1/((z-i)(z+i)) = 1/(2i) $

  因此：
  $ integral_([-R,R] union C_R) f(z) dif z = 2pi i dot 1/(2i) = pi $

  现在证明当 $R -> oo$ 时，沿 $C_R$ 的积分趋于 0：

  在 $C_R$ 上，$|z| = R$，所以：
  $ |1 + z^2| >= |z|^2 - 1 = R^2 - 1 $

  因此：
  $ |integral_(C_R) f(z) dif z| &<= "length"(C_R) dot max_(z in C_R) |f(z)| $
  $ &= pi R dot 1/(R^2 - 1) -> 0 quad (R -> oo) $

  取极限 $R -> oo$：
  $ integral_(-oo)^(+oo) 1/(1 + x^2) dif x = pi $
]

#example[
  当然，这个积分也可以用初等方法计算：
  $ integral 1/(1+x^2) dif x = arctan x + C $
  因此：
  $ integral_(-oo)^(+oo) 1/(1+x^2) dif x = lim_(a->oo) [arctan a - arctan(-a)] = pi/2 - (-pi/2) = pi $

  复变方法的优势在于可以处理更复杂的积分，如：
  $ integral_(-oo)^(+oo) (cos x)/(1+x^2) dif x = pi/e $
  这类积分用实分析方法很难计算。
]

= 总结

本笔记探讨了数学分析中的几个深刻主题：

1. *调和级数*的发散性展示了无穷级数的微妙性质。

2. *自然常数 $e$* 的多种定义方式体现了数学概念的丰富性，其自守性质（导数等于自身）使其成为分析学的核心。

3. *超越数*的概念区分了代数结构和超越结构，林德曼-魏尔斯特拉斯定理是这一领域的巅峰成就。

4. *五次方程不可解*是代数学的里程碑，伽罗瓦群论的思想彻底改变了现代数学的面貌。

5. *复变函数*方法展示了不同数学分支之间的深刻联系，留数定理将复分析的技巧应用于实积分的计算。

这些主题看似分散，实则统一于数学对"结构"和"对称性"的深刻理解之中。

= 参考文献

+ Ahlfors, L. V. (1979). *Complex Analysis*. McGraw-Hill.
+ Edwards, H. M. (1984). *Galois Theory*. Springer.
+ Rudin, W. (1976). *Principles of Mathematical Analysis*. McGraw-Hill.
+ 华罗庚 (1979). 《数论导引》. 科学出版社.
