// Maxwell 方程入门教程
// 编译命令: typst compile maxwell-tutorial.typ

#set document(title: "Maxwell 方程入门教程", author: "Physics Tutorial")
#set page(numbering: "1", margin: (x: 2cm, y: 2cm))
#set text(size: 11pt)
#set heading(numbering: "1.1.")

// 定理环境
#let theorem = (title: none, body) => {
  box(stroke: blue, inset: 8pt, radius: 4pt, fill: blue.lighten(90%))[
    #text(weight: "bold", fill: blue)[定理#title:] #body
  ]
}

#let note = (body) => {
  box(stroke: orange, inset: 8pt, radius: 4pt, fill: orange.lighten(90%))[
    #text(weight: "bold", fill: orange)[注意:] #body
  ]
}

// 标题页
#align(center + horizon)[
  #text(size: 24pt, weight: "bold")[Maxwell 方程入门教程]
  #v(1cm)
  #text(size: 14pt)[从基本原理到现代应用]
  #v(0.5cm)
  #text(size: 12pt, style: "italic")[An Intuitive Guide to Maxwell's Equations]
  #v(2cm)
  #text(size: 11pt)[2026年2月]
]

#pagebreak()

// 目录
#outline(title: "目录", depth: 2)

#pagebreak()

= 引言

麦克斯韦方程组（Maxwell's Equations）是电磁学的基石，由苏格兰物理学家詹姆斯·克拉克·麦克斯韦（James Clerk Maxwell）于1865年提出。这组方程统一了电学、磁学和光学，预言了电磁波的存在，并奠定了现代电力技术和通信技术的基础。

#note[
  麦克斯韦方程组是物理学中最优雅的理论之一，它将复杂的电磁现象归纳为四个简洁的方程。
]

== 历史背景

在19世纪中叶，电磁学领域已经积累了许多实验定律：
- 库仑定律（静电学）
- 安培定律（静磁学）
- 法拉第电磁感应定律

麦克斯韦的贡献在于将这些定律统一起来，并引入了"位移电流"的概念，从而预言了电磁波的存在。

= Maxwell 方程组概述

Maxwell 方程组包含四个基本方程，分别描述电场和磁场的行为：

#table(
  columns: (1fr, 2fr, 2fr),
  inset: 10pt,
  align: center,
  table.header([方程], [微分形式], [积分形式]),
  [高斯定律], [$div arrow(E) = rho / epsilon_0$], [$integral.cont arrow(E) dot d arrow(A) = Q / epsilon_0$],
  [高斯磁定律], [$div arrow(B) = 0$], [$integral.cont arrow(B) dot d arrow(A) = 0$],
  [法拉第定律], [$nabla times arrow(E) = -partial arrow(B) / partial t$], [$integral arrow(E) dot d arrow(l) = -partial Phi_B / partial t$],
  [安培-麦克斯韦定律], [$nabla times arrow(B) = mu_0 arrow(J) + mu_0 epsilon_0 partial arrow(E) / partial t$], [$integral arrow(B) dot d arrow(l) = mu_0 I + mu_0 epsilon_0 partial Phi_E / partial t$],
)

= 各方程详解

== 高斯定律（Gauss's Law）

#theorem(title: "高斯定律")[
  通过一个闭合曲面的电通量等于该曲面内包围的总电荷除以真空介电常数。

  $ integral.cont_S arrow(E) dot d arrow(A) = Q_(text("总")) / epsilon_0 $
]

高斯定律揭示了电场的一个基本特性：
- *正电荷是电场的源*，电场线从正电荷向外发散
- *负电荷是电场的汇*，电场线向负电荷汇聚

=== 物理意义

- 电场线起始于正电荷，终止于负电荷
- 电荷是电场的源，电场线不闭合
- 这是电场与磁场的根本区别之一

== 高斯磁定律（Gauss's Law for Magnetism）

#theorem(title: "高斯磁定律")[
  通过任意闭合曲面的磁通量恒为零。

  $ integral.cont_S arrow(B) dot d arrow(A) = 0 $
]

这个方程表明：
- *磁单极子不存在*（或尚未被发现）
- 磁场线总是闭合的，没有起点也没有终点
- 磁力线形成连续的闭合回路

=== 物理意义

高斯磁定律是电磁理论中唯一一个"不对称"的方程——它表明自然界中不存在磁单极子，这与电荷的情况形成鲜明对比。

== 法拉第电磁感应定律（Faraday's Law）

#theorem(title: "法拉第定律")[
  感应电动势等于穿过回路的磁通量变化率的负值。

  $ integral.cont_C arrow(E) dot d arrow(l) = -partial / partial t integral_S arrow(B) dot d arrow(A) $
]

这个方程揭示了：
- *变化的磁场产生电场*
- 这是发电机、变压器等设备的工作原理
- 负号表示楞次定律：感应电流的方向总是阻碍磁通量的变化

=== 应用实例

- *发电机*: 线圈在磁场中旋转产生电流
- *变压器*: 变化的电流产生变化的磁场，进而感应出电压
- *感应加热*: 交变磁场在导体中产生涡流

== 安培-麦克斯韦定律（Ampère-Maxwell Law）

#theorem(title: "安培-麦克斯韦定律")[
  磁场沿闭合路径的线积分等于穿过该路径的电流加上位移电流。

  $ integral.cont_C arrow(B) dot d arrow(l) = mu_0 I + mu_0 epsilon_0 partial / partial t integral_S arrow(E) dot d arrow(A) $
]

麦克斯韦的关键贡献是添加了*位移电流*项：

$ arrow(J)_D = epsilon_0 partial arrow(E) / partial t $

这个方程表明：
- *电流产生磁场*
- *变化的电场也产生磁场*

=== 位移电流的意义

位移电流是麦克斯韦的天才洞察。即使没有真实的电荷流动，变化的电场也能产生磁场。这一预测导致了电磁波存在的理论证明。

= 方程组的统一之美

== 对称性分析

Maxwell 方程组展现了深刻的对称性：

#align(center)[
  #table(
    columns: (1fr, 1fr),
    inset: 10pt,
    table.header([电场相关], [磁场相关]),
    [电荷产生电场], [电流产生磁场],
    [变化的磁场产生电场], [变化的电场产生磁场],
    [电场线从电荷发散], [磁场线闭合],
  )
]

== 电磁波预言

在真空中（$rho = 0, arrow(J) = 0$），Maxwell 方程组可以导出波动方程：

$ nabla^2 arrow(E) = mu_0 epsilon_0 partial^2 arrow(E) / partial t^2 $

$ nabla^2 arrow(B) = mu_0 epsilon_0 partial^2 arrow(B) / partial t^2 $

电磁波速度为：

$ c = 1 / sqrt(mu_0 epsilon_0) approx 3 times 10^8 "m/s" $

这个速度与光速完全一致，麦克斯韦由此推断：*光是一种电磁波*。

= 求解方法

== 矩量法（Method of Moments, MoM）

矩量法是由 Roger F. Harrington 于1960年代提出的求解 Maxwell 方程的重要数值方法。

=== 基本思想

1. 将待求函数展开为基函数的线性组合
2. 利用加权余量法建立方程组
3. 求解线性方程组得到展开系数

=== 数学表述

对于算子方程 $L(f) = g$，将 $f$ 展开为：

$ f = sum_(n=1)^N alpha_n f_n $

其中 $f_n$ 是基函数，$alpha_n$ 是待定系数。

== 其他数值方法

#table(
  columns: (1.5fr, 2fr, 2fr),
  inset: 8pt,
  table.header([方法], [特点], [适用场景]),
  [有限差分时域法 (FDTD)], [直接在时域求解, 直观], [宽带问题, 瞬态分析],
  [有限元法 (FEM)], [处理复杂几何, 灵活], [非均匀介质, 复杂结构],
  [矩量法 (MoM)], [基于积分方程, 精度高], [天线, 散射问题],
  [边界元法 (BEM)], [降维, 减少未知数], [开放区域问题],
)

= 现代应用

== 无线通信

从无线电到5G/6G，所有无线通信都基于 Maxwell 方程描述的电磁波传播：
- 天线设计
- 信号传播建模
- 频谱管理

== 光学技术

- 光纤通信
- 激光技术
- 成像系统

== 电力工程

- 电机设计
- 变压器优化
- 电网传输

= 总结

Maxwell 方程组是物理学历史上最伟大的成就之一。它不仅统一了电磁学理论，还预言了电磁波的存在，开启了现代通信技术的时代。

#note[
  理解 Maxwell 方程的关键在于把握电场与磁场的相互作用：变化的电场产生磁场，变化的磁场产生电场，这种相互耦合形成了在空间中传播的电磁波。
]

= 参考资源

+ Feynman, R. P. "The Feynman Lectures on Physics", Vol. 2
+ Griffiths, D. J. "Introduction to Electrodynamics"
+ Jackson, J. D. "Classical Electrodynamics"
+ Harrington, R. F. "Field Computation by Moment Methods"

== 在线资源

- "An Intuitive Guide to Maxwell's Equations": 可视化理解 Maxwell 方程
- Roger F. Harrington 的矩量法原著
- Gibson, W. C. "The Method of Moments in Electromagnetics"

---

#v(1cm)
#align(center)[
  #text(style: "italic")[
    "从这几行方程中，我们认识了光的本性，理解了电磁力的统一，也看到了现代技术的无限可能。"
  ]
]
