## 比想象的还复杂
2. {} 和 ()
	- {} 和 () 都是可以初始化 int 或者数组的，以及对象的
	- {} 和 () 也可以初始化对象
	- 如果 class 定义了 initializer_list ，那么就是用 {} 转换了
		- 但是似乎没有定义也是可以的，不过都是 init
	- {} 来做的前提是，aggregate 类型?

4. 有时候真的有点无语:

D() 可能是初始化，也可能 C d(A()); 这都是不行的

## 大纲
存在哪些对象:
1. array
2. object
4. int
5. aggregate

哪些写法

引入的问题:
1. initializer list
2. 不要忘记了，当多出来了 new 关键字之后，这些初始化方法都是如何做的?
3. aggregate 可以用 () 来初始化
4. default 和 value initialization
5. implict 转换

## 确认的细节
1. new 是不是


## 忽然意识到

int a; 不是 initialization ，而是一个 declaration 啊
但是如果 A a; 中 , class A 已经定义了 constructor
那么 A 也是需要有什么

## 再次强调如此出现错误的两个情况

另一个容易混淆的写法是 `B g = (1, 2);`：右侧是逗号表达式，类型为 `int`、值为 `2`。
它既不是两个构造参数，也不是初始化列表；这里才是在尝试从 `int` 初始化 `B`。

A a();

不要写：

```cpp
Widget widget();
```

这通常被解析为名叫 `widget`、没有参数、返回 `Widget` 的函数声明，即 most vexing
parse（最令人烦恼的解析）。 创建默认对象应写：

```cpp
Widget widget;
Widget another{};
```

TODO 这个 vexing 是一个专用词汇?

窄化转换如何理解?

### auto

### 数组还有特殊规则?

## Initialization

还是应该看看这里的规则吧:

https://cppreference.com/cpp/language/copy_initialization

Default-initialization

Copy-initialization
Direct-initialization
List-initialization (C++11)

Value-initialization
Aggregate initialization
Reference initialization
Static non-local initialization
    zero – constant
Dynamic non-local initialization
    ordered – unordered
Copy elision (RVO)

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
