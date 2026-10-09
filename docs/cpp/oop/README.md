## 方便的工具
https://github.com/andreasfertig/cppinsights : cppinsights

## 深度探索 C++ 对象模型
1. [探索 Default Constructor](https://mp.weixin.qq.com/s/xmzsKOXJykZUcco4xZimiA)
2. [探索 Copy Constructor](https://mp.weixin.qq.com/s/zk_wjy6a6u4d-DYy3lPqdg)
  - 总的来说，当需要特殊处理一些事情的时候，那么就需要 non trivial implict constructor 的出现
  - [ ] 理解了 virtual inheritence 的时候再 section 5
  - [ ] 部分细节也需要好好整理清楚
3. [程序转化语义学](https://mp.weixin.qq.com/s?__biz=MzA3ODAzMTI0Nw==&mid=2247483740&idx=1&sn=4c292a5f88dee9df3a7a92de3db7e586&chksm=9f49b1bca83e38aa03e2e06bf9bf665e85ee6c233c8a11d980fd5c348107234b78b255b5210f&scene=132#wechat_redirect)
  - NVR 的优化

## 注意项目

### 所以最好不要在 virtual function 中间使用 default parameter
	- https://stackoverflow.com/questions/3533589/can-virtual-functions-have-default-parameters

参考 demo 为: docs/cpp/oop/virtual-default-parameter.cpp

也就是不要

### 析构函数可以声明为虚函数

这就是 effective cpp 中经常聊到的，析构函数

如果我们需要删除一个指向派生类的基类指针时，
应该把析构函数声明为虚函数。
事实上，只要一个类有可能会被其它类所继承，
就应该声明虚析构函数(哪怕该析构函数不执行任何操作)。**

- https://stackoverflow.com/questions/461203/when-to-use-virtual-destructors

### 虚函数可以为私有函数吗?
https://stackoverflow.com/questions/2170688/private-virtual-method-i

最好私有，这也是 effective cpp 中提到的，之所以作为 virtual

### 虚函数可以被内联吗?

- **通常类成员函数都会被编译器考虑是否进行内联。
但通过基类指针或者引用调用的虚函数必定不能被内联。
当然，实体对象调用虚函数或者静态调用时可以被内联，虚析构函数的静态调用也一定会被内联展开。**



## 似乎我们有一些概念，可以做一个表格来分析下

static virtual override default paarater private / public
inline

成员/函数 const 和 friend 类型 explicit

default

各种元素的耦合都是什么关系

## 总体来说，面向对象和多线程是没有关系的
不过我忘记了，什么时候我们是需要考虑的

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
