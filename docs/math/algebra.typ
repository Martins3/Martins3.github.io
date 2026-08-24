#set heading(numbering: "1.")
#set text(font: "Noto Serif CJK SC", size: 11pt)
#set math.equation(numbering: "(1)")

#let Hom = math.op("Hom")
#let Aut = math.op("Aut")
#let End = math.op("End")
#let Ker = math.op("Ker")
#let Im = math.op("Im")
#let id = math.op("id")
#let ord = math.op("ord")
#let gcd = math.op("gcd")
#let lcm = math.op("lcm")
#let char = math.op("char")
#let Gal = math.op("Gal")
#let GL = math.op("GL")
#let Fix = math.op("Fix")
#let deg = math.op("deg")
#let tr = math.op("tr")
#let det = math.op("det")
#let rank = math.op("rank")
#let Spec = math.op("Spec")
#let Ann = math.op("Ann")
#let Ext = math.op("Ext")
#let Tor = math.op("Tor")
#let cong = math.op("cong")
#let oplus = math.op("oplus")

#align(center)[#text(size: 20pt, weight: "bold")[代数知识大纲]]

本文整理大学数学系需要掌握的代数主线。这里的“代数”主要指抽象代数及其后续方向，而不是高中意义上的代数计算。

代数的核心问题是：研究带有运算的集合，以及保持运算结构的映射。它关心的不是某个具体数怎么算，而是“运算规则”本身会强迫对象具有什么结构。

#text(size: 16pt, weight: "bold")[目录]

#outline(title: none, depth: 3)

#pagebreak()

= 总体逻辑

代数的主线可以这样组织：

1. 集合和映射
2. 运算和结构
3. 同态
4. 商结构
5. 群
6. 环
7. 模
8. 域
9. Galois 理论
10. 表示论、交换代数、同调代数

其中最核心的抽象是：

- 一个代数对象通常是集合加上一些运算。
- 同态是保持运算结构的映射。
- 核、像、商对象是理解结构的基本工具。
- 群抽象了“对称性”。
- 环抽象了“加法和乘法同时存在”的结构。
- 模是向量空间的推广，把线性代数放到环上。
- 域扩张和 Galois 理论把多项式根、对称性和可解性联系起来。

最小闭环：

1. 结构
2. 同态
3. 核
4. 商
5. 同构定理

这个闭环贯穿群、环、模、域扩张和表示论。

= 集合、映射和运算

== 集合和映射

代数对象首先是集合。映射是比较两个集合的基本方式：

$ f: A -> B $

常见性质：

- 单射：$f(a_1) = f(a_2) => a_1 = a_2$。
- 满射：$forall b in B, exists a in A, f(a) = b$。
- 双射：既是单射又是满射。

复合映射：

$ (g circle f)(a) = g(f(a)) $

恒等映射：

$ id_A(a) = a $

== 二元运算

集合 $S$ 上的二元运算是一个映射：

$ *: S times S -> S $

常见运算律：

$ (a * b) * c = a * (b * c) quad "结合律" $

$ a * b = b * a quad "交换律" $

单位元：

$ e * a = a * e = a $

逆元：

$ a * a^(-1) = a^(-1) * a = e $

== 逻辑关系

- 集合提供对象的载体。
- 运算赋予集合结构。
- 映射比较对象，同态比较带结构的对象。
- 结合律、单位元、逆元、交换律的不同组合，会产生半群、幺半群、群、环等结构。

= 群论：对称性的代数

== 群的定义

群是一个集合 $G$ 配备二元运算，使得：

$ (a b) c = a (b c) $

$ exists e in G, quad e a = a e = a $

$ forall a in G, exists a^(-1) in G, quad a a^(-1) = a^(-1) a = e $

如果还满足：

$ a b = b a $

则称为 Abel 群。

== 子群和生成元

子群 $H <= G$ 是在同一运算下仍为群的子集。

子群判别法：

$ H <= G <=> H != emptyset " 且 " forall a,b in H, a b^(-1) in H $

由集合 $S subset G$ 生成的子群记作：

$ chevron.l S chevron.r $

循环群：

$ chevron.l g chevron.r = {g^n : n in ZZ} $

元素阶：

$ ord(g) = min {n >= 1 : g^n = e} $

== 陪集和 Lagrange 定理

左陪集：

$ g H = {g h : h in H} $

右陪集：

$ H g = {h g : h in H} $

Lagrange 定理：

$ |G| = [G : H] |H| $

推论：

$ ord(g) divides |G| $

== 正规子群和商群

正规子群：

$ N " normal in " G <=> g N g^(-1) = N quad forall g in G $

商群：

$ G / N = {g N : g in G} $

商群运算：

$ (g N)(h N) = (g h) N $

这个运算良定义需要 $N " normal in " G$。

== 群同态

群同态：

$ phi: G -> H, quad phi(a b) = phi(a) phi(b) $

核：

$ Ker(phi) = {g in G : phi(g) = e_H} $

像：

$ Im(phi) = {phi(g) : g in G} $

同态基本定理：

$ G / Ker(phi) cong Im(phi) $

== 群作用

群作用是群对集合的对称操作：

$ G times X -> X, quad (g,x) -> g x $

满足：

$ e x = x $

$ (g h) x = g (h x) $

轨道：

$ G x = {g x : g in G} $

稳定子：

$ G_x = {g in G : g x = x} $

轨道-稳定子定理：

$ |G x| = [G : G_x] $

Burnside 引理：

$ |X / G| = 1 / |G| sum_(g in G) |Fix(g)| $

== 逻辑关系

- 子群描述局部结构。
- 陪集把群按子群分块。
- 正规子群让商群成为合法结构。
- 同态把一个群映到另一个群，核记录“被压扁”的部分。
- 群作用把抽象群变成具体对称操作，是连接几何、组合和表示论的桥。

= 环论：加法和乘法同时存在

== 环的定义

环 $R$ 是一个集合，带有加法和乘法，满足：

- $(R,+)$ 是 Abel 群。
- 乘法结合：$(a b)c = a(b c)$。
- 分配律成立：

$ a(b+c) = a b + a c $

$ (a+b)c = a c + b c $

如果乘法有单位元，记为 $1_R$。如果乘法交换，则称为交换环。

== 基本例子

- 整数环 $ZZ$。
- 多项式环 $F[x]$。
- 矩阵环 $M_n(F)$。
- 剩余类环 $ZZ / n ZZ$。
- 函数环。

这些例子说明：环不只是“数”，也可以是多项式、矩阵、函数和算子。

== 理想和商环

理想是可以做商环的子结构。左理想满足：

$ a,b in I => a - b in I $

$ r in R, a in I => r a in I $

交换环中，理想 $I$ 的商环：

$ R / I = {a + I : a in R} $

商环运算：

$ (a + I) + (b + I) = (a + b) + I $

$ (a + I)(b + I) = a b + I $

主理想：

$ (a) = {r a : r in R} $

== 环同态

环同态：

$ phi: R -> S $

满足：

$ phi(a+b) = phi(a) + phi(b) $

$ phi(a b) = phi(a) phi(b) $

通常还要求：

$ phi(1_R) = 1_S $

核：

$ Ker(phi) = {r in R : phi(r) = 0} $

环同态基本定理：

$ R / Ker(phi) cong Im(phi) $

== 整环、域和素理想

整环：交换环 $R$ 中没有零因子：

$ a b = 0 => a = 0 " 或 " b = 0 $

域：每个非零元素都有乘法逆元：

$ forall a != 0, exists a^(-1), quad a a^(-1) = 1 $

理想 $p$ 是素理想：

$ a b in p => a in p " 或 " b in p $

理想 $m$ 是极大理想：

$ m != R " 且不存在 " I " 满足 " m subset.eq I subset.eq R $

重要判别：

$ R / p " 是整环" <=> p " 是素理想" $

$ R / m " 是域" <=> m " 是极大理想" $

== Euclidean 整环、PID 和 UFD

Euclidean 整环 $->$ 主理想整环 $->$ 唯一分解整环：

$ "Euclidean domain" => "PID" => "UFD" $

整数和一元多项式环的整除理论有相同结构：

$ a = u p_1^(e_1) dots p_k^(e_k) $

其中 $u$ 是单位，$p_i$ 是不可约元。

== 逻辑关系

- 理想在环论中扮演正规子群的角色。
- 商环让“模掉某些关系”成为合法操作。
- 素理想和极大理想控制商环是否是整环或域。
- 整除、因式分解、多项式根都可以放到环论框架中统一处理。

= 模论：线性代数的推广

== 模的定义

设 $R$ 是环。左 $R$-模 $M$ 是一个 Abel 群，并且有标量乘法：

$ R times M -> M $

满足：

$ r(x+y) = r x + r y $

$ (r+s)x = r x + s x $

$ (r s)x = r(s x) $

$ 1_R x = x $

如果 $R$ 是域，则 $R$-模就是向量空间。

== 子模、商模和同态

子模 $N subset M$ 在加法和标量乘法下封闭。

商模：

$ M / N = {x + N : x in M} $

模同态：

$ f: M -> N, quad f(r x + y) = r f(x) + f(y) $

同态基本定理：

$ M / Ker(f) cong Im(f) $

== 自由模和有限生成模

自由模：

$ M cong R^n $

有限生成模：

$ M = R x_1 + dots + R x_n $

在域上，有限生成向量空间都有基。但在一般环上，模不一定有基，这是模论比线性代数复杂的地方。

== PID 上的有限生成模结构定理

若 $R$ 是 PID，有限生成 $R$-模 $M$ 可以分解为：

$ M cong R^r oplus R/(d_1) oplus dots oplus R/(d_k) $

其中：

$ d_1 divides d_2 divides dots divides d_k $

这个定理统一了两个重要结论：

- 有限生成 Abel 群分类。
- 矩阵的有理标准形和 Jordan 标准形的代数基础。

== 正合列

正合列：

$ dots -> M_(i-1) -> M_i -> M_(i+1) -> dots $

在 $M_i$ 处正合表示：

$ Im(f_(i-1)) = Ker(f_i) $

短正合列：

$ 0 -> A -> B -> C -> 0 $

表示 $A$ 嵌入 $B$，而 $C$ 是对应的商。

== 逻辑关系

- 向量空间是域上的模。
- Abel 群是 $ZZ$-模。
- 模论把线性代数、Abel 群、表示论放进同一语言。
- 正合列是现代代数中追踪结构损失和结构保留的基本工具。

= 域论和多项式

== 域扩张

如果 $F subset K$ 且 $K$ 是包含 $F$ 的域，则称 $K/F$ 是域扩张。

扩张次数：

$ [K:F] = dim_F K $

塔公式：

$ [L:F] = [L:K][K:F] $

== 代数元和超越元

元素 $alpha in K$ 在 $F$ 上代数，如果存在非零多项式 $f(x) in F[x]$，使得：

$ f(alpha) = 0 $

否则称为超越元。

最小多项式：

$ m_(alpha,F)(x) $

满足：

$ m_(alpha,F)(alpha) = 0 $

并且它是所有使 $alpha$ 为根的非零多项式中次数最低的首一多项式。

若 $alpha$ 在 $F$ 上代数，则：

$ [F(alpha):F] = deg m_(alpha,F)(x) $

== 分裂域和代数闭包

多项式 $f(x) in F[x]$ 的分裂域是包含 $F$ 并使 $f$ 分解成一次因子的最小域。

$ f(x) = c product_(i=1)^n (x - alpha_i) $

代数闭包 $bar(F)$ 满足：每个非常数多项式在其中都有根。

== 有限域

有限域的大小一定是素数幂：

$ |F| = p^n $

有限域的乘法群是循环群：

$ F_q^* cong C_(q-1) $

有限域元素满足：

$ a^q = a quad forall a in F_q $

因此：

$ x^q - x = product_(a in F_q) (x - a) $

== 逻辑关系

- 域是可以做除法的交换环。
- 多项式的根常常不在原域中，所以需要域扩张。
- 最小多项式控制单个代数元生成的扩张。
- 分裂域把一个多项式的所有根放进同一个最小环境。

= Galois 理论：方程根和对称性

== Galois 群

设 $K/F$ 是域扩张。Galois 群定义为：

$ Gal(K/F) = {sigma in Aut(K) : sigma(a) = a quad forall a in F} $

它描述扩张域中保持基域不动的自同构。

如果 $K$ 是 $f(x) in F[x]$ 的分裂域，则 $Gal(K/F)$ 会作用在 $f$ 的根上。

== 正规扩张、可分扩张和 Galois 扩张

有限扩张 $K/F$ 是 Galois 扩张，通常要求：

- 正规：不可约多项式只要在 $K$ 中有一个根，就在 $K$ 中完全分裂。
- 可分：最小多项式没有重根。

有限 Galois 扩张满足：

$ |Gal(K/F)| = [K:F] $

== Galois 基本定理

设 $K/F$ 是有限 Galois 扩张。中间域和子群之间存在反向对应：

$ E " 中间域" <-> H <= Gal(K/F) $

对应关系：

$ E -> Gal(K/E) $

$ H -> K^H = {x in K : sigma(x) = x quad forall sigma in H} $

并且：

$ [E:F] = [Gal(K/F) : Gal(K/E)] $

$ [K:E] = |Gal(K/E)| $

正规性对应：

$ E/F " 是 Galois 扩张" <=> Gal(K/E) " normal in " Gal(K/F) $

此时：

$ Gal(E/F) cong Gal(K/F) / Gal(K/E) $

== 根式可解

多项式方程根式可解，与其 Galois 群是否为可解群有关。

可解群有正规列：

$ {e} = G_0 " normal in " G_1 " normal in " dots " normal in " G_n = G $

并且每个商群 Abel：

$ G_(i+1) / G_i " 是 Abel 群" $

核心结论：

$ f(x) " 根式可解" <=> Gal(f) " 是可解群" $

一般五次方程不可根式求解，本质原因是一般五次多项式的 Galois 群为 $S_5$，而 $S_5$ 不可解。

== 逻辑关系

- 多项式根之间的置换形成群。
- 域扩张记录“为了加入根而扩大的数系”。
- Galois 群记录扩张中保留原域结构的对称性。
- 中间域和子群的对应把“域的问题”转化为“群的问题”。

= 表示论：用线性变换研究群和代数

== 群表示

群 $G$ 在向量空间 $V$ 上的表示是同态：

$ rho: G -> GL(V) $

满足：

$ rho(g h) = rho(g) rho(h) $

表示把抽象群元素变成矩阵，使群论可以使用线性代数工具。

== 特征标

有限维表示的特征标：

$ chi_rho(g) = tr(rho(g)) $

特征标内积：

$ chevron.l chi, psi chevron.r = 1 / |G| sum_(g in G) chi(g) overline(psi(g)) $

不可约表示的特征标满足正交关系。

== 群代数

群代数：

$ F[G] = {sum_(g in G) a_g g : a_g in F} $

乘法由群运算线性延拓：

$ (sum_g a_g g)(sum_h b_h h) = sum_(g,h) a_g b_h (g h) $

表示论也可以看作研究 $F[G]$-模。

== 逻辑关系

- 群表示把群作用线性化。
- 特征标把矩阵表示压缩成函数，但仍保留很多结构信息。
- 群代数把群论转化为环和模的问题。

= 交换代数：几何背后的环论

== 素谱

交换环 $R$ 的素谱：

$ Spec(R) = {p subset R : p " 是素理想"} $

这是代数几何中的基本对象。直观上，素理想可以看作广义的点。

== 局部化

给定乘法闭集 $S subset R$，局部化：

$ S^(-1) R = {a / s : a in R, s in S} $

它把 $S$ 中的元素都变成可逆元。

在素理想 $p$ 处的局部环：

$ R_p = (R - p)^(-1) R $

== Noether 环

Noether 环的定义：

$ I_1 subset.eq I_2 subset.eq dots => exists n, forall k >= n, I_k = I_n $

即理想升链稳定。

Hilbert 基定理：

$ R " Noether" => R[x] " Noether" $

== 逻辑关系

- 交换环的理想结构可以编码几何对象。
- 局部化表示只关注某个点或某个区域附近的信息。
- Noether 条件保证对象有限生成，避免无限复杂性失控。

= 同调代数：用正合性测量结构

== 为什么需要同调

很多自然函子不能保持正合。比如张量积通常是右正合，但不一定左正合；Hom 函子通常是左正合，但不一定右正合。

同调代数的思想是：用派生函子测量正合性失败的程度。

== Tor 和 Ext

Tor 测量张量积保持正合的失败：

$ Tor_1^R(M,N) $

Ext 测量 Hom 保持正合的失败：

$ Ext_R^1(M,N) $

粗略理解：

- $Tor$ 和张量积、扭结信息有关。
- $Ext$ 和扩张、分类短正合列有关。

短正合列：

$ 0 -> A -> B -> C -> 0 $

可以看成 $B$ 是用 $A$ 和 $C$ 拼出来的对象，而 $Ext^1(C,A)$ 控制这种拼法的分类。

== 链复形和同调群

链复形：

$ dots -> C_(n+1) -> C_n -> C_(n-1) -> dots $

满足：

$ d_n circle d_(n+1) = 0 $

同调群：

$ H_n(C) = Ker(d_n) / Im(d_(n+1)) $

含义：闭的对象模掉边界对象。

== 逻辑关系

- 正合列描述结构是否完整传递。
- 同调群测量“差一点正合”的部分。
- 同调代数为代数拓扑、代数几何、表示论和数论提供统一语言。

= 与其他数学分支的关系

== 和线性代数

线性代数是域上的模论：

$ "vector spaces over " F = F "-modules" $

矩阵相似、Jordan 标准形、有理标准形，都可以通过 $F[x]$-模理解。

== 和数论

整数环、代数整数环、理想分解构成代数数论基础。

理想分解形式：

$ (alpha) = p_1^(e_1) dots p_k^(e_k) $

唯一分解失败时，理想分解往往仍然可控。

== 和几何

代数几何的基本反向关系：

$ "几何对象" <-> "函数环" $

例如仿射代数簇和坐标环之间存在深刻对应。

== 和计算机科学

代数结构出现在：

- 编码理论：有限域和多项式。
- 密码学：有限群、有限域、椭圆曲线。
- 程序语义：范畴、代数数据类型、单子。
- 形式化证明：代数结构的公理化。

= 推荐学习顺序

1. 集合、映射、等价关系、商集。
2. 群：子群、陪集、正规子群、商群、同态、群作用。
3. 环：理想、商环、整环、PID、UFD、多项式环。
4. 模：子模、商模、自由模、有限生成模、正合列。
5. 域：域扩张、代数元、最小多项式、分裂域、有限域。
6. Galois 理论：Galois 群、基本定理、根式可解。
7. 后续方向：表示论、交换代数、同调代数、代数数论、代数几何。

学习时应该反复追问同一个问题：

- 这个结构的同态是什么？
- 核是什么？
- 商对象是什么？
- 同构定理是什么？

如果能在群、环、模、域扩张中都回答这个问题，代数的主线就基本建立起来了。
