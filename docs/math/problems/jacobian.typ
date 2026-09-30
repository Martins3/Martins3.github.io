#set document(
  title: "三维 Jacobian 猜想反例",
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

= 三维 Jacobian 猜想反例：结论巨大，材料仍待沉淀

Jacobian 猜想说：对复数域上的多项式映射 $F: CC^n -> CC^n$，如果 Jacobian 行列式处处是非零常数，那么 $F$ 应有多项式逆。它由 Keller 于 1939 年提出。

2026 年 7 月，Levent Alpöge 与 Claude Fable 5 公布下面的三维显式映射。令 $u=1+x y$，

$
  F(x,y,z) = (
    u^3 z + y^2 u(4+3 x y),
    y + 3 x u^2 z + 3 x y^2(4+3 x y),
    2 x - 3 x^2 y - x^3 z
  ).
$

公布材料声称

$ det J_F = -2, $

但三个不同实点

$ (0,0,-1/4), quad (1,-3/2,13/2), quad (-1,3/2,13/2) $

都映到 $(-1/4,0,0)$。常数非零 Jacobian 与非单射同时成立，便构成 $CC^3$ 上的反例；再添加恒等坐标即可推广到所有 $n >= 3$。二维情形仍然开放。#src("JAC-FIBER", "https://jacobianconjectures.com/fiber")

#warning[截至 2026-08-28 的谨慎结论][
  这是高度具体、原则上可通过符号代数直接核验的反例，远强于只有标题的宣告；相关综述也把它列为 2026 年 AI 数学事件。但主要公开材料仍是公告、网站与验证笔记，而不是已经完成传统同行评审的期刊论文。本文把它列为 B：很可能成立，但不把“87 年问题已尘埃落定”写成无条件事实。
]

= 参考
#link("https://jacobianconjectures.com/fiber")[The Fiber]：三维 Jacobian 反例的显式公式与纤维解释。
