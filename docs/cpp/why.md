# 为什么我在 2026 年学习 c++

首先，因为我之前没学。

AI 让学习 c++ 的痛苦部分基本上下降为 0 ，剩下的部分都是
可以让人思考的部分，这些可以做实验的小游戏是很有意思的。

大量的项目还是 c++ 实现的，这是工作中需要的。

最近用 C 语言写了一个比较大的项目，
数据结构管理全部使用链表，体验非常之差。
在 2026 年，其实我认为任何项目使用的语言都应该收敛到 Rust 和 Python。

## 为什么 C++ 这么复杂

1. 35 法则 : 和 operator 有关，和 move 有关
2. move 和引用有关
3. move 和 template 有关

C++ 是需要反复阅读，现在是反复思考，实验，直到感觉到整个思考过程都是流程的。

这里说几个我感觉后知后觉的例子:
1. implict 类型转换最多只能转换一次
2. constexpr -> literal class

一个直观的对比，容量就小很多:
- https://cppreference.com/cpp/language
- https://cppreference.com/c/language

### 真的很像 bash

#### 相同的事情，可以有多种写法:
init 初始化，我是真的感觉这个东西设计复杂了。

new int();
new int;
这两个区分，有点逆天了

3. () 和 {} 的功能总结一下，都是可以表达什么?

class 和 typename 有功能重叠，
但是他们也的确是表示不同的功能

#### 过于强大的简写
- implict
- 构造函数
- operator 功能



## 问题

1. 为了容易写 lib ，然后 c++ 搞了非常复杂的 template 机制，那么我的问题是，
那么其他的语言都是如何解决这个问题的?

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
