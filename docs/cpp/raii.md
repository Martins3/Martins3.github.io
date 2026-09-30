# cpp RAII
<!-- dd435eeb-4529-4700-9273-e1bada3b65d6 -->

RAII 是 Resource Acquisition Is Initialization
- 将资源的生命周期与对象的生命周期绑定——在对象构造时获取资源，在对象析构时自动释放资源。

也就是释放对象，那么自动释放关联的资源

 为什么需要 RAII
 在 C/C++ 中，像内存、文件句柄、网络连接、锁这样的资源，如果手动管理（申请后忘记释放），很容易导致：
 • 内存泄漏
 • 资源耗尽
 • 异常安全漏洞（发生异常时跳过了释放代码）
 RAII 通过利用 C++ 的构造函数和析构函数机制，确保资源无论函数是正常返回还是异常退出，都一定会被正确释放。

 典型示例
 1. 智能指针（管理动态内存）

```c
 #include <memory>

 void foo() {
     std::unique_ptr<int> ptr(new int(42));  // 构造时分配内存
     // 使用 ptr...
     // 函数退出时（无论正常返回还是异常），ptr 的析构函数自动释放内存
 }  // 自动 delete

```
 2. std::lock_guard（管理互斥锁）
 #include <mutex>

 std::mutex mtx;

 void thread_safe_func() {
     std::lock_guard<std::mutex> lock(mtx);  // 构造时加锁
     // 临界区代码...
 }  // 析构时自动解锁，即使发生异常也会解锁
 3. std::fstream（管理文件句柄）
 #include <fstream>

 void write_file() {
     std::ofstream file("data.txt");  // 构造时打开文件
     file << "hello";
 }  // 析构时自动关闭文件

 4. 自定义 RAII 类
 class FileHandle {
     FILE* file;
 public:
     FileHandle(const char* name, const char* mode) {
         file = fopen(name, mode);
     }
     ~FileHandle() {
         if (file) fclose(file);  // 保证文件会被关闭
     }
     // 禁止拷贝，允许移动（省略）
 };
 ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
 总结
  特性       说明
 ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  核心思想   资源获取即初始化，资源释放即销毁
  依赖机制   C++ 的构造函数和析构函数
  主要优势   异常安全、避免泄漏、代码简洁
  常见应用   智能指针、锁守卫、文件句柄、数据库连接等

 RAII 被认为是 C++ 中最重要、最优雅的惯用法之一，现代 C++ 编程强烈建议遵循此原则。

### RAII
https://stackoverflow.com/questions/2321511/what-is-meant-by-resource-acquisition-is-initialization-raii

并不是什么高级功能

为什么叫做
https://en.cppreference.com/w/cpp/language/move_constructor
为什么 move 需要 noexcept

- https://stackoverflow.com/questions/3413470/what-is-stdmove-and-when-should-it-be-used
- https://stackoverflow.com/questions/3106110/what-is-move-semantics
- https://stackoverflow.com/questions/3601602/what-are-rvalues-lvalues-xvalues-glvalues-and-prvalues

- A glvalue (“generalized” lvalue) is an lvalue or an xvalue.
- An rvalue is an xvalue, a temporary object or subobject thereof, or a value that is not associated with an object.
- A prvalue (“pure” rvalue) is an rvalue - xvalue.

所以，lvalue xvalue prvalue 是纯粹的

glvalue = lvalue + xvalue
rvalue = xvalue + prvalue

rvalue can be moved, either because it's a temporary (prvalue) or explicitly moved (xvalue)

- https://stackoverflow.com/questions/3582001/what-are-the-main-purposes-of-stdforward-and-which-problems-does-it-solve
    - 解释的太好了，解释了 万有引用 完美转发
    - [ ] 所以，如果

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
