#set document(
  title: "Erdős 单位距离猜想",
  author: "整理日期：2026-08-28",
)

#set page(
  paper: "a4",
  margin: (x: 2.25cm, y: 2.1cm),
  numbering: "1",
  header: context {
    if counter(page).get().first() > 1 {
      align(right, text(size: 8.5pt, fill: rgb("6d7480"))[2026 AI 数学突破实录])
    }
  },
)
#set text(font: "Noto Sans CJK SC", size: 10.5pt, lang: "zh")
#set par(justify: true, leading: 0.72em)
#set heading(numbering: "1.")
#set math.equation(numbering: "(1)")
#show link: set text(fill: rgb("245c8a"))
#show raw.where(block: true): block.with(
  fill: rgb("f5f7f9"),
  inset: 10pt,
  radius: 4pt,
)

#let blue = rgb("315f85")
#let green = rgb("2f7757")
#let amber = rgb("a66a18")
#let red = rgb("a84646")
#let gray = rgb("66707c")

#let callout(color, fill-color, title, body) = block(
  width: 100%,
  fill: fill-color,
  stroke: (left: 3pt + color),
  inset: (x: 11pt, y: 8pt),
  radius: (right: 4pt),
  [#text(weight: "bold", fill: color)[#title]\
  #body],
)
#let note(title, body) = callout(blue, rgb("eef5fb"), title, body)
#let verified(title, body) = callout(green, rgb("eef8f1"), title, body)
#let warning(title, body) = callout(amber, rgb("fff7e8"), title, body)
#let danger(title, body) = callout(red, rgb("fff0f0"), title, body)
#let src(label, url) = text(size: 8.3pt, fill: blue)[#link(url)[#label]]
#let status(color, body) = box(
  fill: color.lighten(84%),
  stroke: color.lighten(45%),
  inset: (x: 5pt, y: 2pt),
  radius: 3pt,
  text(size: 8pt, weight: "bold", fill: color.darken(15%), body),
)

#align(center)[
  #text(size: 23pt, weight: "bold")[2026 AI 数学突破实录]
  #v(6pt)
  #text(size: 13pt, fill: gray)[OpenAI / ChatGPT 与 Claude：从猜想反例到机器可检验证明]
  #v(10pt)
  #text(size: 9.5pt, fill: gray)[资料截点：2026 年 8 月 28 日]
]

#v(1.2em)

#outline(title: [目录], depth: 3)

#pagebreak()

= 问题的精确定义

在欧氏平面中取一个有限点集 $P$。把距离恰好为 1 的两个点连边，并记这样的*无序点对*总数为 $nu(P)$。固定点数后，再对所有可能的摆法取最大值：

$ nu(n) := max_(P subset RR^2, |P|=n) nu(P). $

所谓“单位”没有特殊性：如果关心距离 $r>0$，把整幅图按比例 $1/r$ 缩放，就又变成距离 1。因此问题真正问的是：*平面上 $n$ 个不同点，最多能让多少对点具有同一个指定距离？*

也可以把它看成图论问题。点是顶点，单位距离点对是边，所得图称为单位距离图；$nu(n)$ 就是 $n$ 个顶点的平面单位距离图所能拥有的最大边数。这里不要求连边不能交叉，也不要求所有没连边的点对距离不同。

#note[三个容易混淆的问题][
  这不是“整数格点之间有多少条长度为 1 的边”，因为点可以摆在平面任意位置；也不是 Hadwiger--Nelson 平面染色问题；更不是 Erdős 的“不同距离问题”。它只数某一个固定长度出现了多少次。
]

= 从可手算的范例开始

小点集已经能说明，“多造单位距离”不是简单地把点均匀排开。

#table(
  columns: (1.25fr, 1.1fr, 2.9fr),
  align: (left, center, left),
  inset: 6pt,
  stroke: rgb("d9dee5"),
  table.header([*点的摆法*], [*单位距离数*], [*为什么*]),
  [直线上等间距的 $n$ 个点], [$n-1$], [只有相邻点对相距 1。数量只是线性的。],
  [边长为 1 的正三角形], [$3$], [三对点全是单位距离，即 $nu(3)=3$。],
  [共用一条边的两个正三角形], [$5$], [取 $(0,0),(1,0),(1/2, plus.minus sqrt(3)/2)$。除上下两个顶点外，其余五对距离都为 1。],
  [边长为 1 的正六边形加中心], [$12$], [六条边和六条“半径”都是单位距离。],
  [$m times m$ 方格，格距为 1], [$2m(m-1)$], [横边和竖边各有 $m(m-1)$ 条；若 $n=m^2$，仍只有 $2n-2sqrt(n)$ 条。],
)

最后一行看似说明方格没有帮助，其实它只使用了方向 $(plus.minus 1,0)$ 与 $(0,plus.minus 1)$。经典构造的关键，是缩放方格，让*许多不同的整数方向同时成为单位向量*。

例如整数方格中的向量

$ (plus.minus 1, plus.minus 2), quad (plus.minus 2, plus.minus 1) $

共有 8 个带符号、带次序的方向，它们的长度都是 $sqrt(5)$。把一个足够大的方格整体缩小 $sqrt(5)$ 倍后，每个远离边界的点就有至多 8 个单位距离邻点。边界会损失一些边，但方格足够大时，主体仍保留下来。

更一般地，若整数 $k$ 有许多表示

$ k = a^2 + b^2, $

那么每个解 $(a,b)$ 都给出一个长度 $sqrt(k)$ 的格点方向。特别地，当 $k$ 是许多互不相同、模 4 余 1 的素数之积时，表示数会随素因子数成指数增长。选好 $k$、取一个大方格并按 $1/sqrt(k)$ 缩放，便能得到

$ nu(n) >= n^(1 + c / log log n) $

这一尺度的经典下界，其中 $c>0$ 是常数。这里额外的指数 $c/log log n$ 会趋于 0，所以它虽超线性，却仍是 $n^(1+o(1))$。

#verified[平方格点真正提供了什么][
  不是“最近邻很多”，而是整数的平方和分解能制造很多*同样长度、不同方向*的向量。几何计数由此被翻译成算术中的表示数问题。这正是新反例后来推广到数域的出发点。
]

= Erdős 猜想到底猜了什么

1946 年，Erdős 猜想存在绝对常数 $C>0$ 与 $N$，使每个 $n>=N$ 都满足

$ nu(n) <= n^(1 + C / log log n). $

把量词完整写出很重要：

$ exists C>0, exists N, forall n>=N: nu(n) <= n^(1+C/log log n). $

它不是猜 $nu(n)=O(n)$，也没有断言某个具体的 $C$。它允许单位距离数比线性大很多，只要求超线性的那部分至多是 $n^(O(1/log log n))$。由于 $C/log log n -> 0$，也可以把猜想概括成

$ nu(n) <= n^(1+o(1)) $

的一种定量版本。经典平方格点下界恰好也在 $n^(1+Theta(1/log log n))$ 这个尺度上，因此几十年来的直觉是：平方和构造已经抓住正确的增长类型，剩下只是上下界常数与技术的差别。

从另一端看，1984 年以来最好的通用上界仍是

$ nu(n) = O(n^(4/3)). $

一个直观理由是：以每个点为圆心画单位圆，单位距离就对应“另一个点落在这条圆上”的点圆关联；平面关联几何限制了这种相交能有多密。于是旧局面是

$ n^(1+Omega(1/log log n)) <= nu(n) <= O(n^(4/3)). $

Erdős 猜想选择相信左端的指数形态，而不是右端的固定幂指数。

= 2026 年的反例如何否定它

新定理证明：存在一个与 $n$ 无关的绝对常数 $delta>0$，对无穷多个 $n$ 有

$ nu(n) >= n^(1+delta). $

“固定的 $delta$”是整个突破的核心。无论猜想允许多大的固定常数 $C$，只要 $n$ 足够大，就有

$ C / log log n < delta, $

从而 $n^(1+delta) > n^(1+C/log log n)$。定理又保证这样的反例规模 $n$ 无穷多，因此不可能用增大阈值 $N$ 把它们全部排除。这正好击穿了原猜想中的“存在 $C,N$，此后对所有 $n$ 成立”。#src("OAI-UD-PAPER", "https://cdn.openai.com/pdf/74c24085-19b0-4534-9c90-465b8e29ad73/unit-distance-proof.pdf")

#warning[这里没有给出一张小反例图][
  猜想是渐近命题，有限个漂亮点集不能推翻它。新证明给出的是一族规模趋于无穷的构造，并保证其单位距离数有固定的多项式增益。定理只声称存在某个 $delta>0$，并未把一个漂亮或接近最优的数值当作卖点。
]

= 从二维平方和到高次代数数域

新证明保留了经典构造的骨架：先寻找大量“长度相同的方向”，再让许多格点可以沿这些方向平移。真正的新意，是把高斯整数 $ZZ[i]$ 中的平方和算术升级到次数不断增长的数域。

可以把证明分成四层理解：

1. *制造许多候选方向。* 取一个高次数全实数域 $L$，再加入 $i$ 得到 $K=L(i)$。证明构造出指数多个元素 $u in K$，使 $u c(u)=1$；这里 $c$ 是类似复共轭的自同构。
2. *让每个坐标都看见长度 1。* 把 $K$ 通过所有复嵌入放进 $CC^f$。因为 $u c(u)=1$，每个嵌入 $sigma$ 都满足 $|sigma(u)|=1$。所以同一个 $u$ 在每个复坐标中都是单位方向。
3. *把方向变成大量边。* 在这个高维嵌入格中截取一个大“多圆盘”窗口。对窗口的平移位置做平均，可以找到一个截面，使许多格点 $x$ 与 $x+u$ 同时留在窗口内；每一对都对应一个候选单位边。
4. *回到真正的平面。* 只投影到第一个复坐标。该投影在所取格点陪集上是单射，不会把不同点压成同一点；同时每个 $u$ 的第一坐标绝对值恰为 1，所以高维中的候选边投影后确实是平面单位距离。

最深的输入藏在第一层：需要次数趋于无穷的全实数域无分歧塔，还要让一批指定素数完全分裂，并把判别式、类数造成的损失控制在指数级。Golod--Shafarevich 理论保证这种塔不会在有限层终止；类群上的抽屉原理再从大量理想分解中提取出足够多的范数 1 元素。#src("UD-REMARKS", "https://arxiv.org/abs/2605.20695")

#note[为什么“投影回二维”并不神奇地保长度][
  一般的高维投影当然会改变长度。这里能成功，不是因为投影保距，而是每个特制方向 $u$ 在*每一个*复坐标中本来就有绝对值 1；选第一坐标后，它仍恰好是单位向量。单射性则另外保证点的数量没有塌缩。
]

九位数学家随后发表了人类消化版，明确称其为 human-verified version，并梳理了与 Ellenberg--Venkatesh、Golod--Shafarevich 及类域塔方法的联系。这个跨越的本质是：旧构造只从一个固定二维格子的算术中榨取 $n^o(1)$ 个方向；新构造让数域次数随规模增长，从而得到指数于次数的方向数，最终在点数中表现为固定幂 $n^delta$。

= 现在还剩下什么

反例改变了下界的性质，却远没有确定 $nu(n)$ 的真实增长速度。目前可以粗略记成

$ n^(1+delta) <= nu(n) <= O(n^(4/3)) $

其中左式只对某个固定 $delta>0$ 和无穷多个 $n$ 保证成立；右式则是对所有 $n$ 的通用上界。两端之间仍有固定的幂指数鸿沟。

具体来说，新结果没有：

- 求出 $nu(n)$ 的精确渐近式；
- 证明最佳下界指数接近 $4/3$；
- 证明对每个充分大的 $n$ 都有同样的 $n^(1+delta)$ 下界；
- 给出小规模 $nu(n)$ 的统一最优构型；
- 让单位距离问题、不同距离问题或平面染色问题自动得到解决。

#danger[一句话准确总结][
  被推翻的是“平面单位距离数至多为 $n^(1+O(1/log log n))$”这一增长形态；仍然开放的是 $nu(n)$ 究竟以哪个固定幂次、甚至是否以某个稳定幂次增长。
]

= 参考资料

#link("https://www.bilibili.com/video/BV1G7jJ6nEbV")[单位距离猜想攻破详解：人类数学家离失业还有多远？]
