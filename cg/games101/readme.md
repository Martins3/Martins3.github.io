

1. [vedio](https://www.bilibili.com/video/av90798049/)
2. https://sites.cs.ucsb.edu/~lingqi/teaching/games101.html


仿射变换 :  affine map = linear map + translation

homogenous coordinates :

orthogonal matrix : 逆矩阵等于转置，旋转矩阵是就是 orthogonal matrix

view transformation
projectin transformation
- Orthographic projection
- perspective projection 

Rodrigue's Rotation Formula 公式的推倒

MVP : model view projection

规定相机具体的方向 : located at the origin, up at Y, look at -Z

从 perspective 到 orthogonal 的原理 : 无论是近还是远，z 不变，任何点的 x y 被成三角形相似的压缩了

rasterization:

occlusion : 


cuboid

antialiasing : 

ambient + diffuse + specular = Blinn-Phong Reflection

shading frequency
1. flat shading
2. gouraud shading
3. phong shading


Graphics Pipeline:
Vertex Processing
Triangle Processing
Rasterization
Fragment Processing
Framebuffer Operation

Barycentric Coordinate

1. Texture mapping 和 shading 的区别是什么 ?
    - shading 应该处理的是 各种 frequency, 逐个像素，还是逐个


Barycentric Coordinate : 重心坐标实现差值，在三角形中间的一个点表示为三个定点的线性表示。

双线性插值(bilinear)，当 texel 需要映射给比自己大的位置的时候，那么在水平方向和竖直方向都是进行差值。
bicubic : 采用临近的 16 数值

Mipmap : allowing(fast, apporx, square) range query. 事先计算存储。
结合 Mipmap 使用差值，防止出现层数不同而出现的割裂。

Mipmap 会导致 overblur : Anisotropic Filtering 各向异性过滤

Spherical Environment Map:

Texture can affect shading，不仅仅用于呈现颜色。

bump mapping / displacement mapping

3D Texture

implicit representation and explicit representation of geometry(TODO 显示和隐式的含义，隐式是表达式，显示是直接枚举数据 ?)

distance function

subdivision : create more triangle and tune position(为了让其更加光滑，介绍了两种方法 TODO)

collapsing an edge : 简化的一种方法

shadow mapping

Whitted-Style Ray Tracing

KD tree
1. 难以判断三角形和 aabb 存在交集
2. 存在一个物体可能被很多 aabb 存储

BVH : 不在通过空间划分，而是通过物体划分

radiometry(辐射度量学)
radiant enenry : 
radiant flux : 单位时间，相当于功率
radiant intensity: 单位时间，单位立体角
irradiance : power per unit area
radiance : per solid angle, per projected unit area

=>
radiance : irradiance per solid angle
radiance : intensity per projected unit area

// 单位立体角 和 单位面积存在什么不同吗 ?
irradiance : total power received by area dA : 使用 L
radiance : total power received by area dA from direction dw : 使用 E

BRDF: bidirectional reflection distribution function: 描述反射的性质

Monte Carlo Integration:

Path tracking :

Russion Roulette : 按照一定的期望继续发出光线

在光源上采样。

材质等于: BRDF

Advanced Process Modeling :
1. non-surfaced models
2. surface modeling
3. procedural appearance


## TODO
1. 4讲中间，被压缩之后，到底是向内，还是向外
2. montel carlo 积分是怎么推倒的，利用 uniform 的还大概可以理解，其他的概念分布函数，就需要深刻的理解了
3. wittle-style ray tracing 指的是 ?
4. 散射 折射 反射 菲涅尔项 : 都是如何影响 BRDF 的 ?
