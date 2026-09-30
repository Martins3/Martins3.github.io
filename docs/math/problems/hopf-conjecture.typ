#set document(
  title: "Hopf 猜想",
  author: "整理日期：2026-09-04",
)

#set page(
  paper: "a4",
  margin: (x: 2.3cm, y: 2.1cm),
  numbering: "1",
)
#set text(font: "Noto Sans CJK SC", size: 10pt, lang: "zh")
#set par(justify: true, leading: 0.7em)
#set heading(numbering: "1.")
#set math.equation(numbering: "(1)")
#show link: set text(fill: rgb("245c8a"))

#let blue = rgb("315f85")
#let green = rgb("2f7757")
#let amber = rgb("a66a18")
#let red = rgb("a84646")
#let gray = rgb("66707c")
#let Pf = math.op("Pf")

#let callout(color, fill-color, title, body) = block(
  width: 100%,
  fill: fill-color,
  stroke: (left: 3pt + color),
  inset: (x: 11pt, y: 8pt),
  radius: (right: 4pt),
  [#text(weight: "bold", fill: color)[#title]\
  #body],
)
#let definition(title, body) = callout(blue, rgb("eef5fb"), title, body)
#let result(title, body) = callout(green, rgb("eef8f1"), title, body)
#let warning(title, body) = callout(amber, rgb("fff7e8"), title, body)
#let open(title, body) = callout(red, rgb("fff0f0"), title, body)
#let src(label, url) = text(size: 8.3pt, fill: blue)[#link(url)[#label]]

#align(center)[
  #text(size: 23pt, weight: "bold")[Hopf 猜想]
  #v(6pt)
  #text(size: 12.5pt, fill: gray)[正曲率能否决定空间的整体拓扑？]
]

#v(1.2em)

微分几何里有两类问题都常被称为“Hopf 猜想”。第一类问：偶数维闭流形处处具有正截面曲率时，它的 Euler 示性数是否一定为正？第二类问：$S^2 times S^2$ 能否承载处处正的截面曲率？

它们共享同一个主题：#strong[局部曲率条件究竟能多强地限制整体拓扑]。但二者不是同一个命题，后文会始终把它们称为“示性数版本”和“乘积版本”。

#outline(title: [目录], depth: 2)

#pagebreak()

= 三个前置概念

== 闭 Riemann 流形

流形可以粗略理解为“每个很小的局部都像欧氏空间”的空间。给流形装上 Riemann 度量 $g$ 后，就能谈长度、角度、面积、测地线和曲率。

“闭流形”指紧致且没有边界的流形。球面 $S^2$、高维球面 $S^n$ 和环面 $T^2$ 都是闭流形；欧氏空间 $RR^n$ 不是紧致的，因此不是闭流形。

== 截面曲率

在曲面上，每一点只有一个二维切平面，高斯曲率 $K$ 给出该点的弯曲程度。在 $n>=3$ 维时，一个点的切空间中有许多二维平面，所以要为每个二维方向 $sigma subset T_p M$ 分别定义截面曲率

$ sec_p(sigma). $

#definition[正截面曲率][
  若对每个点 $p in M$ 和每个二维切平面 $sigma subset T_p M$ 都有

  $ sec_p(sigma) > 0, $

  就称 $(M,g)$ 具有正截面曲率。把 $>$ 换成 $>=$，得到非负截面曲率。
]

这是一个很强的逐点、逐方向条件。正 Ricci 曲率或正标量曲率只控制若干截面曲率的和，明显弱于所有截面曲率都为正。

== Euler 示性数

Euler 示性数 $chi(M)$ 是拓扑不变量，不随度量的改变而改变。用实系数 Betti 数表示，它是

$ chi(M) = sum_(i=0)^n (-1)^i b_i(M), $

其中 $b_i$ 粗略记录 $i$ 维“洞”的数量。

#table(
  columns: (1.5fr, 1fr, 2.2fr),
  align: (left, center, left),
  inset: 6pt,
  stroke: rgb("d9dee5"),
  table.header([*空间*], [$chi$], [*直观或计算*]),
  [$S^(2m)$], [$2$], [偶维球面只有 $b_0=b_(2m)=1$。],
  [$S^(2m+1)$], [$0$], [奇维球面的两项在交错和中抵消。],
  [$T^2$], [$0$], [$1-2+1=0$。],
  [$CC P^m$], [$m+1$], [偶数维同调群各贡献 1。],
  [$S^2 times S^2$], [$4$], [$chi(S^2)chi(S^2)=2 dot 2$。],
)

截面曲率依赖所选度量，是几何量；Euler 示性数不依赖度量，是拓扑量。Hopf 猜想要建立的正是二者之间的桥梁。

= 示性数版本

#open[Hopf 示性数猜想][
  若 $M^(2m)$ 是具有正截面曲率的闭 Riemann 流形，则

  $ chi(M) > 0. $
]

还有一个自然的非负曲率版本：若 $sec >= 0$，是否必有 $chi(M)>=0$？正曲率命题和非负曲率命题要分开看，因为从严格不等式退化到非严格不等式后，会出现平坦方向和乘积分解等新现象。

== 二维为什么成立

对闭的定向曲面，Gauss--Bonnet 定理给出

$ integral_M K dif A = 2 pi chi(M). $

二维的截面曲率就是高斯曲率。若处处 $K>0$，左边严格为正，因此

$ chi(M) > 0. $

这说明猜想在二维几乎是 Gauss--Bonnet 定理的直接推论。事实上，闭的定向曲面满足 $chi=2-2g$，其中 $g$ 是亏格；正曲率迫使 $g=0$，所以它在拓扑上就是球面。

== 四维为什么也成立

四维可以利用 Synge 定理和 Poincaré 对偶来处理。若 $M^4$ 可定向并有正截面曲率，Synge 定理推出它单连通，于是

$ b_1(M)=b_3(M)=0. $

再由 $b_0=b_4=1$，得到

$ chi(M)=b_0-b_1+b_2-b_3+b_4=2+b_2>0. $

不可定向情形可以转到定向双覆盖；Euler 示性数在有限覆盖下按覆盖次数相乘。因而四维也满足猜想。

#result[低维结论][
  Hopf 示性数猜想在维数 2 和 4 成立。真正的新困难从更高偶数维开始。
]

= Gauss--Bonnet 为何没有直接解决高维情形

Chern--Gauss--Bonnet 定理在 $2m$ 维仍把 Euler 示性数写成曲率积分：

$ chi(M) = 1 / (2 pi)^m integral_M Pf(Omega). $

这里 $Omega$ 是曲率形式矩阵，$Pf$ 是 Pfaffian。二维时，被积函数本质上就是高斯曲率，所以 $K>0$ 会直接给出正积分。

高维情况不同。$Pf(Omega)$ 是许多曲率张量分量乘积的带符号组合；“每个二维截面的曲率都为正”并不会显然推出

$ Pf(Omega)(p) > 0 $

在每一点成立。也就是说，积分公式确实存在，但它的被积函数没有从假设中得到明显的逐点符号。

#warning[核心障碍][
  Hopf 猜想不是缺少一个高维 Gauss--Bonnet 公式；公式早已存在。困难在于，截面曲率的正性无法直接转化为 Euler 形式的逐点正性。
]

这也解释了为什么把二维证明机械推广到高维行不通：二维只有一个截面方向，高维却需要把大量彼此耦合的曲率分量组合起来。

= 乘积版本：$S^2 times S^2$

#open[Hopf 乘积猜想的基本特例][
  $S^2 times S^2$ 不存在处处具有正截面曲率的 Riemann 度量。
]

更一般的表述会猜测，两个正维闭流形的乘积不能承载正截面曲率；其中 $S^2 times S^2$ 是最经典、最基本的测试对象。

== 它明明有非负曲率

给两个 $S^2$ 都装上标准圆球度量，再取乘积度量。完全位于某个球面因子内的二维平面具有正曲率；但由两个不同因子的方向张成的“混合平面”满足

$ sec(sigma_("mixed")) = 0. $

所以标准乘积度量满足 $sec>=0$，却不满足 $sec>0$。问题是：能否不拘泥于乘积形式，换一个更巧妙的度量，把所有混合方向的零曲率同时抬高？至今没有人知道答案。

== 为什么不能靠小扰动轻易解决

直觉上，可以尝试轻微改变乘积度量，让原来的零曲率变成正数。但零曲率平面形成一个很大的连续族；一个扰动可能抬高某些方向，同时压低另一些方向。要得到正截面曲率，必须在每一点、每个二维方向上同时保持严格正值。

这是一组无限多、彼此耦合的不等式。对某个固定平面计算曲率变化并不困难，困难的是构造一个度量变化，使所有原本为零的平面都朝正确方向变化，而且不破坏原有的正曲率方向。

== 它与示性数版本是什么关系

$S^2 times S^2$ 的 Euler 示性数为

$ chi(S^2 times S^2)=4>0. $

因此它完全符合示性数猜想所要求的必要符号。即使示性数版本得到证明，也不能排除 $S^2 times S^2$ 的正曲率度量；乘积版本需要比 Euler 示性数更精细的拓扑或几何障碍。

#warning[不要混淆两个命题][
  示性数版本说“正曲率 $=>$ $chi>0$”；乘积版本猜测“$S^2 times S^2$ 没有正曲率度量”。由于 $S^2 times S^2$ 本来就有 $chi=4$，前者不能推出后者。
]

= 已知进展说明了什么

一般的示性数猜想仍然开放，但加入足够强的对称性后已经得到许多肯定结果。典型做法是让环面 $T^k$ 等紧 Lie 群等距作用在 $M$ 上，再利用固定点集、闭测地线、同调周期性和群作用结构，把曲率条件转化成拓扑限制。

例如，已有结果在低余维群作用或高对称秩等附加条件下证明 $chi(M)>0$；后续工作又把某些结论所需的环面对称维数继续降低。#src("PÜTTMANN--SEARLE", "https://arxiv.org/abs/1207.4086") #src("NIENHAUS", "https://arxiv.org/abs/2211.13151")

这些定理表明猜想与正曲率流形的对称结构高度相容，但不能覆盖一般情形：一个正曲率度量完全可能只有很小甚至有限的等距群。

对 $S^2 times S^2$ 而言，Hsiang--Kleiner 的四维结果给出了更直接的限制：具有正截面曲率且承认非平凡连续等距对称的闭定向四维流形，在拓扑上只能是 $S^4$ 或 $CC P^2$。因此，若 $S^2 times S^2$ 真有正曲率度量，它不能带有这种连续对称；只在熟悉的高度对称候选中搜索不足以解决问题。#src("HSIANG--KLEINER", "https://doi.org/10.4310/jdg/1214443064")

#result[目前可以确认的边界][
  我们知道许多正曲率空间的 Euler 示性数为正，也知道猜想在低维或较强对称性条件下成立；但一般的高维示性数版本仍未解决，$S^2 times S^2$ 是否能承载正截面曲率也仍未解决。#src("KENNARD--MOUILLÉ--NIENHAUS", "https://arxiv.org/abs/2507.16936")
]

= 常见误解

#block(breakable: false)[
  #table(
    columns: (1.55fr, 3.15fr),
    align: (left, left),
    inset: 7pt,
    stroke: rgb("d9dee5"),
    table.header([*说法*], [*问题在哪里*]),
    [“正 Ricci 曲率就足够了。”],
    [Ricci 曲率只对截面曲率求和，比正截面曲率弱得多，不能直接代替猜想的假设。],
    [“Chern--Gauss--Bonnet 已经证明了猜想。”],
    [它给出积分公式，却没有保证高维 Euler 形式在正截面曲率下逐点为正。],
    [“$S^2 times S^2$ 的乘积度量就是正曲率。”],
    [混合二维平面的截面曲率为零，所以它只有非负曲率。],
    [“示性数猜想成立就能解决乘积版本。”],
    [$S^2 times S^2$ 已经满足 $chi=4>0$，示性数的符号不足以排除正曲率。],
  )
]

= 一句话理解

#open[总结][
  Hopf 示性数猜想问局部的正截面曲率是否迫使偶维闭流形具有正 Euler 示性数；$S^2 times S^2$ 问题则展示了非负曲率与正曲率之间极难跨越的边界。二者都在追问：曲率究竟能看见多少拓扑信息？
]

= 参考资料

- #link("https://arxiv.org/abs/1207.4086")[Püttmann--Searle：低余维或高对称秩下的 Hopf 猜想。]
- #link("https://doi.org/10.4310/jdg/1214443064")[Hsiang--Kleiner：带连续对称的正曲率四维流形。]
- #link("https://arxiv.org/abs/2211.13151")[Nienhaus：四周期性定理与带对称的 Hopf 猜想。]
- #link("https://arxiv.org/abs/2507.16936")[Kennard--Mouillé--Nienhaus：Hopf 猜想与中间 Ricci 曲率。]
