#set heading(numbering: "1.")
#set text(font: "Noto Serif CJK SC", size: 11pt)
#set math.equation(numbering: "(1)")

#let sup = math.op("sup")
#let inf = math.op("inf")
#let limsup = math.op("limsup")
#let liminf = math.op("liminf")
#let dist = math.op("dist")
#let diam = math.op("diam")
#let osc = math.op("osc")
#let grad = math.op("grad")
#let div = math.op("div")
#let curl = math.op("curl")
#let esssup = math.op("ess sup")
#let span = math.op("span")
#let dim = math.op("dim")
#let Ker = math.op("Ker")
#let Im = math.op("Im")
#let ind = math.op("1")

#align(center)[#text(size: 20pt, weight: "bold")[分析知识大纲]]

本文整理大学数学系需要掌握的分析学主线。这里的“分析”包括数学分析、实分析、复分析、泛函分析，以及它们和微分方程、概率论、几何的接口。

分析学的核心问题是：研究极限过程。只要出现“无限逼近”“连续变化”“无穷级数”“函数列”“积分”“微分方程”，就会进入分析学的范围。

#text(size: 16pt, weight: "bold")[目录]

#outline(title: none, depth: 3)

#pagebreak()

= 总体逻辑

分析学的主线可以这样组织：

1. 实数完备性
2. 极限
3. 连续
4. 微分
5. 积分
6. 级数
7. 一致收敛
8. 度量空间
9. 测度和 Lebesgue 积分
10. 函数空间
11. 泛函分析、复分析、PDE

其中最核心的抽象是：

- 极限是分析学的基本语言。
- 实数完备性保证很多极限对象确实存在。
- 连续性说明函数和极限可以交换。
- 微分研究局部线性近似。
- 积分研究整体累积。
- 一致收敛控制函数列极限能否保留连续、可微、可积等性质。
- 测度论把“长度、面积、体积、概率”统一为集合函数。
- 泛函分析把函数看成向量空间中的点。

最小闭环：

1. 完备性
2. 极限存在
3. 连续性
4. 紧性
5. 一致控制

这个闭环贯穿数学分析、实分析、泛函分析和偏微分方程。

= 实数系和完备性

== 为什么需要实数

有理数不足以承载分析学，因为很多自然极限不落在有理数中。例如：

$ x_n^2 -> 2 $

在 $QQ$ 中没有极限 $sqrt(2)$。所以分析学需要 $RR$ 的完备性。

== 上确界性质

实数完备性的常用表述是上确界性质：

$ A subset RR, A != emptyset, A " 有上界" => exists sup A in RR $

下确界类似：

$ inf A = - sup(-A) $

== Cauchy 完备性

数列 $(x_n)$ 是 Cauchy 列，如果：

$ forall epsilon > 0, exists N, forall m,n >= N, |x_m - x_n| < epsilon $

实数完备性等价于：

$ (x_n) " 是 Cauchy 列" => exists x in RR, x_n -> x $

== 单调收敛定理

若 $(x_n)$ 单调递增且有上界，则：

$ x_n -> sup {x_n : n in NN} $

若 $(x_n)$ 单调递减且有下界，则：

$ x_n -> inf {x_n : n in NN} $

== 逻辑关系

- 上确界性质是很多存在性定理的源头。
- Cauchy 完备性说明“内部越来越接近”的序列确实有极限。
- Bolzano-Weierstrass、闭区间套、Heine-Borel 等定理都依赖完备性。

= 数列、函数极限和连续性

== 数列极限

数列极限定义：

$ x_n -> a <=> forall epsilon > 0, exists N, forall n >= N, |x_n - a| < epsilon $

常用极限运算：

$ x_n -> a, y_n -> b => x_n + y_n -> a + b $

$ x_n y_n -> a b $

若 $b != 0$ 且 $y_n != 0$，则：

$ x_n / y_n -> a / b $

== 函数极限

函数极限定义：

$ lim_(x -> a) f(x) = L $

表示：

$ forall epsilon > 0, exists delta > 0, 0 < |x-a| < delta => |f(x)-L| < epsilon $

== 连续性

函数 $f$ 在 $a$ 处连续：

$ lim_(x -> a) f(x) = f(a) $

等价的序列刻画：

$ x_n -> a => f(x_n) -> f(a) $

== 闭区间上的连续函数

若 $f$ 在 $[a,b]$ 上连续，则：

最大最小值定理：

$ exists x_1,x_2 in [a,b], f(x_1) <= f(x) <= f(x_2) $

介值定理：

$ f(a) <= y <= f(b) => exists c in [a,b], f(c) = y $

一致连续：

$ forall epsilon > 0, exists delta > 0, forall x,y in [a,b], |x-y| < delta => |f(x)-f(y)| < epsilon $

== 逻辑关系

- 极限是连续性的基础。
- 连续函数把接近的输入送到接近的输出。
- 紧集上的连续函数有更强性质：有界、取到最大最小值、一致连续。

= 微分：局部线性化

== 导数

一元函数导数：

$ f'(a) = lim_(h -> 0) (f(a+h) - f(a)) / h $

导数表示函数在一点附近的最佳线性近似：

$ f(a+h) = f(a) + f'(a) h + o(h) $

== 基本求导公式

乘积法则：

$ (f g)' = f' g + f g' $

链式法则：

$ (f circle g)'(x) = f'(g(x)) g'(x) $

反函数求导：

$ (f^(-1))'(y) = 1 / f'(x), quad y = f(x) $

== 中值定理

Rolle 定理：

$ f(a) = f(b) => exists c in (a,b), f'(c) = 0 $

Lagrange 中值定理：

$ exists c in (a,b), f'(c) = (f(b)-f(a))/(b-a) $

Taylor 公式：

$ f(x) = sum_(k=0)^n f^((k))(a) / k! (x-a)^k + R_n(x) $

Lagrange 余项：

$ R_n(x) = f^((n+1))(xi) / (n+1)! (x-a)^(n+1) $

== 多元微分

偏导：

$ partial_i f(a) = lim_(h -> 0) (f(a + h e_i) - f(a)) / h $

梯度：

$ grad f = (partial_1 f, dots, partial_n f) $

全微分：

$ f(a+h) = f(a) + D f(a) h + o(norm(h)) $

Jacobian 矩阵：

$ D f(a) = mat(partial f_1 / partial x_1, dots, partial f_1 / partial x_n; dots.v, dots.down, dots.v; partial f_m / partial x_1, dots, partial f_m / partial x_n) $

Hessian 矩阵：

$ H_f(a) = (partial_i partial_j f(a))_(i,j) $

== 逻辑关系

- 导数是函数的局部线性近似。
- 中值定理把局部导数信息转化为整体函数信息。
- Taylor 公式把函数近似为多项式，是分析估计和数值计算的基础。
- 多元微分把“局部线性化”从斜率推广为线性映射。

= Riemann 积分和基本定理

== Riemann 积分

划分：

$ P: a = x_0 < x_1 < dots < x_n = b $

Riemann 和：

$ sum_(i=1)^n f(xi_i) (x_i - x_(i-1)) $

若划分越来越细时 Riemann 和趋于同一极限，则定义：

$ integral_a^b f(x) dif x $

== 微积分基本定理

若 $f$ 连续，定义：

$ F(x) = integral_a^x f(t) dif t $

则：

$ F'(x) = f(x) $

若 $F' = f$，则：

$ integral_a^b f(x) dif x = F(b) - F(a) $

== 常用积分方法

换元公式：

$ integral_a^b f(phi(t)) phi'(t) dif t = integral_(phi(a))^(phi(b)) f(x) dif x $

分部积分：

$ integral_a^b u dif v = u v |_a^b - integral_a^b v dif u $

== 反常积分

无穷区间：

$ integral_a^oo f(x) dif x = lim_(b -> oo) integral_a^b f(x) dif x $

奇点：

$ integral_a^b f(x) dif x = lim_(t -> a+) integral_t^b f(x) dif x $

== 逻辑关系

- Riemann 积分把曲线下方面积定义为极限。
- 微积分基本定理说明微分和积分互为逆过程。
- 反常积分把积分扩展到无界区间和无界函数。

= 级数和函数列

== 数项级数

级数：

$ sum_(n=1)^oo a_n $

部分和：

$ s_N = sum_(n=1)^N a_n $

级数收敛定义：

$ sum_(n=1)^oo a_n " 收敛" <=> s_N " 有极限" $

必要条件：

$ sum_(n=1)^oo a_n " 收敛" => a_n -> 0 $

绝对收敛：

$ sum_(n=1)^oo |a_n| < oo => sum_(n=1)^oo a_n " 收敛" $

== 常用判别法

比较判别：

$ 0 <= a_n <= b_n, sum b_n < oo => sum a_n < oo $

比值判别：

$ lim_(n -> oo) |a_(n+1) / a_n| = L < 1 => sum a_n " 绝对收敛" $

根值判别：

$ limsup_(n -> oo) root(n, |a_n|) < 1 => sum a_n " 绝对收敛" $

交错级数判别：

$ a_n " decreases to " 0 => sum_(n=1)^oo (-1)^(n-1) a_n " 收敛" $

== 函数列和一致收敛

逐点收敛：

$ f_n(x) -> f(x) quad "对每个 " x $

一致收敛：

$ sup_(x in E) |f_n(x) - f(x)| -> 0 $

Weierstrass 判别法：

$ |f_n(x)| <= M_n, sum M_n < oo => sum f_n " 一致收敛" $

一致极限定理：

$ f_n " 连续且 " f_n -> f " 一致" => f " 连续" $

逐项积分：

$ f_n -> f " 一致" => integral_a^b f_n -> integral_a^b f $

逐项求导的常用条件：

$ f_n' -> g " 一致，且 " f_n(x_0) " 收敛" => f_n -> f, f' = g $

== 幂级数

幂级数：

$ sum_(n=0)^oo a_n (x-a)^n $

收敛半径：

$ R = 1 / limsup_(n -> oo) root(n, |a_n|) $

在收敛区间内部可以逐项求导和逐项积分：

$ (sum_(n=0)^oo a_n x^n)' = sum_(n=1)^oo n a_n x^(n-1) $

== 逻辑关系

- 级数是无限加法，必须用极限定义。
- 绝对收敛比条件收敛稳定。
- 一致收敛控制“极限函数是否保留原函数性质”。
- 幂级数把分析函数和代数运算联系起来。

= 度量空间和拓扑语言

== 度量空间

度量空间是集合 $X$ 加上距离函数：

$ d: X times X -> RR $

满足：

$ d(x,y) >= 0, quad d(x,y) = 0 <=> x = y $

$ d(x,y) = d(y,x) $

$ d(x,z) <= d(x,y) + d(y,z) $

开球：

$ B(x,r) = {y in X : d(x,y) < r} $

== 开集、闭集和紧集

开集：每个点都有一个小开球仍在集合内。

闭集：包含自己的所有极限点。

紧集的开覆盖定义：

$ K subset union_(alpha in A) U_alpha => exists alpha_1, dots, alpha_n, K subset union_(i=1)^n U_(alpha_i) $

在 $RR^n$ 中，Heine-Borel 定理：

$ K " 紧" <=> K " 闭且有界" $

== 完备性和压缩映射

度量空间中的 Cauchy 列：

$ forall epsilon > 0, exists N, m,n >= N => d(x_m,x_n) < epsilon $

完备性：

$ "每个 Cauchy 列都收敛到空间中的点" $

压缩映射：

$ d(T x,T y) <= q d(x,y), quad 0 < q < 1 $

Banach 不动点定理：

$ exists ! x^* in X, quad T x^* = x^* $

== 逻辑关系

- 度量空间把极限、连续、紧性从实数推广到抽象空间。
- 紧性是“有限性”的分析版本。
- 完备性保证迭代和极限过程不跑出空间。
- Banach 不动点定理是微分方程存在唯一性和数值迭代的基础。

= 测度和 Lebesgue 积分

== 测度空间

测度空间：

$ (X, cal(F), mu) $

测度满足：

$ mu(emptyset) = 0 $

$ mu(union_(n=1)^oo A_n) = sum_(n=1)^oo mu(A_n) quad "当 " A_i inter A_j = emptyset $

概率测度是总质量为 1 的测度：

$ mu(X) = 1 $

== 可测函数

函数 $f: X -> RR$ 可测，如果：

$ {x in X : f(x) > a} in cal(F) quad forall a in RR $

== Lebesgue 积分

非负简单函数：

$ s = sum_(i=1)^n a_i ind_(A_i) $

其积分定义为：

$ integral s dif mu = sum_(i=1)^n a_i mu(A_i) $

非负可测函数：

$ integral f dif mu = sup {integral s dif mu : 0 <= s <= f, s " 简单"} $

一般函数分解为：

$ f = f^+ - f^- $

若：

$ integral f^+ dif mu < oo " 且 " integral f^- dif mu < oo $

则 $f$ 可积。

== 三大收敛定理

单调收敛定理：

$ 0 <= f_n " increases to " f => integral f_n dif mu -> integral f dif mu $

Fatou 引理：

$ integral liminf_(n -> oo) f_n dif mu <= liminf_(n -> oo) integral f_n dif mu $

支配收敛定理：

$ f_n -> f " a.e.", |f_n| <= g, integral g dif mu < oo => integral f_n dif mu -> integral f dif mu $

== Lp 空间

$ L^p $ 范数：

$ norm(f)_p = (integral |f|^p dif mu)^(1/p), quad 1 <= p < oo $

$ L^oo $ 范数：

$ norm(f)_oo = esssup |f| $

Holder 不等式：

$ integral |f g| dif mu <= norm(f)_p norm(g)_q, quad 1/p + 1/q = 1 $

Minkowski 不等式：

$ norm(f+g)_p <= norm(f)_p + norm(g)_p $

== 逻辑关系

- 测度论把长度、面积、体积、概率统一起来。
- Lebesgue 积分比 Riemann 积分更适合处理极限。
- 三大收敛定理回答“什么时候极限和积分可以交换”。
- $L^p$ 空间把可积函数组织成向量空间，是泛函分析和 PDE 的基础。

= Fourier 分析

== Fourier 级数

周期函数可以尝试展开为三角级数：

$ f(x) ~ a_0 / 2 + sum_(n=1)^oo (a_n cos(n x) + b_n sin(n x)) $

系数：

$ a_n = 1/pi integral_(-pi)^pi f(x) cos(n x) dif x $

$ b_n = 1/pi integral_(-pi)^pi f(x) sin(n x) dif x $

复数形式：

$ f(x) ~ sum_(n in ZZ) c_n e^(i n x) $

$ c_n = 1/(2 pi) integral_(-pi)^pi f(x) e^(-i n x) dif x $

== Fourier 变换

Fourier 变换：

$ hat(f)(xi) = integral_(-oo)^oo f(x) e^(-i x xi) dif x $

反演公式：

$ f(x) = 1/(2 pi) integral_(-oo)^oo hat(f)(xi) e^(i x xi) dif xi $

卷积：

$ (f * g)(x) = integral_(-oo)^oo f(x-y) g(y) dif y $

卷积定理：

$ hat(f * g)(xi) = hat(f)(xi) hat(g)(xi) $

导数和频率：

$ hat(f')(xi) = i xi hat(f)(xi) $

== Parseval 公式

$ integral_(-pi)^pi |f(x)|^2 dif x = 2 pi sum_(n in ZZ) |c_n|^2 $

Fourier 分析的核心思想是：把函数分解到频率空间中，使微分、卷积、平移等操作变得更简单。

= 复分析

== 复可微和解析函数

复导数：

$ f'(z_0) = lim_(h -> 0) (f(z_0+h) - f(z_0)) / h $

这个极限要求 $h$ 从复平面任意方向趋向 0，因此比实可微强很多。

Cauchy-Riemann 方程。若 $f = u + i v$，则：

$ partial u / partial x = partial v / partial y $

$ partial u / partial y = - partial v / partial x $

== Cauchy 积分理论

Cauchy 积分定理：

$ integral_gamma f(z) dif z = 0 $

Cauchy 积分公式：

$ f(a) = 1/(2 pi i) integral_gamma f(z) / (z-a) dif z $

高阶导数公式：

$ f^((n))(a) = n!/(2 pi i) integral_gamma f(z) / (z-a)^(n+1) dif z $

== Laurent 级数和留数

Laurent 展开：

$ f(z) = sum_(n=-oo)^oo a_n (z-a)^n $

留数：

$ "Res"(f,a) = a_(-1) $

留数定理：

$ integral_gamma f(z) dif z = 2 pi i sum_k "Res"(f,a_k) $

== 逻辑关系

- 复可微远强于实可微，通常推出解析性。
- Cauchy 积分公式说明函数内部值由边界值决定。
- 留数定理把复杂积分转化为孤立奇点处的局部信息。

= 泛函分析

== 赋范空间和 Banach 空间

赋范空间：

$ norm(x) >= 0, norm(x) = 0 <=> x = 0 $

$ norm(alpha x) = |alpha| norm(x) $

$ norm(x+y) <= norm(x) + norm(y) $

Banach 空间是完备的赋范空间。

常见例子：

$ L^p, C([a,b]), ell^p $

== 内积空间和 Hilbert 空间

内积记作：

$ (x,y) $

满足线性、共轭对称和正定性。

范数由内积诱导：

$ norm(x) = sqrt((x,x)) $

Cauchy-Schwarz：

$ |(x,y)| <= norm(x) norm(y) $

Hilbert 空间是完备内积空间。

正交投影：

$ x = P_M x + (x - P_M x), quad x - P_M x " perpendicular to " M $

== 有界线性算子

线性算子：

$ T(alpha x + beta y) = alpha T x + beta T y $

有界性：

$ exists C, norm(T x) <= C norm(x) $

算子范数：

$ norm(T) = sup_(norm(x) <= 1) norm(T x) $

有界和连续等价：

$ T " 有界" <=> T " 连续" $

== 三大基本定理

Hahn-Banach 定理：有界线性泛函可以延拓，并保持范数。

开映射定理：Banach 空间之间的满射有界线性算子是开映射。

一致有界原理：

$ forall x, sup_alpha norm(T_alpha x) < oo => sup_alpha norm(T_alpha) < oo $

== 逻辑关系

- 泛函分析把函数当成无限维向量空间中的点。
- 完备性保证极限过程可控。
- Hilbert 空间把几何直觉推广到无限维。
- 算子理论是微分方程、量子力学、数值分析的共同语言。

= 微分方程接口

== 常微分方程

初值问题：

$ x'(t) = f(t, x(t)), quad x(t_0) = x_0 $

积分形式：

$ x(t) = x_0 + integral_(t_0)^t f(s, x(s)) dif s $

Picard 迭代：

$ x_(n+1)(t) = x_0 + integral_(t_0)^t f(s, x_n(s)) dif s $

存在唯一性通常由 Banach 不动点定理推出。

== 偏微分方程

Laplace 方程：

$ Delta u = 0 $

热方程：

$ partial_t u = Delta u $

波动方程：

$ partial_t^2 u = c^2 Delta u $

Poisson 方程：

$ - Delta u = f $

== 弱解思想

经典解要求函数足够光滑。弱解通过分部积分降低可微性要求。

例如 Poisson 方程的弱形式：

$ integral_Omega grad u dot grad v dif x = integral_Omega f v dif x quad forall v $

== 逻辑关系

- ODE 把微积分和不动点理论联系起来。
- PDE 需要 Fourier 分析、泛函分析、Sobolev 空间和弱解。
- 分析学中的紧性、完备性、估计，是证明解存在性的核心工具。

= 与其他数学分支的关系

== 和概率论

概率论可以看作测度空间上的分析：

$ E[X] = integral_Omega X dif P $

大数定律、中心极限定理、鞅收敛，都依赖极限和积分交换。

== 和几何

微分几何大量使用多元分析：

- 切空间来自局部线性化。
- 微分形式推广积分。
- Stokes 定理统一 Newton-Leibniz、Green、Gauss、Stokes 公式。

统一形式：

$ integral_M dif omega = integral_(partial M) omega $

== 和代数

复分析和代数几何通过解析函数、多项式零点、Riemann 曲面联系。

泛函分析中的算子代数把代数结构和拓扑、范数、谱理论结合起来。

== 和计算

数值分析本质上是“有限计算如何逼近无限对象”：

- 误差估计来自 Taylor 公式。
- 收敛性来自极限理论。
- 稳定性来自范数和算子估计。
- PDE 数值解依赖弱形式和函数空间。

= 推荐学习顺序

1. 实数完备性、数列极限、函数极限、连续性。
2. 一元微分、Riemann 积分、微积分基本定理。
3. 级数、函数列、一致收敛、幂级数。
4. 多元微分、重积分、曲线曲面积分。
5. 度量空间、紧性、完备性、压缩映射。
6. 测度论、Lebesgue 积分、$L^p$ 空间。
7. Fourier 分析、复分析。
8. 泛函分析、Sobolev 空间、偏微分方程。

学习分析时可以反复追问：

- 这个极限是否存在？
- 是否唯一？
- 能否和函数、积分、导数、求和交换？
- 需要什么一致性或支配条件？

如果能围绕这些问题组织知识，分析学的大部分结构就能串起来。
