# cpp template : fold

<!-- 9e6e45cb-d9f3-4ad1-80df-fae94f88bea2 -->

## 首先回顾一下 C 风格的可变参数

普通函数可以使用 C 风格的可变参数：

```c
#include <cstdarg>

void print(int count, ...)
{
	va_list args;
	va_start(args, count);

	for (int i = 0; i < count; ++i) {
		int value = va_arg(args, int);
		std::cout << value << '\n';
	}

	va_end(args);
}
```

但这两种机制完全不同：

| 写法                       | 所属机制       | 类型安全 |
| -------------------------- | -------------- | -------- |
| template<typename... Args> | 可变参数模板   | 是       |
| void f(int count, ...)     | C 风格可变参数 | 否       |

## 基本原理

折叠表达式（fold expression）是 C++17 引入的语法糖，
用来把一个**参数包**用某个二元运算符“折叠”成一个表达式。 你可以把它理解成：**把
`args...` 展开后，用运算符 `op` 串起来，并决定括号从左加还是从右加。**

## 四种形式

假设参数包 `args...` 展开为 `a, b, c`，运算符为 `op`：

| 写法                    | 名称       | 展开结果                    |
| ----------------------- | ---------- | --------------------------- |
| `(... op args)`         | 一元左折叠 | `((a op b) op c)`           |
| `(args op ...)`         | 一元右折叠 | `(a op (b op c))`           |
| `(init op ... op args)` | 二元左折叠 | `(((init op a) op b) op c)` |
| `(args op ... op init)` | 二元右折叠 | `(a op (b op (c op init)))` |

记忆技巧：**看 `...` 在参数包的左边还是右边**。

- `...` 在包左边 → 左折叠
- `...` 在包右边 → 右折叠

“左/右”指的是结合方向，不是初始值的位置。

## 例子

### 1. 求和

```cpp
template<class... Ts>
auto sum1(Ts... ts) {
    return (ts + ...);   // 一元右折叠：1 + (2 + (3 + 4))
}

template<class... Ts>
auto sum2(Ts... ts) {
    return (... + ts);   // 一元左折叠：((1 + 2) + 3) + 4
}
```

加法有结合律，结果一样。但减法、除法、移位等就不一样：

```cpp
template<class... Ts>
auto sub1(Ts... ts) {
    return (ts - ...);   // 1 - (2 - (3 - 4))
}

template<class... Ts>
auto sub2(Ts... ts) {
    return (... - ts);   // ((1 - 2) - 3) - 4
}
```

### 2. 带初始值的二元折叠

一元折叠不能处理空参数包，也不能指定初始值。二元折叠可以：

```cpp
template<class... Ts>
auto sum(Ts... ts) {
    return (0 + ... + ts); // 二元左折叠：(((0 + a) + b) + c)
}
```

如果 `ts` 为空，返回 `0`。

### 3. 打印任意参数

```cpp
template<class... Ts>
void print(Ts&&... ts) {
    (std::cout << ... << ts) << '\n';
}
```

`(std::cout << ... << ts)` 是二元左折叠，展开为：

```cpp
((std::cout << ts1) << ts2) << ts3;
```

### 4. 逻辑与

```cpp
template<class... Ts>
bool all(Ts... ts) {
    return (... && ts); // 一元左折叠：((a && b) && c)
}
```

空参数包时，`(... && ts)` 返回 `true`。

### 5. 逗号折叠：依次执行

```cpp
template<class... Fs>
void call_all(Fs&&... fs) {
    (std::forward<Fs>(fs)(), ...); // 依次调用 fs()
}
```

空参数包时，逗号折叠返回 `void()`。

## 空参数包规则

- 一元折叠对空包：
  - `&&` → `true`
  - `||` → `false`
  - `,` → `void()`
  - 其他运算符 → 非法
- 二元折叠对空包：直接返回初始值 `init`。

## 注意事项

1. 折叠表达式必须加括号：
   ```cpp
   return (... + args); // 正确
   // return ... + args; // 错误
   ```

2. 二元折叠中，两个 `op` 必须相同：
   ```cpp
   (0 + ... + args) // 正确
   (0 + ... - args) // 错误
   ```

3. 支持大多数二元运算符，如 `+ - * / && || << ,` 等，但不支持
   `.`、`->`、`::`、`?:` 等。

4. 求值顺序：除了
   `&&`、`||`、逗号等本身有顺序规则的运算符，其他折叠一般不保证操作数求值顺序。

## 一句话总结

折叠表达式就是：**把参数包 `args...` 用运算符 `op` 从左边或右边依次连接起来**。
一元折叠没有初始值；二元折叠多一个初始值，常用于处理空包和指定类型。

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
