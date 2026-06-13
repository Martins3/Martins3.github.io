# qemu 如何做测试的

https://www.qemu.org/docs/master/devel/testing/main.html

- make check-unit V=1

看来 unit test 也是链接所有的东西
```txt
🧀  l tests/unit/test-aio
Permissions Size User     Date Modified Name
.rwxr-xr-x   11M martins3  6 Jan 21:58   tests/unit/test-aio
```

- make check-qtest

```txt
🧀  make check-help
Regression testing targets:
 make check                    Run block, qapi-schema, unit, softfloat, qtest and decodetree tests
 make bench                    Run speed tests

Individual test suites:
 make check-qtest-TARGET       Run qtest tests for given target
 make check-qtest              Run qtest tests
 make check-functional         Run python-based functional tests
 make check-functional-TARGET  Run functional tests for a given target
 make check-unit               Run qobject tests
 make check-qapi-schema        Run QAPI schema tests
 make check-tracetool          Run tracetool generator tests
 make check-block              Run block tests
 make check-tcg                Run TCG tests
 make check-softfloat          Run FPU emulation tests

 make check-report.junit.xml   Generates an aggregated XML test report
 make check-venv               Creates a Python venv for tests
 make check-clean              Clean the tests and related data

The following are useful for CI builds
 make check-build              Build most test binaries


The variable SPEED can be set to control the gtester speed setting.
Default options are -k and (for make V=1) --verbose; they can be
changed with variable GTESTER_OPTIONS.
```

## 也看看这个
https://wiki.qemu.org/Testing/Machines

## docs/kvm/kvm-forum/2025.md 中的
The next generation QEMU functional testing framework
也是可以看看的

## 更加宽泛的话题，如何测试一个 static 函数

对静态（`static`）函数进行单元测试是一个常见但有挑战的问题，因为 `static` 函数在 C 语言中具有内部链接（internal linkage），即其作用域限定在定义它的编译单元（translation unit）内，从其他源文件（例如测试文件）无法直接调用。

知名项目通常采用以下几种策略之一来解决这个问题：

### ✅ 策略一：**在测试时将 `static` 函数改为非 `static`（或通过宏控制）**

这是 **Linux 内核** 和 **SQLite** 等项目常用的做法。

#### 示例：SQLite 的做法
SQLite 在其测试框架中，会为测试目的专门构建一个“测试版本”的源文件，其中通过预处理器宏取消 `static` 限定。

例如，在正常代码中：

```c
// file: btree.c
static int decodeFlags(BtPage *pPage) {
    return pPage->flags & 0x0F;
}
```

在测试时，通过定义宏来覆盖 `static`：

```c
// 编译测试时：-Dstatic=
```

这样 `static` 被预处理器替换为空，函数变成外部可见，测试文件就可以调用它。

> 这种方式虽然“hacky”，但简洁、无需修改原始逻辑，被 SQLite 广泛使用。

### ✅ 策略二：**将静态函数抽取到可测的辅助头文件中（测试专用头）**

例如 **OpenSSL** 和 **FFmpeg** 在部分模块中采用这种方式。

#### 做法：
- 正常实现保留在 `.c` 文件中。
- 创建一个 `xxx_test.h`，在其中用 `extern` 声明原本 `static` 的函数（需在编译时让函数非 `static`，通常配合宏）。

但本质上仍需改变 `static` 属性。

---

### ✅ 策略三：**通过“测试代理函数”间接测试**

即不在测试中直接调用 `static` 函数，而是通过调用公开的非 `static` 函数，覆盖 `static` 函数的逻辑路径。

这是 **Google Test** 文档推荐的“黑盒测试”思想，也被 **systemd** 等项目采用。

#### 优点：
- 保持封装性。
- 测试的是真实使用路径。

#### 缺点：
- 若 `static` 函数逻辑复杂且未被充分覆盖，难以验证边界条件。

---

### ✅ 策略四：**使用链接时替换或测试专用构建（Link-time instrumentation）**

例如 **Linux 内核的 KUnit** 测试框架：

- 在 KUnit 中，测试文件和被测代码**编译到同一个内核模块**中。
- 因此，即使函数是 `static`，只要测试代码和实现代码在**同一个编译单元**（或通过 `#include` 实现文件），就能访问。

#### 示例（简化版 KUnit 风格）：

```c
// mymodule.c
static int helper(int x) { return x * 2; }

int public_api(int y) { return helper(y) + 1; }
```

测试文件：

```c
// mymodule_test.c
#include "mymodule.c"  // 直接包含 .c 文件！

static void test_helper(struct kunit *test)
{
    KUNIT_EXPECT_EQ(test, helper(3), 6);
}
```


### 总结

| 方法 | 优点 | 缺点 | 适用场景 |
|------|------|------|--------|
| `-Dstatic=` | 简单直接 | 可能符号冲突 | 小型项目、SQLite 风格 |
| `#include .c` | 保留 `static` 语义，测试隔离好 | 构建复杂，需防重复定义 | 内核、嵌入式（如 KUnit） |
| 黑盒测试（不测 static） | 保持封装 | 覆盖率可能不足 | 高层 API 逻辑清晰时 |
| 抽取为非 static（测试专用） | 灵活 | 需维护两套接口 | 大型项目需精细控制 |

如果你希望保持代码不变且追求高覆盖率，**`#include "target.c"` 是最可靠的方式**，尤其适合你熟悉的系统级 C 项目。

如需具体 Makefile 或 CMake 示例，也可以告诉我。

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
