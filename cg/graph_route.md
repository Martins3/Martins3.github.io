
https://github.com/stanford-cs248/draw-svg
    - https://news.ycombinator.com/item?id=22640038

https://github.com/tunabrain/tungsten : taichi 中间提到的

https://raytracing.github.io/books/RayTracingInOneWeekend.html

https://github.com/LearnOpenGL-CN/LearnOpenGL-CN : OpenGL 教程

- https://book.douban.com/subject/30426701/ DirectX 12 3D 游戏开发实战



- 基于颜色的绘制
- 基于光照的绘制

光照模型: 全局和局部光照模型

齐次坐标

Mesh Tessellation

https://github.com/ssloy/tinyrenderer : 号称只有 500 行，但是实际上还是首先处理 games101 吧
https://github.com/Dalamar42/rayt : Monte Carlo ray tracer developed using Rust
https://github.com/RayTracing/raytracing.github.io

计算机图形学入门101 : https://zhuanlan.zhihu.com/p/129926618

https://github.com/lettier/3d-game-shaders-for-beginners/blob/master/sections/building-the-demo.md : 太酷了

https://github.com/opengl-tutorials/ogl : opengl教程

https://github.com/ssloy/tinyraycaster : 又是这个人，一周之类做个游戏
https://github.com/ssloy/tinyraytracer : 其实这个人搞了一堆这种有意思的项目


https://gabrielgambetta.com/computer-graphics-from-scratch/ :

# 上课内容


## Mesh
> 我们已经能够快速地进行*光栅化（rasterize） 和三角网格的渲染（ render ）*操作

- Each vertex references one outgoing halfedge, i.e. a halfedge that starts at this vertex.
- Each face references one of the halfedges bounding it.
- Each halfedge provides a handle to
  - the vertex it points to ,
  - the face it belongs to
  - the next halfedge inside the face (ordered counter-clockwise) ,
  - the opposite halfedge ,
  - (optionally: the previous halfedge in the face ).


```c
struct HalfEdge{
  Vertex * vert;
  HalfEdge * pre;
  HalfEdge * next;
  Face * face
}

struct Vertex {
  float vcoord[3]; // ??
  float ncoord[3]; // ???
  HalfEdge * he;
}

struct Face{
 HalfEdge * he;
}

```

OpenMesh 提供操作库。

细分，简化，光滑，特征描述子


Simplication:



二维流形(manifold):

如果局部拓扑处处等价于一个圆盘。
在三角网格流形拓扑中，恰好仅有两个三角形完全共边，每个三角形分别于三个相邻的三角形有一条共边。

带边界的二维流形
  确保边界仅仅属于一个三角形。
> *所以，为什么需要介绍 manifold ?*

简化的方法:
1. sampling
2. decimation
3. vertex-merging

网格细分:
1. 细化阶段: 增加节点
    1. loop 细分
    2. sqrt(3)
2. 平滑阶段:

层次模型简化:
- 几何形状过渡

长方体滤波: 同一个长方体的点被合并，但是高频细节

能量函数:确定简化的顺序

顶点分裂变换 和 收缩变换是一个逆操作

Hoppe :

二次误差:

Laplacian 光滑:

spin image:

3D shape context

## 数字几何处理
