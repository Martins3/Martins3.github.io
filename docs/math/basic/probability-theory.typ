#set heading(numbering: "1.")
#set text(font: "Noto Serif CJK SC", size: 11pt)
#set math.equation(numbering: "(1)")

#let exp = math.op("exp")
#let var = math.op("Var")
#let cov = math.op("Cov")
#let corr = math.op("Corr")
#let e = math.op("E")
#let p = math.op("P")
#let iid = math.op("i.i.d.")
#let poisson = math.op("Poisson")
#let normal = math.op("N")
#let binomial = math.op("Bin")
#let bernoulli = math.op("Bernoulli")
#let uniform = math.op("Unif")
#let gamma = math.op("Gamma")
#let beta = math.op("Beta")
#let geom = math.op("Geom")
#let chi = math.op("chi")
#let covmat = math.op("Cov")
#let ind = math.op("1")

#align(center)[#text(size: 20pt, weight: "bold")[概率论知识大纲]]

本文整理大学数学系需要掌握的概率论主线。重点不是孤立罗列概念，而是说明概念之间的依赖关系，以及每一层工具解决什么问题。

#text(size: 16pt, weight: "bold")[目录]

#outline(title: none, depth: 3)

#pagebreak()

= 总体逻辑

概率论的主线可以这样组织：

1. 样本空间和事件
2. 概率测度
3. 随机变量
4. 分布
5. 期望和积分
6. 独立性和条件概率
7. 收敛和极限定理
8. 随机过程
9. 统计推断和应用

其中最核心的抽象是：

- 事件是集合，概率是集合上的测度。
- 随机变量是从样本空间到数值空间的可测函数。
- 分布把随机变量的问题转化为数轴、欧氏空间或者一般空间上的测度问题。
- 期望是对随机变量做积分，不只是“平均值”。
- 极限定理解释大量随机现象为什么会稳定，或者为什么会近似正态。

= 概率空间：事件和概率测度

== 基本对象

概率论首先需要一个可以谈论“事件”的空间。一个概率空间写作：

$ (Omega, cal(F), P) $

其中：

- $Omega$ 是样本空间，表示所有可能结果。
- $cal(F)$ 是 $Omega$ 上的 sigma 代数，表示允许讨论概率的事件族。
- $P$ 是概率测度。

概率测度满足三条公理：

$ P(A) >= 0 $
$ P(Omega) = 1 $
$ P(union_(n=1)^oo A_n) = sum_(n=1)^oo P(A_n) quad "当 " A_i inter A_j = emptyset, i != j $

== 关键公式

补事件：

$ P(A^c) = 1 - P(A) $

容斥公式的二事件版本：

$ P(A union B) = P(A) + P(B) - P(A inter B) $

连续性：

$ A_n " increases to " A => P(A_n) -> P(A) $
$ A_n " decreases to " A => P(A_n) -> P(A) $

== 逻辑关系

- 没有 sigma 代数，就无法严谨讨论哪些集合可以赋予概率。
- 没有可数可加性，就无法处理极限事件，例如“无限次试验中至少发生一次”。
- 后面的随机变量、分布、条件期望，本质都建立在概率空间上。

需要掌握：

- 用集合语言表示事件。
- 证明事件之间的包含、互斥、独立。
- 使用概率公理推导基本公式。
- 理解 Borel 集合为什么自然出现在实值随机变量中。

= 随机变量和分布

== 随机变量

随机变量不是“会随机变化的数”，而是定义在概率空间上的可测函数：

$ X: Omega -> RR $

可测性保证下面这种事件属于 $cal(F)$：

$ {omega in Omega : X(omega) <= x} in cal(F) $

分布函数定义为：

$ F_X(x) = P(X <= x) $

更一般地，$X$ 的分布是 $RR$ 上的概率测度：

$ mu_X(B) = P(X in B), quad B in cal(B)(RR) $

== 离散和连续情形

离散随机变量：

$ P(X in A) = sum_(x in A) P(X = x) $

连续随机变量：

$ P(a <= X <= b) = integral_a^b f_X(x) dif x $

其中 $f_X$ 是密度函数，并满足：

$ f_X(x) >= 0, quad integral_(-oo)^oo f_X(x) dif x = 1 $

分布函数和密度的关系：

$ F_X(x) = integral_(-oo)^x f_X(t) dif t, quad f_X(x) = F_X'(x) $

== 随机向量

随机向量的联合分布：

$ F_(X,Y)(x,y) = P(X <= x, Y <= y) $

边缘分布：

$ f_X(x) = integral_(-oo)^oo f_(X,Y)(x,y) dif y $
$ f_Y(y) = integral_(-oo)^oo f_(X,Y)(x,y) dif x $

== 逻辑关系

- 随机变量把抽象的随机结果变成可以计算的数值。
- 分布把 $P(X in A)$ 从样本空间搬到数值空间。
- 联合分布描述多个随机变量的整体关系，边缘分布只是其中一部分信息。

需要掌握：

- 从分布函数判断随机变量类型。
- 在离散、连续、混合情形下计算概率。
- 从联合分布求边缘分布。
- 对随机变量做函数变换，例如 $Y = g(X)$。
- 理解“同分布”和“相等”不是一回事。

= 常见分布

常见分布不是背公式，而是理解它们对应的随机机制。

== 离散分布

Bernoulli 分布：

$ X ~ bernoulli(p), quad P(X = 1) = p, quad P(X = 0) = 1 - p $

Binomial 分布：

$ X ~ binomial(n, p), quad P(X = k) = binom(n, k) p^k (1-p)^(n-k) $

Geometric 分布：

$ X ~ geom(p), quad P(X = k) = (1-p)^(k-1) p, quad k = 1, 2, dots $

Poisson 分布：

$ X ~ poisson(lambda), quad P(X = k) = e^(-lambda) lambda^k / k!, quad k = 0, 1, 2, dots $

Poisson 分布是二项分布的稀有事件极限：

$ binomial(n, lambda / n) -> poisson(lambda) $

== 连续分布

Uniform 分布：

$ X ~ uniform(a,b), quad f(x) = 1/(b-a) ind_(a <= x <= b) $

Exponential 分布：

$ X ~ "Exp"(lambda), quad f(x) = lambda e^(-lambda x) ind_(x >= 0) $

无记忆性：

$ P(X > s + t | X > s) = P(X > t) $

Gamma 分布：

$ X ~ gamma(alpha, lambda), quad f(x) = lambda^alpha / Gamma(alpha) x^(alpha - 1) e^(-lambda x), quad x > 0 $

Normal 分布：

$ X ~ normal(mu, sigma^2), quad f(x) = 1/(sqrt(2 pi) sigma) exp(- (x - mu)^2 / (2 sigma^2)) $

== 统计中常见分布

若 $Z_i ~ normal(0,1)$ 且相互独立，则：

$ sum_(i=1)^n Z_i^2 ~ chi_n^2 $

若 $Z ~ normal(0,1)$，$U ~ chi_n^2$ 且独立，则：

$ T = Z / sqrt(U / n) ~ t_n $

这些分布是正态样本推断、置信区间和假设检验的基础。

== 逻辑关系

- Bernoulli $->$ Binomial $->$ Poisson 是从单次试验到计数极限。
- Exponential $->$ Gamma 是从一次等待到多次等待。
- Normal $->$ Chi-square/t/F 是数理统计的重要基础。
- Multinomial $->$ Dirichlet 常用于贝叶斯统计。

= 期望、方差和积分观点

== 期望

离散情形：

$ E[X] = sum_x x P(X = x) $

连续情形：

$ E[X] = integral_(-oo)^oo x f_X(x) dif x $

一般情形中，期望是 Lebesgue 积分：

$ E[X] = integral_Omega X dif P $

函数的期望可以直接对分布积分：

$ E[g(X)] = integral_RR g(x) dif mu_X(x) $

连续情形写成：

$ E[g(X)] = integral_(-oo)^oo g(x) f_X(x) dif x $

== 方差和协方差

$ var(X) = E[(X - E[X])^2] = E[X^2] - (E[X])^2 $

$ cov(X,Y) = E[(X - E[X])(Y - E[Y])] = E[X Y] - E[X] E[Y] $

$ corr(X,Y) = cov(X,Y) / sqrt(var(X) var(Y)) $

== 常用不等式

Markov 不等式：

$ P(X >= a) <= E[X] / a quad (X >= 0, a > 0) $

Chebyshev 不等式：

$ P(|X - E[X]| >= epsilon) <= var(X) / epsilon^2 $

Jensen 不等式：

$ phi(E[X]) <= E[phi(X)] quad "当 " phi " 为凸函数" $

Cauchy-Schwarz 不等式：

$ |E[X Y]| <= sqrt(E[X^2] E[Y^2]) $

== 逻辑关系

- 期望是对随机变量积分。
- 方差是二阶矩给出的波动尺度。
- 协方差描述两个随机变量的线性关联。
- 不等式把难以精确计算的概率转化为可控上界。

= 独立性和条件概率

== 条件概率

条件概率定义：

$ P(A | B) = P(A inter B) / P(B), quad P(B) > 0 $

乘法公式：

$ P(A inter B) = P(A | B) P(B) $

全概率公式：

$ P(A) = sum_i P(A | B_i) P(B_i) $

Bayes 公式：

$ P(B_j | A) = (P(A | B_j) P(B_j)) / (sum_i P(A | B_i) P(B_i)) $

== 独立性

事件独立：

$ P(A inter B) = P(A) P(B) $

随机变量独立：

$ P(X in A, Y in B) = P(X in A) P(Y in B) $

有密度时，独立性等价于：

$ f_(X,Y)(x,y) = f_X(x) f_Y(y) $

== 条件期望初步

条件期望可以理解为给定信息后的最佳预测：

$ E[X | Y] = g(Y) $

并满足：

$ E[X] = E[E[X | Y]] $

如果 $X$ 和 $Y$ 独立，则：

$ E[X | Y] = E[X] $

== 逻辑关系

- 条件概率描述信息更新之后概率如何变化。
- 独立性表示一个对象不提供关于另一个对象的信息。
- 条件期望是给定信息后的最佳预测，也是鞅论的基础。

= 生成函数、特征函数和变换方法

这些工具的作用是把分布问题变成函数问题。

概率母函数：

$ G_X(s) = E[s^X] = sum_(k=0)^oo P(X = k) s^k $

矩母函数：

$ M_X(t) = E[e^(t X)] $

特征函数：

$ phi_X(t) = E[e^(i t X)] $

Laplace 变换：

$ L_X(t) = E[e^(-t X)] $

独立和的特征函数：

$ X " 与 " Y " 独立" => phi_(X+Y)(t) = phi_X(t) phi_Y(t) $

卷积公式：

$ f_(X+Y)(z) = integral_(-oo)^oo f_X(x) f_Y(z - x) dif x $

逻辑关系：

- 独立随机变量之和对应分布卷积。
- 卷积在变换之后变成乘法。
- 特征函数总是存在，比矩母函数更一般。
- 极限定理常通过特征函数证明。

= 收敛概念

概率论里有多种收敛，它们强弱不同。

几乎处处收敛：

$ P(lim_(n->oo) X_n = X) = 1 $

依概率收敛：

$ forall epsilon > 0, quad P(|X_n - X| > epsilon) -> 0 $

$ L^p $ 收敛：

$ E[|X_n - X|^p] -> 0 $

依分布收敛：

$ F_(X_n)(x) -> F_X(x) quad "在 " F_X " 的连续点" $

常见蕴含关系：

$ X_n -> X " in " L^p => X_n -> X " in probability" => X_n -> X " in distribution" $

几乎处处收敛也推出依概率收敛：

$ X_n -> X " a.s." => X_n -> X " in probability" $

Borel-Cantelli 引理：

$ sum_(n=1)^oo P(A_n) < oo => P(A_n " i.o.") = 0 $

如果 $A_n$ 相互独立，并且 $sum_n P(A_n) = oo$，则：

$ P(A_n " i.o.") = 1 $

一致可积用于把收敛和期望交换，是理解条件期望和极限定理的重要技术条件。

= 大数定律和中心极限定理

== 大数定律

设 $X_1, X_2, dots$ 独立同分布，$E[X_1] = mu$，样本均值为：

$ bar(X)_n = 1/n sum_(i=1)^n X_i $

弱大数定律：

$ bar(X)_n -> mu " in probability" $

强大数定律：

$ bar(X)_n -> mu " a.s." $

直观含义：大量独立重复实验的平均值会稳定到期望。

== 中心极限定理

若 $X_i$ 独立同分布，$E[X_i] = mu$，$var(X_i) = sigma^2 < oo$，则：

$ (sum_(i=1)^n X_i - n mu) / (sigma sqrt(n)) -> normal(0,1) " in distribution" $

等价地：

$ sqrt(n) (bar(X)_n - mu) / sigma -> normal(0,1) $

Delta 方法：

$ sqrt(n)(T_n - theta) -> normal(0, sigma^2) => sqrt(n)(g(T_n) - g(theta)) -> normal(0, (g'(theta))^2 sigma^2) $

Slutsky 定理的常用形式：

$ X_n -> X " in distribution", quad Y_n -> c " in probability" => X_n + Y_n -> X + c $

$ X_n Y_n -> c X " in distribution" $

== 逻辑关系

- 大数定律说明样本平均会稳定到期望。
- 中心极限定理说明样本平均的波动在适当缩放后趋向正态。
- Slutsky 定理和 Delta 方法把极限定理扩展到函数和估计量。

= 条件期望、鞅和停时

== 条件期望

设 $cal(G) subset cal(F)$。条件期望 $E[X | cal(G)]$ 是 $cal(G)$-可测随机变量，并满足：

$ integral_G E[X | cal(G)] dif P = integral_G X dif P, quad forall G in cal(G) $

塔性质：

$ E[E[X | cal(G)]] = E[X] $

若 $cal(H) subset cal(G)$，则：

$ E[E[X | cal(G)] | cal(H)] = E[X | cal(H)] $

== 鞅

给定滤过 $cal(F)_0 subset cal(F)_1 subset dots$，过程 $(X_n)$ 是鞅，如果：

$ X_n " is " cal(F)_n "-measurable" $
$ E[|X_n|] < oo $
$ E[X_(n+1) | cal(F)_n] = X_n $

上鞅和下鞅分别把最后一个等号改成 $<=$ 和 $>=$。

停时：

$ tau " 是停时" <=> {tau <= n} in cal(F)_n $

Optional stopping theorem 的典型结论是，在适当有界性或可积性条件下：

$ E[X_tau] = E[X_0] $

== 逻辑关系

- 条件期望是“给定信息后的平均”。
- 鞅描述公平游戏或者无漂移过程。
- 停时描述依赖历史信息的随机时间。
- 鞅工具可以处理随机过程中的最大值、收敛和停止问题。

= 随机过程

随机过程是按时间索引的一族随机变量：

$ {X_t : t in T} $

== Markov 链

Markov 性：

$ P(X_(n+1) = j | X_n = i, X_(n-1), dots, X_0) = P(X_(n+1) = j | X_n = i) $

转移矩阵：

$ P = (p_(i j)), quad p_(i j) = P(X_(n+1) = j | X_n = i) $

$ n $ 步转移概率：

$ P^(n) = P^n $

平稳分布：

$ pi P = pi, quad sum_i pi_i = 1 $

== Poisson 过程

计数过程 $(N_t)_(t >= 0)$ 是强度为 $lambda$ 的 Poisson 过程时：

$ N_t ~ poisson(lambda t) $

独立增量：

$ N_t - N_s " 与过去独立" quad (t > s) $

平稳增量：

$ N_t - N_s ~ poisson(lambda (t-s)) $

== Brownian motion

标准 Brownian motion $(B_t)_(t >= 0)$ 满足：

$ B_0 = 0 $
$ B_t - B_s ~ normal(0, t-s), quad t > s $

并且具有独立增量和连续路径。

== 逻辑关系

- 随机变量描述一个随机对象，随机过程描述随时间演化的随机对象。
- Markov 性表示未来只依赖当前状态。
- Poisson 过程是连续时间计数模型。
- Brownian motion 是连续时间极限模型，也是随机分析的入口。

= 与数理统计的接口

概率论提供模型，统计学研究如何从数据反推模型。

== 样本和似然

设样本 $X_1, dots, X_n$ 来自密度或概率质量函数 $f(x | theta)$。似然函数为：

$ L(theta) = product_(i=1)^n f(X_i | theta) $

对数似然：

$ ell(theta) = sum_(i=1)^n log f(X_i | theta) $

最大似然估计：

$ hat(theta) = arg max_theta L(theta) = arg max_theta ell(theta) $

== 估计量性质

无偏性：

$ E[hat(theta)] = theta $

一致性：

$ hat(theta)_n -> theta " in probability" $

渐近正态性：

$ sqrt(n)(hat(theta)_n - theta) -> normal(0, I(theta)^(-1)) $

== 假设检验

p-value 的形式可以写成：

$ "p-value" = P_(H_0)(T(X) >= T(x_"obs")) $

它表示在零假设成立时，得到当前这样极端或更极端统计量的概率。

== 逻辑关系

- 概率论：已知分布，推导数据性质。
- 数理统计：已知数据，推断分布或者参数。
- 极限定理是统计推断中近似方法的理论基础。

= 更高阶方向

完成前面的内容之后，可以继续深入：

- 测度论概率。
- 随机微积分。
- 随机微分方程。
- 信息论。
- 大偏差理论。
- 随机场。
- 随机矩阵。
- 高维概率。
- 随机算法。
- 概率图模型。
- 因果推断。

这些方向的依赖关系大致是：

- 测度论概率 $->$ 条件期望、鞅、随机过程。
- Brownian motion $->$ 随机微积分 $->$ 随机微分方程。
- 极限定理 $->$ 大偏差、高维概率、随机矩阵。
- 条件独立 $->$ 概率图模型、因果推断。
- 熵和 KL 散度 $->$ 信息论、统计学习、变分推断。

= 推荐学习顺序

1. 先学概率空间、随机变量、分布、期望。
2. 然后学常见分布、条件概率、独立性。
3. 接着学不等式、收敛概念、大数定律、中心极限定理。
4. 再学条件期望、鞅、Markov 链、Poisson 过程、Brownian motion。
5. 最后把概率论和数理统计、随机过程、高维概率连接起来。

最小闭环：

1. 概率空间
2. 随机变量
3. 分布
4. 期望
5. 独立性
6. 条件概率
7. 大数定律
8. 中心极限定理

如果这个闭环清楚，概率论的大部分基础内容就能组织起来。

= 资源

https://news.ycombinator.com/item?id=49583440

- https://seeing-theory.brown.edu/
- https://mbmlbook.com/
- https://xcelab.net/rm/

= 问题

- 为什么说，概率论还有学派的划分?

- 中新极限定理如何推出来的?
- Point Estimation
- Confidence Interval
- Bootstrap : 几种分布
	- uniform
	- Exponential
	- normal
	- student t
	- Chi-square
	- fisher snedecor
- Bayes' Theorem
- Likelihood Function
	- 这里又有好几种
- Prior to Posterior

- Regression Analysis
	- Ordinary Least Squares
	- Correlation


已知 10 个晶体管中有 7 个正品及 3 个次品, 每次任意抽取一个进行测试, 测试后不放回, 直到把 3 个次品都找到为止, 则需要测试 7 次的概率为
2/ 15 ，那么是对还是错
A. 对
B. 错
