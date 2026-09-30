# C++ vtable 与 RTTI 布局

两个工具的作用是互补的：

| 命令                                           | 输出                                          | 说明                                       |
| ---------------------------------------------- | --------------------------------------------- | ------------------------------------------ |
| `clang -cc1 -fdump-record-layouts actress.cpp` | 类成员/基类在对象里的**字节偏移**             | 只看到「对象长什么样」，看不到 vtable 内容 |
| `g++ -fdump-lang-class -c actress.cpp`         | **vtable / VTT 的内容**、typeinfo 符号、thunk | 生成 `actress.cpp.001l.class`              |


> `clang -cc1` 直接进 clang 前端、不走
> driver，所以不会链接；actress.cpp 刻意不含头文件， 于是两种 dump
> 里都只有我们自己定义的类，没有标准库噪音。

下面按 Itanium ABI（Linux x86-64 / ARM64 都遵守）逐一拆解。这是 Itanium C++ ABI
的 vtable 布局约定，不是 C++ 标准的一部分，但主流 Unix 编译器都这样实现。

## 0. 一个对象的最小形态

```cpp
struct Base {
    virtual ~Base() {}
    virtual void f() { printf("Base::f\n"); }
    virtual void g() { printf("Base::g\n"); }
    int b = 1;
};
```

clang 的 record-layout 输出：

```
*** Dumping AST Record Layout
         0 | struct Base
         0 |   (Base vtable pointer)
         8 |   int b
           | [sizeof=16, dsize=12, align=8,
           |  nvsize=12, nvalign=8]
```

要点：

- 编译器偷偷在对象**第一个槽位**塞了一个 `vptr`（8 字节），指向这个类的 vtable。
- 成员 `b` 从偏移 8 开始，所以 `sizeof(Base)=16`；`dsize=12`
  表示真正有数据的部分是 12 字节。
- 谁先出现、谁在最前面，取决于基类/成员的声明顺序，和 `virtual`
  无关——只要类有虚函数，vptr 就占第一个槽位。

g++ 的 class-hierarchy 输出给出了 vtable 的内容：

```
Vtable for Base
Base::_ZTV4Base: 6 entries
0     (int (*)(...))0
8     (int (*)(...))(& _ZTI4Base)
16    (int (*)(...))Base::~Base
24    (int (*)(...))Base::~Base
32    (int (*)(...))Base::f
40    (int (*)(...))Base::g

Class Base
   size=16 align=8
   base size=12 base align=8
Base (0x0x...) 0
    vptr=((& Base::_ZTV4Base) + 16)
```

关键结论：`vptr` 并不指向 vtable
的**开头**，而是指向「第一个虚函数」的位置（`_ZTV4Base + 16`）。 vtable
开头两个槽位是管理信息：

```
vptr[-2]  offset-to-top    当前子对象回到完整对象起点的字节偏移
vptr[-1]  typeinfo 指针     RTTI（_ZTI4Base）
vptr[0]   第一个虚函数
...
```

[actress.cpp](actress.cpp) 里的 `dump_vtable()` 就是按这个约定手动读出
`vptr[-2]` 和 `vptr[-1]`， 所以不需要 `<typeinfo>` 头文件也能看到 RTTI 指针。

## 1. 单继承：派生类复用基类的 vptr

```
*** Dumping AST Record Layout
         0 | struct Derived
         0 |   struct Base (primary base)
         0 |     (Base vtable pointer)
         8 |     int b
        12 |   int d
           | [sizeof=16, dsize=16, align=8,
           |  nvsize=16, nvalign=8]
```

`Base` 被标注为 **primary base**：`Derived` 和 `Base` 共享同一个 vptr（偏移 0
处）， 不额外分配新的 vptr。`Derived` 新增的成员 `d` 放在偏移 12。

```
Vtable for Derived
Derived::_ZTV7Derived: 6 entries
0     (int (*)(...))0
8     (int (*)(...))(& _ZTI7Derived)
16    (int (*)(...))Derived::~Derived
24    (int (*)(...))Derived::~Derived
32    (int (*)(...))Derived::f
40    (int (*)(...))Base::g
```

对比 Base 的 vtable：`f` 的位置被换成了 `Derived::f`，而未重写的 `g` 仍然是
`Base::g`。
这就是「虚调用」的全部秘密——编译器只按**槽位**调用，槽位里放谁就是谁。 `Derived`
的 typeinfo 也从 `_ZTI4Base` 换成了 `_ZTI7Derived`。

运行输出：

```
== 单继承: Derived : Base ==
&d = 0x..., (Base*)&d = 0x... (地址相同, offset-to-top = 0)
Derived::f
(Base*)&Derived    vptr=0x...  offset-to-top=0  typeinfo=0x...
```

## 2. 多继承：this 调整与 thunk

```cpp
struct Multi : Left, Right {
    void l() override { ... }
    void r() override { ... }
    int m = 5;
};
```

```
*** Dumping AST Record Layout
         0 | struct Multi
         0 |   struct Left (primary base)
         0 |     (Left vtable pointer)
         8 |     int lv
        16 |   struct Right (base)
        16 |     (Right vtable pointer)
        24 |     int rv
        28 |   int m
           | [sizeof=32, dsize=32, align=8,
           |  nvsize=32, nvalign=8]
```

只有第一个基类 `Left` 能当 primary base（复用偏移 0 的 vptr）；第二个基类
`Right` 只能放在 偏移 16，并且**自己带一个 vptr**。所以 `Multi` 对象里有两个
vptr，但都指向同一个 `_ZTV5Multi` 的不同位置：

```
Vtable for Multi
Multi::_ZTV5Multi: 11 entries
0     (int (*)(...))0
8     (int (*)(...))(& _ZTI5Multi)
16    (int (*)(...))Multi::~Multi
24    (int (*)(...))Multi::~Multi
32    (int (*)(...))Multi::l
40    (int (*)(...))Multi::r
48    (int (*)(...))-16
56    (int (*)(...))(& _ZTI5Multi)
64    (int (*)(...))Multi::_ZThn16_N5MultiD1Ev
72    (int (*)(...))Multi::_ZThn16_N5MultiD0Ev
80    (int (*)(...))Multi::_ZThn16_N5Multi1rEv

Class Multi
   size=32 align=8
   base size=32 base align=8
Multi (0x0x...) 0
    vptr=((& Multi::_ZTV5Multi) + 16)
Left (0x0x...) 0
      primary-for Multi (0x0x...)
Right (0x0x...) 16
      vptr=((& Multi::_ZTV5Multi) + 64)
```

- `Left` 子对象（偏移 0）的 vptr 指向 `_ZTV5Multi + 16`，这一段的
  `offset-to-top = 0`。
- `Right` 子对象（偏移 16）的 vptr 指向 `_ZTV5Multi + 64`，这一段的
  `offset-to-top = -16`， typeinfo 仍然是 `_ZTI5Multi`。
- 注意 `_ZThn16_...` 这类符号：`Th` = thunk，`n16` = 把 `this` 减去 16。因为通过
  `Right*` 调用虚函数时，`this` 指向的是 `Right` 子对象（偏移 16），而
  `Multi::r` 期望的 `this` 是完整对象起点，所以 thunk 先 `this -= 16`
  再跳进真正的函数。

运行输出里 `(Right*)&m` 比 `&m` 大 16，且两个指针的 `typeinfo`
相同、`offset-to-top` 不同：

```
== 多继承: Multi : Left, Right ==
&m = 0x..., (Left*)&m = 0x... (相同)
(Right*)&m = 0x... (this 被 +16, 指向第二个 vtable)
Multi::r
(Left*)&Multi      vptr=0x...  offset-to-top=0   typeinfo=0x...
(Right*)&Multi     vptr=0x...  offset-to-top=-16 typeinfo=0x...
```

这正解释了为什么「把 `Multi*` 转成 `Right*`」不是无操作——指针值真的要变。

## 3. 菱形虚继承：vbase offset、VTT 与 construction vtable

```cpp
struct VBase   { virtual ~VBase() {} virtual void v() {...} int vb = 6; };
struct VLeft   : virtual VBase { void v() override {...} int vl = 7; };
struct VRight  : virtual VBase { int vr = 8; };
struct VMost   : VLeft, VRight { int vm = 9; };
```

```
*** Dumping AST Record Layout
         0 | struct VMost
         0 |   struct VLeft (primary base)
         0 |     (VLeft vtable pointer)
         8 |     int vl
        16 |   struct VRight (base)
        16 |     (VRight vtable pointer)
        24 |     int vr
        28 |   int vm
        32 |   struct VBase (virtual base)
        32 |     (VBase vtable pointer)
        40 |     int vb
           | [sizeof=48, dsize=44, align=8,
           |  nvsize=32, nvalign=8]
```

虚基类 `VBase` 不再按声明位置摆放，而是被挤到整个对象**最后**（偏移
32），且只存在一份。 这带来两个经典问题：

1. 通过 `VBase*`（或 `VLeft*`）调用虚函数时，编译器**编译期不知道** `VBase`
   子对象在哪， 必须在 vtable 里存一份「vbase offset」供运行时查找。
2. 构造 `VMost` 的过程中，`VBase` 还没构造好，此时 vtable
   里不能出现指向「尚未构造部分」的虚函数， 因此需要一套**构造期专用**的
   vtable。

对应地，g++ 为虚继承生成了三类额外东西：

### 3.1 vtable 里的 vbase offset / vcall offset

`VLeft` 单独的 vtable（13 个条目）：

```
Vtable for VLeft
VLeft::_ZTV5VLeft: 13 entries
0     16
8     (int (*)(...))0
16    (int (*)(...))(& _ZTI5VLeft)
24    (int (*)(...))VLeft::v
32    (int (*)(...))VLeft::~VLeft
40    (int (*)(...))VLeft::~VLeft
48    18446744073709551600      // = -16, vbase offset
56    18446744073709551600
64    (int (*)(...))-16         // vcall offset
72    (int (*)(...))(& _ZTI5VLeft)
80    (int (*)(...))VLeft::_ZTv0_n24_N5VLeftD1Ev
88    (int (*)(...))VLeft::_ZTv0_n24_N5VLeftD0Ev
96    (int (*)(...))VLeft::_ZTv0_n32_N5VLeft1vEv

Class VLeft
   size=32 align=8
   base size=12 base align=8
VLeft (0x0x...) 0
    vptridx=0 vptr=((& VLeft::_ZTV5VLeft) + 24)
VBase (0x0x...) 16 virtual
      vptridx=8 vbaseoffset=-24 vptr=((& VLeft::_ZTV5VLeft) + 80)
```

- `VBase` 子对象在 `VLeft` 里的偏移是 16，`vbaseoffset=-24` 表示从 `VBase`
  子对象「往回 24 字节」 回到 `VLeft` 起点（VBase 自己的 vptr 在 16，`VLeft` 的
  vptr 在 0，差 16；thunk 的 `n24` 又是另一段补偿）。
- `_ZTv0_n24_...` / `_ZTv0_n32_...`：`Tv` = virtual thunk，`v0` 表示从 vtable 的
  0 号槽位 （offset-to-top）读取 this 调整量，`n24`/`n32` 是额外常量补偿。这些
  thunk 让「通过虚基类指针调虚函数」 也能把 `this` 调回正确位置。

### 3.2 VTT（virtual table table）

```
VTT for VMost
VMost::_ZTT5VMost: 7 entries
0     ((& VMost::_ZTV5VMost) + 24)
8     ((& VMost::_ZTC5VMost0_5VLeft) + 24)
16    ((& VMost::_ZTC5VMost0_5VLeft) + 80)
24    ((& VMost::_ZTC5VMost16_6VRight) + 24)
32    ((& VMost::_ZTC5VMost16_6VRight) + 72)
40    ((& VMost::_ZTV5VMost) + 120)
48    ((& VMost::_ZTV5VMost) + 72)
```

VTT 是一张「表指向表」的索引表。`VMost` 的构造是逐层进行的：先构造 `VBase`，再
`VLeft`、`VRight`， 最后才是 `VMost`
自己。每一层构造完，都要把**当前这一层**正确的 vptr 写进对象；这些 vptr 的取值
就从 VTT 里查。

### 3.3 construction vtable

```
Construction vtable for VLeft (0x...) instance in VMost
VMost::_ZTC5VMost0_5VLeft: 13 entries
0     32
...
24    (int (*)(...))VLeft::v
32    0
40    0
...
96    (int (*)(...))VLeft::_ZTv0_n32_N5VLeft1vEv
```

`_ZTC`（construction vtable）里的很多槽位是 0。在 `VMost`
还没构造完成时，若虚调用命中了 这些尚未就绪的函数，直接走这些 0
槽位会立刻暴露问题，而不是调到错误实现。构造完成后， 编译器把 vptr 切回
`_ZTV`（完整对象的 vtable）。

运行输出：`VMost` 的 `sizeof` 从「两个非虚基类」的 32 涨到
48，多出来的正是虚基类 `VBase` 子对象（含它自己的 vptr）：

```
== 菱形虚继承: VMost : VLeft, VRight : virtual VBase ==
sizeof(VMost) = 48
VLeft::v
VLeft::v
VMost              vptr=0x...  offset-to-top=0  typeinfo=0x...
```

## 4. RTTI：就藏在 vtable 里

RTTI 不是某种额外的旁路机制，它只依赖 vtable 里的两样东西：

- `vptr[-1]` 的 **typeinfo 指针**（`_ZTI...`），就是 `typeid(obj)` 拿到的
  `std::type_info`。
- `vptr[-2]` 的
  **offset-to-top**，用于把「某个子对象的指针」换算回「完整对象的指针」。

`dynamic_cast` 在运行时的工作大致是：拿到对象的 typeinfo 和
offset-to-top，恢复出完整对象， 再沿着完整对象的继承图（编译器生成在 typeinfo
旁边的一张表）去判断「目标类型是否可达」； 可达就把 `this`
调整到目标子对象，不可达就返回 `nullptr`。所以：

- 单继承、offset-to-top 恒为 0 时，`dynamic_cast` 只是查表 + 不调整
  this，开销小。
- 多继承/虚继承里，offset-to-top 和 vbase offset 才是 `dynamic_cast`
  需要做「指针修正」的来源。
- 前面多继承的例子已经印证：`(Left*)&Multi` 和 `(Right*)&Multi` 的 `typeinfo`
  相同（都指向 `_ZTI5Multi`，即完整对象的真实类型），只有 `offset-to-top` 不同。

这也是为什么 `dynamic_cast` 只能作用于**多态类型**（有虚函数的类）——没有
vptr，就没有 typeinfo 和 offset-to-top，运行时无从谈起。

## 5. 小结

| 场景   | vptr 数量                  | 关键机制                                                |
| ------ | -------------------------- | ------------------------------------------------------- |
| 单继承 | 1（派生类复用基类）        | vtable 槽位替换即可，this 不变                          |
| 多继承 | 每个非 primary base 各一个 | 二级 vtable + thunk（`_ZThn16_...`）修正 this           |
| 虚继承 | 每个虚基类子对象各一个     | vbase offset / vcall offset + VTT + construction vtable |
| RTTI   | —                          | vtable 槽位 `[-1]` typeinfo、`[-2]` offset-to-top       |

一句话：**vtable 存的既是「调哪个函数」，也是「this
该加多少」和「我到底是什么类型」**。

## 参考资料

- [Itanium C++ ABI: Class Layout / Virtual Table Layout](https://itanium-cxx-abi.github.io/cxx-abi/abi.html)
- [cppreference: dynamic_cast](https://en.cppreference.com/w/cpp/language/dynamic_cast)
- [cppreference: typeid](https://en.cppreference.com/w/cpp/language/typeid)
- 《深度探索 C++ 对象模型》（Lippman），第 1、3、4 章
- C++为什么要弄出虚表这个东西？ - 果冻虾仁的回答 - 知乎
	- https://www.zhihu.com/question/389546003/answer/1194780618

## 问题考察
- class 的大小
  - [空类的大小为 1 字节](https://stackoverflow.com/questions/621616/c-what-is-the-size-of-an-object-of-an-empty-class)
  - 集成获取所有的成员
  - 集成获取获取 vptr （取决于 parent 的数量, 不是 ancester 的数量）

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
