// 分析 C++ vtable 与 RTTI 布局的演示程序。
//
// 故意不包含任何头文件，这样 clang -cc1 -fdump-record-layouts 和
// g++ -fdump-lang-class 的输出里只有我们自己定义的类，不会被标准库噪音淹没。
//
// 运行:
//   g++ -std=c++20 actress.cpp -o actress.out && ./actress.out
//
// 查看内存布局:
//   clang -cc1 -fdump-record-layouts actress.cpp
//   g++ -fdump-lang-class -c actress.cpp        # 生成 actress.cpp.001l.class

extern "C" int printf(const char *fmt, ...);

// ---- 单继承 -------------------------------------------------------------
struct Base {
    virtual ~Base() {}
    virtual void f() { printf("Base::f\n"); }
    virtual void g() { printf("Base::g\n"); }
    int b = 1;
};

struct Derived : Base {
    void f() override { printf("Derived::f\n"); }
    int d = 4;
};

// ---- 多继承 (两个非空基类, 各有一个 vtable) ------------------------------
struct Left {
    virtual ~Left() {}
    virtual void l() { printf("Left::l\n"); }
    int lv = 2;
};

struct Right {
    virtual ~Right() {}
    virtual void r() { printf("Right::r\n"); }
    int rv = 3;
};

struct Multi : Left, Right {
    void l() override { printf("Multi::l\n"); }
    void r() override { printf("Multi::r\n"); }
    int m = 5;
};

// ---- 菱形虚继承 (需要 VTT 和 virtual base offset) ------------------------
struct VBase {
    virtual ~VBase() {}
    virtual void v() { printf("VBase::v\n"); }
    int vb = 6;
};

struct VLeft : virtual VBase {
    void v() override { printf("VLeft::v\n"); }
    int vl = 7;
};

struct VRight : virtual VBase {
    int vr = 8;
};

struct VMost : VLeft, VRight {
    int vm = 9;
};

// Itanium ABI (Linux x86-64 / ARM64) 约定:
//   对象第一个槽位存放 vptr, vptr 指向"第一个虚函数"所在的地址。
//   紧挨着它前面还有两个槽位, 属于 vtable 的管理信息:
//     vptr[-2] = offset-to-top  当前子对象回到完整对象起点的字节偏移
//     vptr[-1] = typeinfo 指针   RTTI, 即 g++ dump 里的 _ZTI... 符号
void dump_vtable(const char *name, void *obj)
{
    void **vptr = *(void ***)obj;
    long offset_to_top = (long)vptr[-2];
    void *rtti = vptr[-1];
    printf("%-18s vptr=%p  offset-to-top=%ld  typeinfo=%p\n",
           name, (void *)vptr, offset_to_top, rtti);
}

int main()
{
    printf("== 单继承: Derived : Base ==\n");
    Derived d;
    Base *pb = &d;
    printf("&d = %p, (Base*)&d = %p (地址相同, offset-to-top = 0)\n",
           (void *)&d, (void *)pb);
    pb->f(); // 虚调用, 运行时从 vtable 找到 Derived::f
    dump_vtable("(Base*)&Derived", pb);

    printf("\n== 多继承: Multi : Left, Right ==\n");
    Multi m;
    Left *pl = &m;
    Right *pr = &m;
    printf("&m = %p, (Left*)&m = %p (相同)\n", (void *)&m, (void *)pl);
    printf("(Right*)&m = %p (this 被 +16, 指向第二个 vtable)\n", (void *)pr);
    pr->r(); // 虚调用, 经 thunk 把 this 减 16 后调用 Multi::r
    dump_vtable("(Left*)&Multi", pl);
    dump_vtable("(Right*)&Multi", pr);

    printf("\n== 菱形虚继承: VMost : VLeft, VRight : virtual VBase ==\n");
    VMost v;
    printf("sizeof(VMost) = %zu\n", sizeof v);
    VBase *pvb = &v;
    v.v();    // 都解析到 VLeft::v
    pvb->v(); // 虚基类子对象在对象尾部 offset 32, 需要 vbase offset 才能定位
    dump_vtable("VMost", &v);

    return 0;
}
