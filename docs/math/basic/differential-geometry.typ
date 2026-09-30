#set heading(numbering: "1.")
#set text(font: "Noto Sans CJK SC", size: 11pt)
#set math.equation(numbering: "(1)")

#let rank = math.op("rank")
#let tr = math.op("tr")
#let det = math.op("det")
#let grad = math.op("grad")
#let div = math.op("div")
#let Hess = math.op("Hess")
#let Ric = math.op("Ric")
#let Scal = math.op("Scal")
#let sec = math.op("sec")
#let Exp = math.op("Exp")
#let inj = math.op("inj")
#let diam = math.op("diam")
#let vol = math.op("vol")
#let Pf = math.op("Pf")
#let id = math.op("id")

#align(center)[#text(size: 20pt, weight: "bold")[微分几何知识大纲]]

本文整理大学数学系需要掌握的微分几何主线。重点不是孤立记忆曲率公式，而是理解：怎样从曲线和曲面的局部计算，逐步抽象到流形、度量、联络和曲率，再用这些局部数据研究空间的整体拓扑。

#text(size: 16pt, weight: "bold")[目录]

#columns(2, gutter: 1.2cm)[
  #set text(size: 9pt)
  #outline(title: none, depth: 3)
]

#pagebreak()

= 总体逻辑

微分几何的主线可以这样组织：

1. 欧氏空间中的曲线
2. 欧氏空间中的曲面
3. 光滑流形和坐标图
4. 切空间、余切空间和张量
5. 微分形式和积分
6. Riemann 度量
7. 联络和平行移动
8. 测地线和指数映射
9. Riemann 曲率
10. 子流形的内蕴与外蕴几何
11. 局部曲率与整体拓扑

其中最核心的抽象是：

- 流形提供可以做微积分的空间。
- 切空间把流形在一点附近线性化。
- Riemann 度量在每个切空间上定义长度和角度。
- 联络比较不同点的切向量，并定义沿曲线的导数。
- 测地线是局部最直的曲线，也是长度泛函的临界点。
- 曲率衡量平行移动、二阶导数和欧氏几何规律失效的程度。
- 整体微分几何研究局部曲率怎样限制距离、体积、基本群和 Euler 示性数。

微分几何中要始终区分三层对象：

- #strong[坐标表达]：$g_(i j)$、$Gamma^k_(i j)$、$R^l_(i j k)$ 等分量。
- #strong[坐标无关对象]：度量 $g$、联络 $nabla$、曲率张量 $R$。
- #strong[几何或拓扑结论]：距离、完备性、曲率符号、Euler 示性数等。

坐标是计算工具，不是几何对象本身。一个正确的公式应该在换坐标后描述同一个对象。

= 预备知识

学习微分几何需要几条基础主线。

多元微积分：

- 偏导数、方向导数和全微分。
- 反函数定理和隐函数定理。
- 多重积分和变量替换。

线性代数：

- 向量空间、对偶空间和线性映射。
- 内积、正交分解和二次型。
- 行列式、特征值和矩阵对角化。
- 多重线性映射和外代数。

拓扑与分析：

- 开集、紧致性、连通性和商空间。
- 一致收敛、常微分方程的存在唯一性。
- 基本群、同调和上同调的初步概念。

依赖关系可以概括为：

$ "多元微积分" -> "局部坐标计算" $
$ "线性代数" -> "切空间、度量和张量" $
$ "拓扑" -> "流形和整体结论" $
$ "常微分方程" -> "测地线和平行移动" $

= 欧氏空间中的曲线

== 正则曲线和参数

一条参数曲线写作：

$ gamma: I -> RR^n, quad t mapsto gamma(t) $

若对所有 $t in I$ 都有

$ gamma'(t) != 0, $

则称 $gamma$ 是正则曲线。$gamma'(t)$ 是速度向量，$|gamma'(t)|$ 是速率。

曲线在区间 $[a,b]$ 上的长度为：

$ L(gamma) = integral_a^b |gamma'(t)| dif t $

长度与保持方向的重新参数化无关。这说明参数 $t$ 只是描述曲线的工具，曲线的几何形状不应依赖具体参数。

弧长参数定义为：

$ s(t) = integral_(t_0)^t |gamma'(u)| dif u $

用弧长 $s$ 作为参数后：

$ |gamma'(s)| = 1 $

单位速曲线使后续曲率公式最简单。

== 曲率

对单位速曲线，单位切向量为：

$ T(s) = gamma'(s) $

因为 $|T|=1$，对 $s$ 求导得到 $T'(s)$ 与 $T(s)$ 正交。曲率定义为：

$ kappa(s) = |T'(s)| $

曲率越大，曲线偏离直线越快。若 $kappa>0$，主法向量为：

$ N(s) = T'(s) / kappa(s) $

对一般参数曲线 $gamma(t) subset RR^3$：

$ kappa(t) = |gamma'(t) times gamma''(t)| / |gamma'(t)|^3 $

平面圆 $gamma(t)=(r cos t,r sin t)$ 的曲率恒为：

$ kappa = 1/r $

所以半径越小，弯曲越强；直线可以看成曲率为 0 的极限情形。

== 挠率和 Frenet 标架

在 $RR^3$ 中定义副法向量：

$ B = T times N $

对单位速且 $kappa>0$ 的曲线，Frenet--Serret 公式是：

$ T' = kappa N $
$ N' = -kappa T + tau B $
$ B' = -tau N $

其中 $tau$ 是挠率。一般参数下：

$ tau = det(gamma', gamma'', gamma''') / |gamma' times gamma''|^2 $

曲率描述曲线如何弯离切线，挠率描述曲线如何离开密切平面。平面曲线的挠率为 0；圆柱螺线通常同时具有非零常曲率和非零常挠率。

== 逻辑关系

- 参数化给出计算曲线的方式。
- 弧长消除参数速度的影响。
- 曲率是切向量的一阶变化，也是位置向量的二阶信息。
- 在三维中，曲率和挠率在刚体运动意义下基本决定曲线的局部形状。

需要掌握：

- 判断曲线是否正则。
- 计算长度并改用弧长参数。
- 计算平面曲线和空间曲线的曲率。
- 理解曲率是几何量，而 $gamma''(t)$ 本身依赖参数。

= 欧氏空间中的曲面

== 参数曲面和切平面

局部参数曲面写作：

$ X: U subset RR^2 -> RR^3, quad (u,v) mapsto X(u,v) $

若

$ X_u times X_v != 0, $

则 $X$ 是正则参数化。点 $p=X(u,v)$ 处的切平面由 $X_u,X_v$ 张成：

$ T_p M = "span"{X_u,X_v} $

选择单位法向量：

$ N = (X_u times X_v) / |X_u times X_v| $

法向量的选择对应曲面的定向。把 $N$ 换成 $-N$ 会改变部分外蕴曲率的符号，但不会改变 Gaussian 曲率。

== 第一基本形式

曲面的第一基本形式是欧氏内积在切平面上的限制：

$ I = E dif u^2 + 2F dif u dif v + G dif v^2 $

其中：

$ E = X_u dot X_u, quad F = X_u dot X_v, quad G = X_v dot X_v $

若曲面上的曲线写成 $alpha(t)=X(u(t),v(t))$，则：

$ |alpha'(t)|^2 = E dot(u)^2 + 2F dot(u)dot(v) + G dot(v)^2 $

因此第一基本形式决定：

- 曲线长度；
- 两个切向量的夹角；
- 曲面区域的面积。

面积元为：

$ dif A = sqrt(E G-F^2) dif u dif v $

== 第二基本形式和形算子

曲面怎样弯曲在 $RR^3$ 中，由法向量的变化描述。形算子定义为：

$ S = -dif N $

第二基本形式定义为：

$ "II"(V,W) = I(S V,W) $

在坐标中：

$ "II" = e dif u^2 + 2f dif u dif v + g dif v^2 $

$ e = X_(u u) dot N, quad f = X_(u v) dot N, quad g = X_(v v) dot N $

形算子 $S$ 的特征值 $kappa_1,kappa_2$ 称为主曲率，对应方向称为主方向。

Gaussian 曲率和平均曲率分别是：

$ K = det S = kappa_1 kappa_2 = (e g-f^2)/(E G-F^2) $

$ H = 1/2 tr S = 1/2(kappa_1+kappa_2) $

改变法向量方向会使 $kappa_1,kappa_2,H$ 变号，但 $K$ 不变。

== 典型曲面

平面：

$ kappa_1=kappa_2=0, quad K=H=0 $

半径为 $r$ 的球面：

$ kappa_1=kappa_2=plus.minus 1/r, quad K=1/r^2 $

圆柱面：

$ kappa_1=plus.minus 1/r, quad kappa_2=0, quad K=0 $

马鞍面 $z=x^2-y^2$ 在原点附近有一正一负两个主曲率，所以 $K<0$。

球面和平面局部几何不同；圆柱虽然看起来弯曲，但 $K=0$，可以把纸张卷成圆柱而不拉伸。这预示 Gaussian 曲率只由曲面内部的长度测量决定。

== Gauss 绝妙定理

第一基本形式看起来只记录长度和角度，第二基本形式才记录曲面在 $RR^3$ 中的弯曲。但 Gauss 的绝妙定理说明：

#block(inset: 8pt, fill: rgb("eef5fb"))[
  Gaussian 曲率 $K$ 可以完全由第一基本形式及其导数计算，因此是内蕴几何量。
]

如果两个曲面局部等距，它们的 Gaussian 曲率必须对应相等。平面和圆柱都满足 $K=0$，所以纸张可以无拉伸地卷成圆柱；球面满足 $K>0$，无法无失真地铺平成平面。

== 逻辑关系

- 第一基本形式描述曲面内部可测的长度、角度和面积。
- 第二基本形式描述曲面相对于环境空间的弯曲。
- 主曲率是形算子的特征值。
- Gaussian 曲率虽然由主曲率定义，却是内蕴量。
- 平均曲率依赖嵌入，是极小曲面理论的核心对象。

= 光滑流形

== 为什么需要流形

参数曲面只处理二维对象，而且经常假设它嵌入 $RR^3$。更一般的空间可能维数更高，也可能没有一个自然的外部欧氏空间。流形把“局部可以使用坐标做微积分”作为基本结构。

一个 $n$ 维拓扑流形 $M$ 满足：

- $M$ 是 Hausdorff 空间；
- $M$ 有可数拓扑基；
- 每个点都有邻域同胚于 $RR^n$ 的开集。

坐标图是二元组 $(U,phi)$：

$ phi: U subset M -> phi(U) subset RR^n $

若两个坐标图重叠，坐标变换为：

$ psi circle phi^(-1): phi(U inter V) -> psi(U inter V) $

当所有坐标变换都是 $C^oo$ 时，就得到光滑流形。

== 典型例子

欧氏空间 $RR^n$ 是最基本的流形。

球面：

$ S^n = {x in RR^(n+1) : |x|=1} $

它不能由一张全局坐标图无奇点覆盖，但可以使用多个局部坐标图。

环面可以写成商空间：

$ T^n = RR^n / ZZ^n $

矩阵群 $"GL"(n,RR)$ 是 $RR^(n^2)$ 中行列式非零的开集，因此也是光滑流形。

若 $M,N$ 分别是 $m,n$ 维流形，则乘积 $M times N$ 是 $m+n$ 维流形。

若光滑映射 $F: RR^m -> RR^k$ 在水平集 $F^(-1)(c)$ 上满足：

$ rank(dif F)=k, $

则正则值定理说明 $F^(-1)(c)$ 是 $m-k$ 维子流形。球面就是 $F(x)=|x|^2$ 的正则水平集。

== 光滑映射和微分同胚

映射 $F:M->N$ 是否光滑，通过局部坐标表达判断：

$ psi circle F circle phi^(-1) $

必须是欧氏空间之间的光滑映射。

若 $F$ 双射、$F$ 和 $F^(-1)$ 都光滑，则称 $F$ 是微分同胚。微分几何通常把微分同胚的流形看作相同的光滑空间。

同胚只保留拓扑结构，微分同胚还保留光滑结构，等距映射则进一步保留 Riemann 度量：

$ "等距" => "微分同胚" => "同胚" $

反向一般不成立。

= 切空间和微分

== 切向量

在曲面中，切向量可以看成曲线上一点的速度。这个定义推广到任意流形：若两条经过 $p$ 的曲线 $gamma_1,gamma_2$ 在某张坐标图中有相同速度，就把它们视为同一个切向量。

点 $p$ 处所有切向量组成 $n$ 维向量空间：

$ T_p M $

在局部坐标 $(x^1,dots,x^n)$ 中，自然基为：

$ partial_1, dots, partial_n $

其中 $partial_i$ 也可以看成对光滑函数求方向导数的算子：

$ partial_i f = (partial f)/(partial x^i) $

因此切向量还有等价定义：它是在 $p$ 点作用于光滑函数、满足 Leibniz 法则的导子。

== 映射的微分

光滑映射 $F:M->N$ 在 $p$ 点诱导线性映射：

$ dif F_p: T_p M -> T_(F(p)) N $

若 $v=gamma'(0)$，则：

$ dif F_p(v) = (F circle gamma)'(0) $

链式法则写成：

$ dif(G circle F)_p = dif G_(F(p)) circle dif F_p $

在坐标中，$dif F_p$ 就是 Jacobian 矩阵。抽象微分是 Jacobian 的坐标无关版本。

== 向量场和 Lie 括号

向量场为每个点光滑地选择一个切向量：

$ X_p in T_p M $

局部写作：

$ X = sum_i X^i partial_i $

向量场作用在函数上：

$ X f = sum_i X^i partial_i f $

两个向量场的 Lie 括号定义为：

$ [X,Y]f = X(Y f)-Y(X f) $

坐标表达为：

$ [X,Y]^k = sum_i (X^i partial_i Y^k - Y^i partial_i X^k) $

$[X,Y]=0$ 表示两个向量场生成的局部流在一阶意义下可交换。Lie 括号也是联络的挠率和 Frobenius 可积性定理中的基本对象。

== 逻辑关系

- 切空间是流形的一阶线性近似。
- 映射的微分在线性近似之间传递切向量。
- 向量场是切空间随点变化形成的光滑截面。
- Lie 括号测量两个无穷小方向的不交换性。

= 余切空间、张量和微分形式

== 余切向量和张量

切空间的对偶空间称为余切空间：

$ T_p^* M = {omega: T_p M -> RR " 为线性映射"} $

坐标基 $partial_i$ 的对偶基写作：

$ dif x^1, dots, dif x^n $

它们满足：

$ dif x^i(partial_j) = delta^i_j $

一个 $(r,s)$ 型张量在每一点都包含 $r$ 个逆变方向和 $s$ 个协变方向；等价地，它属于 $r$ 份切空间与 $s$ 份余切空间的张量积。实际计算中常见的对象包括：

- 函数：$(0,0)$ 型张量；
- 向量场：$(1,0)$ 型张量；
- 1-形式：$(0,1)$ 型张量；
- Riemann 度量：对称 $(0,2)$ 型张量；
- 曲率张量：可写成 $(1,3)$ 或 $(0,4)$ 型张量。

== 微分形式

$k$-形式是完全反对称的 $(0,k)$ 型张量。局部可以写成：

$ omega = sum_(i_1<dots<i_k) omega_(i_1 dots i_k) dif x^(i_1) ∧ dots ∧ dif x^(i_k) $

外积满足：

$ alpha ∧ beta = (-1)^(k l) beta ∧ alpha $

其中 $alpha$ 是 $k$-形式，$beta$ 是 $l$-形式。特别地，1-形式满足：

$ alpha ∧ alpha = 0 $

== 外微分

外微分把 $k$-形式变成 $k+1$-形式：

$ dif: Omega^k(M) -> Omega^(k+1)(M) $

对函数 $f$：

$ dif f = sum_i partial_i f dif x^i $

它满足：

$ dif(alpha ∧ beta) = dif alpha ∧ beta + (-1)^k alpha ∧ dif beta $

$ dif circle dif = 0 $

满足 $dif omega=0$ 的形式称为闭形式；能写成 $omega=dif eta$ 的形式称为恰当形式。每个恰当形式都是闭形式，但反向是否成立取决于空间的整体拓扑。

== 拉回和 Stokes 定理

光滑映射 $F:M->N$ 可以把 $N$ 上的微分形式拉回到 $M$：

$ F^*: Omega^k(N) -> Omega^k(M) $

拉回与外微分可交换：

$ dif(F^* omega)=F^*(dif omega) $

Stokes 定理统一了微积分中的多个积分定理：

$ integral_M dif omega = integral_(partial M) omega $

它包含微积分基本定理、Green 公式、Gauss 散度定理和经典 Stokes 公式。

== de Rham 上同调

闭形式模去恰当形式得到 de Rham 上同调：

$ H_"dR"^k(M) = ker(dif_k) / "im"(dif_(k-1)) $

de Rham 定理说明，微分形式得到的上同调与拓扑定义的实系数上同调一致。这是“用微积分探测拓扑”的核心桥梁。

== 逻辑关系

- 余切向量作用在切向量上。
- 张量把线性代数对象逐点放到流形上。
- 微分形式适合积分，因为反对称性自动编码定向体积。
- $dif^2=0$ 产生上同调，局部微分方程由此连接到整体拓扑。

= Riemann 度量

== 度量的定义

Riemann 度量 $g$ 在每个切空间 $T_p M$ 上给出正定内积 $g_p$，并随 $p$ 光滑变化。

局部坐标中：

$ g = sum_(i,j) g_(i j) dif x^i dif x^j $

矩阵 $(g_(i j))$ 对称正定。对切向量

$ v=sum_i v^i partial_i $

有：

$ |v|_g^2 = g(v,v)=sum_(i,j) g_(i j)v^i v^j $

Riemann 流形是二元组 $(M,g)$。同一个光滑流形可以承载许多不同度量，因此“流形的拓扑”和“所选度量的几何”必须分开。

== 长度、能量和距离

分段光滑曲线 $gamma:[a,b]->M$ 的长度：

$ L(gamma)=integral_a^b sqrt(g_(gamma(t))(gamma'(t),gamma'(t))) dif t $

能量：

$ E(gamma)=1/2 integral_a^b g(gamma',gamma') dif t $

两点间的 Riemann 距离定义为：

$ d(p,q)=inf_{gamma:p->q} L(gamma) $

长度与重新参数化无关，能量一般依赖参数。固定端点和时间区间时，常速最短曲线同时最小化能量。

== 体积元

若 $g=(g_(i j))$，则 Riemann 体积元为：

$ dif vol_g = sqrt(det(g_(i j))) dif x^1 dots dif x^n $

积分写成：

$ integral_M f dif vol_g $

这里的行列式因子正是坐标变化时补偿 Jacobian 的部分，使积分不依赖所选坐标。

== 梯度、散度和 Laplace--Beltrami 算子

梯度由下式定义：

$ g(grad f,X)=dif f(X) $

局部坐标中：

$ (grad f)^i=sum_j g^(i j) partial_j f $

散度为：

$ div X = 1/sqrt(det g) partial_i(sqrt(det g) X^i) $

Laplace--Beltrami 算子为：

$ Delta f = div(grad f) $

符号约定不同的书可能使用 $-Delta$。它是欧氏 Laplace 算子在 Riemann 流形上的自然推广。

== 等距和共形

若微分同胚 $F:(M,g)->(N,h)$ 满足：

$ F^*h=g, $

则 $F$ 是等距映射，它保持长度、角度、距离和曲率。

若：

$ F^*h=e^(2u)g, $

则 $F$ 是共形映射，它保持角度，但通常不保持长度和曲率。

== 逻辑关系

- 光滑结构允许求导，Riemann 度量允许测量。
- 度量诱导长度、距离、体积和微分算子。
- 度量把切空间与余切空间对应起来，因此可以把 $dif f$ 转成 $grad f$。
- 等距是 Riemann 几何中真正保持全部度量信息的映射。

= 联络和协变导数

== 为什么普通导数不够

在 $RR^n$ 中，不同点的切空间可以自然视为同一个向量空间，所以可以直接比较向量。一般流形上，$T_p M$ 和 $T_q M$ 是不同空间，表达式

$ X(q)-X(p) $

没有天然意义。联络规定怎样沿方向求向量场的导数。

仿射联络写作：

$ nabla_X Y $

它对 $X$ 是函数线性的，对 $Y$ 满足 Leibniz 法则：

$ nabla_X(f Y)=X(f)Y+f nabla_X Y $

== Christoffel 符号

在局部坐标中，联络由下式确定：

$ nabla_(partial_i) partial_j = sum_k Gamma^k_(i j) partial_k $

对

$ X=X^i partial_i, quad Y=Y^j partial_j $

有：

$ (nabla_X Y)^k=X^i partial_i Y^k+Gamma^k_(i j)X^i Y^j $

Christoffel 符号本身不是张量，因为换坐标时会出现坐标变换的二阶导数。$nabla_X Y$ 整体却是坐标无关的向量场。

== Levi--Civita 联络

Riemann 度量唯一确定一个同时满足下列条件的联络：

无挠：

$ nabla_X Y-nabla_Y X=[X,Y] $

度量相容：

$ X(g(Y,Z))=g(nabla_X Y,Z)+g(Y,nabla_X Z) $

这个唯一联络称为 Levi--Civita 联络。它的坐标公式为：

$ Gamma^k_(i j)=1/2 g^(k l)(partial_i g_(j l)+partial_j g_(i l)-partial_l g_(i j)) $

其中 $(g^(i j))$ 是 $(g_(i j))$ 的逆矩阵，并使用重复指标求和约定。

== 沿曲线的协变导数

设 $V(t)$ 是沿曲线 $gamma(t)$ 的向量场。协变导数写作：

$ (D V)/(dif t)=nabla_(gamma') V $

坐标表达为：

$ ((D V)/(dif t))^k = (dif V^k)/(dif t)+Gamma^k_(i j) gamma'^i V^j $

若：

$ (D V)/(dif t)=0, $

则称 $V$ 沿 $gamma$ 平行。给定初值 $V(a)$，平行移动方程是一阶线性常微分方程，因此局部存在唯一解。

度量相容性保证平行移动保持向量的长度和夹角。

== 逻辑关系

- 联络使不同点的切空间可以沿曲线比较。
- Christoffel 符号是联络在坐标中的系数，不是曲率。
- Levi--Civita 联络由度量唯一确定。
- 平行移动、测地线、Hessian 和曲率都依赖联络。

= 测地线和指数映射

== 测地线方程

测地线定义为切向量沿自身平行的曲线：

$ (D gamma')/(dif t)=0 $

坐标形式是二阶常微分方程：

$ gamma''^k+Gamma^k_(i j)gamma'^i gamma'^j=0 $

给定初始位置和初始速度：

$ gamma(0)=p, quad gamma'(0)=v $

测地线在局部唯一存在。

欧氏空间中的测地线是直线。球面上的测地线是大圆，而不是任意纬线。

== 变分观点

测地线是能量泛函的临界点。对固定端点的曲线变分 $gamma_s$，第一变分公式给出：

$ (dif)/(dif s)|_(s=0) E(gamma_s) = -integral_a^b g(V,(D gamma')/(dif t)) dif t $

因此所有固定端点变分的一阶变化都为零，当且仅当：

$ (D gamma')/(dif t)=0 $

测地线只保证局部“直”，不一定在任意长区间上都是全局最短路。球面上大圆弧超过半圆后便不再最短。

== 指数映射

对 $v in T_p M$，令 $gamma_v$ 是满足 $gamma_v(0)=p$、$gamma_v'(0)=v$ 的测地线。指数映射定义为：

$ Exp_p(v)=gamma_v(1) $

在 $0 in T_p M$ 附近，$Exp_p$ 是局部微分同胚，由此得到法坐标。

在法坐标中心 $p$：

$ g_(i j)(p)=delta_(i j), quad Gamma^k_(i j)(p)=0 $

但 Christoffel 符号在一点消失不代表曲率消失，因为曲率还包含 $Gamma$ 的一阶导数。法坐标只能消去一阶的坐标效应，不能消去真正的二阶几何信息。

== 完备性和 Hopf--Rinow 定理

Riemann 流形测地完备，是指每条测地线可以对所有 $t in RR$ 延伸。对连通 Riemann 流形，Hopf--Rinow 定理把下列性质联系起来：

- 作为度量空间 $(M,d)$ 是完备的；
- 流形测地完备；
- 每个闭有界集合紧致；
- 任意两点之间存在一条最短测地线。

紧致且无边界的 Riemann 流形自动完备。开单位球带欧氏度量则不完备，因为测地线可以在有限时间撞到缺失的边界。

== 割点和共轭点

从 $p$ 出发的测地线在某段时间内最短，但可能在某点后失去最短性。最远仍保持最短的边界形成割点现象。

Jacobi 场描述测地线族的一阶变化，满足：

$ (D^2 J)/(dif t^2)+R(J,gamma')gamma'=0 $

若非零 Jacobi 场在一条测地线的两个点消失，这两个点互为共轭点。曲率通过 Jacobi 方程控制邻近测地线是聚拢还是分散。

== 逻辑关系

- 测地线由 Levi--Civita 联络定义。
- 变分法说明测地线为什么与最短路有关。
- 指数映射把切空间中的直线方向投到流形。
- 完备性保证测地线不会在有限时间无故终止。
- 割点和共轭点解释局部最短性怎样失效。

= 曲率张量

== Riemann 曲率

曲率张量定义为：

$ R(X,Y)Z=nabla_X nabla_Y Z-nabla_Y nabla_X Z-nabla_([X,Y])Z $

它测量二阶协变导数不交换的程度，也测量向量沿无穷小闭合回路平行移动后不能回到原方向的程度。

坐标分量可以写成：

$ R^l_(i j k)=partial_i Gamma^l_(j k)-partial_j Gamma^l_(i k)+Gamma^m_(j k)Gamma^l_(i m)-Gamma^m_(i k)Gamma^l_(j m) $

不同教材可能交换指标次序或给 $R$ 整体加负号。使用曲率公式时必须先确认约定。

把一个指标用度量降下来：

$ R(X,Y,Z,W)=g(R(X,Y)Z,W) $

它满足若干对称性：

$ R(X,Y,Z,W)=-R(Y,X,Z,W) $

$ R(X,Y,Z,W)=-R(X,Y,W,Z) $

$ R(X,Y,Z,W)=R(Z,W,X,Y) $

第一 Bianchi 恒等式：

$ R(X,Y)Z+R(Y,Z)X+R(Z,X)Y=0 $

== 截面曲率

给定线性无关切向量 $u,v$，它们张成二维平面 $sigma$。截面曲率定义为：

$ sec(sigma) = g(R(u,v)v,u)/(g(u,u)g(v,v)-g(u,v)^2) $

若 $u,v$ 正交归一，则：

$ sec(sigma)=g(R(u,v)v,u) $

截面曲率是二维 Gaussian 曲率在高维中的推广。一个点的所有截面曲率可以恢复完整 Riemann 曲率张量。

== Ricci 曲率和标量曲率

取正交归一基 $e_1,dots,e_n$。Ricci 曲率是曲率张量的迹：

$ Ric(v,v)=sum_i g(R(e_i,v)v,e_i) $

当 $|v|=1$ 时，它是所有包含 $v$ 的正交截面曲率之和。

标量曲率再取一次迹：

$ Scal=sum_i Ric(e_i,e_i)=2sum_(i<j) sec(e_i,e_j) $

在二维中：

$ Scal=2K $

信息量逐级减少：

$ "Riemann 曲率" -> "截面曲率" -> "Ricci 曲率" -> "标量曲率" $

严格地说，全部截面曲率仍能决定 Riemann 曲率；Ricci 和标量曲率则是压缩后的平均信息。

== 常曲率空间

若所有点、所有二维切平面的截面曲率都等于常数 $c$，则：

$ R(X,Y)Z=c(g(Y,Z)X-g(X,Z)Y) $

并且：

$ Ric=(n-1)c g, quad Scal=n(n-1)c $

三个基本模型：

- 欧氏空间 $RR^n$：$c=0$；
- 半径 $r$ 的球面 $S^n(r)$：$c=1/r^2$；
- 双曲空间 $HH^n(r)$：$c=-1/r^2$。

== 曲率正性的强弱

在维数至少 3 时：

$ sec>0 => Ric>0 => Scal>0 $

反向一般不成立。正截面曲率要求每个二维方向都为正；正 Ricci 曲率只要求围绕每个向量的截面曲率之和为正；正标量曲率只要求所有截面曲率的总和为正。

因此，把一个关于正截面曲率的猜想改成正 Ricci 曲率或正标量曲率，通常会变成完全不同的问题。

== Jacobi 场的曲率直觉

设 $gamma$ 是测地线，$J$ 与 $gamma'$ 正交。Jacobi 方程近似为：

$ J''+sec(gamma',J)|gamma'|^2J=0 $

- 正截面曲率使邻近测地线倾向于重新聚拢，类似球面经线。
- 零曲率使测地线保持线性分离，类似欧氏平面直线。
- 负截面曲率使邻近测地线倾向于指数分离，类似双曲平面。

这是比较几何背后的基本动力学图像。

= 一个完整的坐标计算流程

给定局部坐标中的度量：

$ g=g_(i j)dif x^i dif x^j $

通常按以下顺序计算：

1. 写出度量矩阵 $(g_(i j))$。
2. 求逆矩阵 $(g^(i j))$ 和 $det g$。
3. 用度量的一阶导数计算 $Gamma^k_(i j)$。
4. 用 $Gamma$ 的一阶导数和二次项计算 $R^l_(i j k)$。
5. 缩并得到 $Ric_(i j)$ 和 $Scal$。
6. 把 $Gamma$ 代入测地线方程。

== 欧氏平面的极坐标

极坐标度量为：

$ g=dif r^2+r^2 dif theta^2 $

非零 Christoffel 符号包括：

$ Gamma^r_(theta theta)=-r $

$ Gamma^theta_(r theta)=Gamma^theta_(theta r)=1/r $

虽然 $Gamma$ 不为零，Riemann 曲率仍为零。这说明 Christoffel 符号可以仅由弯曲的坐标系产生，不代表空间本身弯曲。

== 球面坐标

半径 $r$ 的二维球面度量为：

$ g=r^2(dif theta^2+sin^2 theta dif phi^2) $

计算得到：

$ K=1/r^2 $

这个曲率无法通过换坐标消除。可以在一点选择法坐标使 $Gamma(p)=0$，但曲率张量在该点仍然非零。

== 计算时的常见检查

- $(g_(i j))$ 应对称正定。
- Levi--Civita 联络满足 $Gamma^k_(i j)=Gamma^k_(j i)$。
- 曲率分量应满足反对称性和 Bianchi 恒等式。
- 欧氏空间换成任意坐标后，最终曲率仍应为 0。
- 二维结果应满足 $Scal=2K$。
- 常曲率模型应满足 $Ric=(n-1)c g$。

= 子流形几何

== 切空间和法空间分解

设 $M^m$ 等距嵌入 Riemann 流形 $bar(M)^n$。沿 $M$ 有正交分解：

$ T_p bar(M)=T_p M plus T_p M^perp $

环境空间的 Levi--Civita 联络可以分成切向和法向部分。Gauss 公式为：

$ bar(nabla)_X Y=nabla_X Y+"II"(X,Y) $

其中 $nabla_X Y$ 是切向部分，$"II"(X,Y)$ 是第二基本形式，取值于法空间。

== 平均曲率和极小子流形

取 $T_p M$ 的正交归一基 $e_1,dots,e_m$，平均曲率向量定义为：

$ bold(H)=1/m sum_i "II"(e_i,e_i) $

若：

$ bold(H)=0, $

则称 $M$ 是极小子流形。这里“极小”首先表示面积泛函的一阶变分为零，不保证它在所有变分下真的是局部最小值。

对 $RR^3$ 中的定向曲面，平均曲率向量与前面主曲率平均值 $H=(kappa_1+kappa_2)/2$ 对应。

== Gauss 方程

子流形的内蕴曲率、环境曲率和第二基本形式满足：

$ g(R(X,Y)Z,W)=bar(g)(bar(R)(X,Y)Z,W)
  +bar(g)("II"(X,W),"II"(Y,Z))
  -bar(g)("II"(X,Z),"II"(Y,W)) $

对 $RR^3$ 中的曲面，环境曲率为零，Gauss 方程化为：

$ K=kappa_1kappa_2 $

这再次说明曲面的内蕴 Gaussian 曲率由外蕴弯曲的两个主方向共同决定。

== Gauss--Codazzi 方程

并不是任意第一、第二基本形式都来自某个实际曲面。Gauss 方程给出曲率兼容条件，Codazzi 方程给出第二基本形式导数的兼容条件。

曲面的基本定理大意是：在单连通区域上，若第一、第二基本形式满足 Gauss--Codazzi 方程，就能在刚体运动意义下唯一重建局部曲面。

== 逻辑关系

- 第二基本形式比较子流形联络与环境联络。
- 平均曲率控制体积的一阶变分。
- Gauss 方程连接内蕴曲率和外蕴弯曲。
- 极小曲面、平均曲率流和广义相对论中的嵌入问题都建立在这些对象上。

= 整体微分几何

局部公式描述一点附近的几何，整体定理则把曲率假设转化为整个空间的拓扑和度量结论。

== Gauss--Bonnet 定理

对闭的定向曲面 $M$：

$ integral_M K dif A=2pi chi(M) $

左边由 Riemann 度量和 Gaussian 曲率计算，右边是拓扑不变量 Euler 示性数。

例如：

$ chi(S^2)=2 => integral_(S^2)K dif A=4pi $

$ chi(T^2)=0 => integral_(T^2)K dif A=0 $

因此环面不可能处处具有正 Gaussian 曲率，也不可能处处具有负 Gaussian 曲率。

有边界的曲面还要加入边界的测地曲率项；带角点时再加入外角项。它们共同保证总曲率只依赖拓扑。

== Chern--Gauss--Bonnet 定理

对闭定向偶维流形 $M^(2m)$：

$ chi(M)=1/(2pi)^m integral_M Pf(Omega) $

其中 $Omega$ 是曲率形式矩阵，$Pf$ 是 Pfaffian。它把二维 Gauss--Bonnet 推广到任意偶数维。

高维 Euler 形式是许多曲率分量乘积的带符号组合。即使所有截面曲率都为正，也不能直接看出 $Pf(Omega)$ 逐点为正；这正是 Hopf 示性数猜想困难的来源。

== Bonnet--Myers 定理

若完整 $n$ 维 Riemann 流形满足：

$ Ric >= (n-1)k g, quad k>0 $

则：

$ diam(M)<=pi/sqrt(k) $

并且 $M$ 紧致、基本群有限。

正 Ricci 曲率通过 Jacobi 场和共轭点控制测地线，使空间不能无限延伸。

== Cartan--Hadamard 定理

若完整、单连通 Riemann 流形满足：

$ sec<=0, $

则对任意 $p in M$：

$ Exp_p: T_p M -> M $

是全局微分同胚。因此 $M$ 微分同胚于 $RR^n$，任意两点之间有唯一测地线。

负或非正曲率使测地线发散，从而减少共轭点和多条最短路造成的复杂性。

== Synge 定理

对正截面曲率的紧致流形：

- 若维数为偶数且流形可定向，则它单连通。
- 若维数为奇数，则它可定向。

这个定理展示了曲率、维数、定向性和基本群之间的直接联系。Hopf 猜想在四维成立，也可以结合 Synge 定理和 Poincaré 对偶来理解。

== 比较几何

比较几何把一般流形与常曲率模型比较。

Rauch 比较定理：比较 Jacobi 场和测地线发散速度。

Toponogov 比较定理：在截面曲率下界条件下，把测地三角形与常曲率模型三角形比较。

Bishop--Gromov 比较定理：在 Ricci 曲率下界条件下，控制测地球体积增长。

逻辑关系：

- 截面曲率直接控制二维测地变分和三角形。
- Ricci 曲率控制许多方向的平均聚焦和体积增长。
- 曲率下界常导出直径、体积、基本群和拓扑型的限制。

= 特征类和曲率

向量丛可能在局部看起来像乘积，却在整体上发生扭曲。特征类用上同调类记录这种整体扭曲。

常见特征类包括：

- Euler 类；
- Chern 类；
- Pontryagin 类；
- Stiefel--Whitney 类。

Chern--Weil 理论从联络曲率构造闭微分形式，并证明其 de Rham 上同调类与联络选择无关。例如 Euler 类的积分给出 Euler 示性数。

这条逻辑链非常重要：

$ "联络" -> "曲率" -> "闭微分形式" -> "上同调类" -> "拓扑不变量" $

局部改变度量和联络会改变曲率形式的具体表达，但不会改变最终的特征类。

= 与 Hopf 猜想的接口

有了前面的语言，可以准确表述 Hopf 示性数猜想：

#block(inset: 8pt, fill: rgb("fff0f0"))[
  若闭的偶维 Riemann 流形 $M^(2m)$ 满足 $sec>0$，是否一定有 $chi(M)>0$？
]

这句话连接了三个层次：

- $sec>0$ 是每一点、每个二维切平面上的局部几何条件。
- $chi(M)$ 是由同调群决定的整体拓扑不变量。
- Chern--Gauss--Bonnet 提供曲率积分公式，但没有自动给出高维 Euler 形式的符号。

$S^2 times S^2$ 问题是另一项常被称为 Hopf 猜想的问题。标准乘积度量满足 $sec>=0$，但由两个不同球面因子方向张成的混合平面曲率为 0。问题是能否选择另一种度量，使所有截面曲率同时严格为正。

理解这两个问题至少需要以下依赖链：

$ "切空间" -> "Riemann 度量" -> "Levi--Civita 联络" -> "截面曲率" $

$ "微分形式" -> "de Rham 上同调" -> "Euler 类" -> "Euler 示性数" $

前一条是几何线，后一条是拓扑线。Hopf 猜想困难，正因为它要求在一般高维流形上把两条线精确连接起来。

= 常见混淆

曲率与弯曲外观：

- 圆柱在 $RR^3$ 中看起来弯曲，但内蕴 Gaussian 曲率为 0。
- Gaussian 曲率是曲面内部可以测量的量，平均曲率依赖嵌入。

Christoffel 符号与曲率：

- $Gamma^k_(i j)$ 可以因坐标选择而非零。
- 曲率张量是否为零是坐标无关的事实。
- 可以在一点令 $Gamma=0$，但一般不能同时令曲率为 0。

测地线与最短路：

- 测地线是长度或能量泛函的局部临界曲线。
- 短测地线段局部最短，长测地线段可能越过割点后失去最短性。

内蕴与外蕴：

- Riemann 度量、Levi--Civita 联络、测地线和 Riemann 曲率是内蕴对象。
- 第二基本形式、形算子和平均曲率需要给定嵌入。
- Gaussian 曲率可以由外蕴主曲率计算，但最终是内蕴量。

不同曲率条件：

- 正截面曲率最强，要求每个二维方向都为正。
- 正 Ricci 曲率是方向平均条件。
- 正标量曲率是更粗的总平均条件。
- 从弱曲率条件得到的结论不能自动套到强曲率猜想上，反之亦然。

= 更高阶方向

完成基础内容后，可以继续深入：

- 比较几何和刚性定理。
- 极小曲面与几何测度论。
- Ricci flow 和平均曲率流。
- Kähler 几何与复微分几何。
- 辛几何和 Hamilton 系统。
- Lie 群与齐性空间。
- 几何分析和非线性偏微分方程。
- 指标定理和全局分析。
- 广义相对论中的 Lorentz 几何。
- 几何拓扑和规范场论。

依赖关系大致为：

- Riemann 曲率 $->$ 比较几何、Ricci flow、Einstein 度量。
- 第二基本形式 $->$ 极小曲面、平均曲率流。
- 微分形式和上同调 $->$ 辛几何、特征类、指标定理。
- 复流形加 Hermitian 度量 $->$ Kähler 几何。
- 流形上的变分法和 PDE $->$ 几何分析。

= 推荐学习顺序

1. 先学欧氏空间曲线和曲面的第一、第二基本形式。
2. 接着学光滑流形、切空间、余切空间和光滑映射。
3. 再学张量、微分形式、外微分和 Stokes 定理。
4. 然后学 Riemann 度量、Levi--Civita 联络和平行移动。
5. 接着学测地线、指数映射、完备性和 Jacobi 场。
6. 再系统学习 Riemann、截面、Ricci 和标量曲率。
7. 最后学习 Gauss--Bonnet、比较定理、特征类和几何分析。

最小闭环：

1. 流形和坐标图
2. 切空间
3. Riemann 度量
4. Levi--Civita 联络
5. 测地线
6. 曲率张量
7. 截面曲率
8. Gauss--Bonnet 定理

如果这个闭环清楚，就能看懂大多数 Riemann 几何基础材料，也能理解 Hopf 猜想为什么是“局部曲率控制整体拓扑”的典型难题。
