# 关于函数式编程的一些理解
 这一篇文章简述一下我对于的函数式编程的理解，函数式编程想必大家在不经意的时候
听说过，但是忙于手头上的 事情而没有一探究竟。由于学校的老师在CMU接受培训了函数式，
笔者在大学生活中间一再和函数式编程打交道，分别 修过以函数式语言为基础数据结构和
SML语言，所以如今再次总结一番。

    函数式编程，无论是谁，都是没有办法忽视的东西，即使你从来没有听说过的Haskell,
Lisp以及Ocaml, 但是你必定听说过C/C++ , Java, JavaScript 以及Python,
你甚至用其中一两门写过管理系统,
但是这些语言全部都是支持的函数式的，面对这些来势汹汹的新特性，新思想，是时候停下
手中的工作来了解的一下函数式了。

## 函数式编程的历史
  函数式编程给人的感觉非常的高大上，由此推测函数式编程是最近开发出来的黑科技，
但是实际上函数式编程语言历史 非常悠久，最早要追溯到上个世纪五十年代，要知道
C语言的发明时间是[1972年](https://en.wikipedia.org/wiki/C_(programming_language),
而C++, java的发明的时间更是在[1985](http://www.cplusplus.com/info/history/)和
[1995](https://en.wikipedia.org/wiki/Java_(programming_language) 才姗姗来迟。

Kenneth E. Iverson 在上个世纪60年代发明了APL语言，他的一本叫做*A Programming Language*详尽描述该语言。
上个世纪70年代，Robin Milner发明了ML语言。
1980年代末期，集函数式编程研究成果于大成的Haskell发布。

## 谈一谈SML/NJ
  大学从来不缺少SML/NJ(以下简称SML)的陪伴，首先在大二上学期时候学习**串并行数据结构**,
这一门课使用SML作为 描述语言，但是上这一门课之前并没有的修过这一门课程, 所以必须
很悲催的边上实验课边 完成学习SML语言，更加绝望的是，授课老师对于这一门语言不是非
常的熟悉，以至于我们 没有办法从他那里得到任何的帮助，当然，这还不算让人绝望的，
那就是当所有试验费劲千辛万苦的完成之后，我们有开设了一门SML语言学习课程。

### SML的基本语法
  SML的语法命令式环境的思考角度会有一些奇怪，比如循环结构，比如函数的定义等等，
任何事物脱离了他的设计哲学都会编程奇技淫巧，当你学习了不止一门函数式语言之后，你
就会发现函数式语言和命令式语言相同，大体相似而细节不同。

### 真的需要SML吗？
  其实是没有必要的，SML的初衷就是的为了教学而书写语言，大多数函数式的语言都非常
的不接底气。第一，从来没有分析过计算性能问题，计算机体系结构中间的一再强调不要的
使用递归，但是实际上函数式却在分析如何将递归修改为尾递归。第二， SML的一个非常的
严重的问题就是他没有处理一些最基本业务的完善的函数，比如IO, 进程线程管理。最后，
既然下定决心学习函数式编程，F#, Haskell Ocaml等语言的影响更加广泛，所以在实际编
程中间遇到的问题必定会有更加多的人提供帮助。最后的最后，抱怨一下，我实在的是不知
道的为什么需要会开设函数式编程的语言的课程的时候使用SML作为基础，这么语言实在看
到不到的什么压制其他函数式语言的优势，恐怕真正原因是在人事上面的？

### 测试一下SML
总所周知, SML中间含有list类型，基本原理是链表，而C++的STL库文件含有一个stack
的数据结构，实现的基础也是的链表， 但是二叉树的实现两者就大相径庭， 
SML的实现的基础是， 通过类型的定义， 比如
` datatype tree = Lf | Node of tree * int * tree; `,
但是C++是通过的定义结构体或者类， 然后让类中间持有指针来实现树的构建，
现在分别测试两种数据结果在传统的面向过程和函数式编程的效率

首先测试的SML类型的插入，对应的SML代码如下:
```
  val l = [];
  fun f(0:int) = [0]
    | f(x) = x::f(x  - 1);

  fun insertList([], t) = t
    | insertList(l::X, t) = insertList(X, insertElements(t, l));

  val timer = Timer.startCPUTimer();

  val l = f(1000000);
  val a =   Timer.checkCPUTimes(timer);
```

结果如下:
>   val a =
>   {gc={sys=TIME {usec=0},usr=TIME {usec=464620}},
>   nongc={sys=TIME {usec=15103},usr=TIME {usec=499494}}}
>   : {gc:{sys:Time.time, usr:Time.time}, nongc:{sys:Time.time, usr:Time.time}}
可以发现， 即使添加是1000000的数据量的插入，依旧是只需要499ms的时间

使用C++的stack的push 操作重复上述过程，对应的代码如下
```cpp
#include <ctime>
#include <cstdio>
#include <stack>
#include <iostream>
#include <cstddef>

using namespace std;

int main(){
    clock_t begin = clock();
    stack<int> s;
    for (int i = 0; i < 1000000; ++i) {
        s.push(i);
    }
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC * 1000;
    printf("%lf ms", elapsed_secs);
    return 0;
}
```
结果如下：
> 23.639000 ms

其实两者直接已经含有较大的差距了。

下面来看二叉树插入，而且采用特殊数据插入， 查看对应结果
SML对应的源代码
```
  val l = [];
  fun f(0:int) = [0]
    | f(x) = x::f(x  - 1);

  datatype tree = Lf | Node of tree * int * tree;
  val t: tree = Lf;


  fun reT Lf = Lf
    | reT(Node(L, a, R)) = Node(reT(R), a, reT(L));

  fun insertElements(Lf, x)= Node(Lf, x, Lf)
     | insertElements(Node(l, n, r), x) =
                      case Int.compare(x , n) of
                        EQUAL => Node(l, x, r)
                      | LESS => Node(insertElements(l, x), n, r)
                      | GREATER => Node(l, n, insertElements(r, x))

  fun insertList([], t) = t
    | insertList(l::X, t) = insertList(X, insertElements(t, l));

  val timer = Timer.startCPUTimer();

  val l = f(10000);
  (* val t = insertList(l, Lf); *)


  val a =   Timer.checkCPUTimes(timer);

```

测试结果:
> val a =
>  {gc={sys=TIME {usec=0},usr=TIME {usec=44839}},
>   nongc={sys=TIME {usec=49878},usr=TIME {usec=1247284}}}
>  : {gc:{sys:Time.time, usr:Time.time}, nongc:{sys:Time.time, usr:Time.time}}
>  消耗时间大约2000ms

CPP对应的源代码
```
#include <ctime>
#include <cstdio>
#include <stack>
#include <iostream>
#include <cstddef>

using namespace std;

class Node{
public:
    Node * left;
    Node * right;
    int val;
    Node():left(nullptr), right(nullptr){}
    Node(int _val):left(nullptr), right(nullptr), val(_val){}
};

void insert(Node * node, int x){
    if(x == node->val) return;
    if(x < node->val){
        if(node->left == nullptr)
            node->left = new Node(x);
        else
            insert(node->left, x);
    }
    if(x > node->val){
        if(node->right == nullptr)
            node->right = new Node(x);
        else
            insert(node->right, x);
    }
}

void insertStack(Node * node, stack<int>& s){
    while(!s.empty()){
        insert(node, s.top()); s.pop();
    }
}

int main(){
    clock_t begin = clock();
    stack<int> s;
    for (int i = 0; i < 10000; ++i) {
        s.push(i);
    }
    Node node(0);
    insertStack(&node, s);

    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC * 1000;
    printf("%lf ms", elapsed_secs);
    return 0;
}
```
测试结果:
> 554.3 ms
通过上面两个例子比较， 可以发现SML的效率虽然低，但是依旧优秀。


### SML参考资料
1. [ML程序设计教程](https://book.douban.com/subject/1316040/)

## 说一说Haskell

### 运行第一行Haskell代码
  对于的ubuntu的安装, 只是需要一条命令就可以
```
sudo apt-get install haskell-platform
```
  我自己的使用的Haskell编辑器是vim, 需要安装的插件是[haskell-vim-now](https://github.com/begriffs/haskell-vim-now) 和[haskell-vim](https://github.com/neovimhaskell/haskell-vim),前者相当于一个IDE的配置，后者提供了简单缩进和高亮的选项.
  万事具备，只欠东风，所以来开始基本Haskell学习。ghci是haskell的解释器，当在终端中间输入ghci, 然后就可以的实现的交互，实现的效果大概是下面

显然交互式满足不了很多场景的需求，比如当源代码很大的时候，当需要保存源代码的时候
，这时候书写对应的脚本然后使用haskell的编译器ghc会更加方便，例如如下的源代码，

保存为文件a.hs, 然后执行ghc a.hs  && ./a 执行结果如下所示

### Haskell的基本语法
1. 简单数据类型
    1. Int is an integer with at least 30 bits of precision.
    2. Integer is an integer with unlimited precision.
    3. Float is a single precision floating point number.
    4. Double is a double precision floating point number.
    4. Rational is a fraction type, with no rounding error.
2. 结构数据类型
    1. list
    2. tuple
3. list常用的操作
    1. 获取列表下标
    ```
    [0..] !! 5
    ```
    2. 连接两个list
    3. 往列表头增加元素
    ```
    0:[1..5]
    ```
    4. 常用函数
    ```
    head [1..5] -- 1
    tail [1..5] -- [2, 3, 4, 5]
    init [1..5] -- [1, 2, 3, 4]
    last [1..5] -- 5
    ```
    5. 列表推导 (list comprehension)
    ```
    [x*2 | x <- [1..5]] -- [2, 4, 6, 8, 10]
    ```
4. 类型
    1. 使用 :: 可以指定类型
    2. 使用 :t 可以实现检查变量的类型

5. 函数

haskell的函数定义和面向对象和面向过程的语言的不同，由于haskell使用的是模式匹配的
的方法，下面是一个例子。
```
qsort [] = []
qsort (p:xs) = qsort lesser ++ [p] ++ qsort greater
    where lesser  = filter (< p) xs
          greater = filter (>= p) xs
```


6. main
当执行一个 Haskell 程序时，函数 `main` 就被调用。 它必须返回一个类型 `IO ()` 的值
```
main :: IO ()
main = putStrLn $ "Hello, sky! " ++ (say Blue)
```


### Haskell的参考资料
1. [Haskell趣学指南](https://legacy.gitbook.com/book/mno2/learnyouahaskell-zh/details) 这一本书讲解的知识浅显易懂，
无论是作者和还是翻译者们都是非常的用心，笔者在大一的时候即时第一次接触的函数式编程就是使用的这一本书，在gitbook上面提供免费阅读版本，无论是用于入门haskell还是得到一些关于函数式的直观的理解，这一本书都是的非常的有用。
2. [Real World Haskell](http://book.realworldhaskell.org/) 非常权威的资料, 这一本书可以在线免费阅读。
3. [官方快速入门文档](https://wiki.haskell.org/Learn_Haskell_in_10_minutes)
4. [learn x in y minutes](https://learnxinyminutes.com/docs/zh-cn/haskell-cn/) learn x in minutes 提供大量的关于的各种语言的教程，当然也是包括haskell的，从短短的几分钟时间可以看到这门语言的基础的概貌而不用在在一个大部头的序言上面的就开始打退堂鼓，在没有任何基础的情况下，显然是值得一看的。


## 到底是函数式还是命令式
在函数是编程中间，函数是一等公民，函数可以在任何地方定义，作为函数参数或者返回值
，也可以对于的函数进行组合，在面向对象中间，这一个被替换为对象，而在命令式中，这
变成了变量，或者称为内存单元。
**什么是函数式思维**, 函数式编程关心数据的映射，命令式编程关心解决问题的步骤, 函
数是一种特殊映射，函数式编程在于的的将数据进行变换，而命令式的思路却是贴近硬件的
，也就是在编译器，链接器等各种工具的作用下编程CPU可以理解话语，语言的都是人和机
器一个桥梁，有时候语言对于机器更加友好，比如汇编甚至机器码，有时候对于人更加友好
，比如函数式编程，在函数式的编程中间，人不在关心一个变量是如何从内存空间这里移动
到哪里，而是关心解决的问题是什么以及如何使用数学描述出来的。虽然面向对象的效率没
有命令式高，但是面向对象依旧大行其道，愿意恐怕也是便于设计，是人和机器交流的的折
中点。

## 后记
函数式编程最大也是的也是最关键的问题在于，函数式编程似乎从来没有从计算机体系结构
的角度思考过问题，这一类语言似乎强调自己在思维上面成功，而没有考虑过如何服务于的
的千家万户的日常生活，导致性能主要有两个:
一是，在实现早期的函数式编程语言时并没有考虑过效率问题，函数式的编程的语言的发明
者是MIT的教授，对于事物探索如果要求马上可以实用，往往会一劳无获的
但是如果是公司研发人员，也许函数式语言将会是另一种样貌，　或者函数式永远都是不会
发明出来的。
二是，面向函数式编程特性（如保证函数参数不变性等）的独特数据结构和算法，这些语言
的内部的要求导致函数式编程无法和当前一些大行其道的命令式编程相提并论。

## 参考资料
1. [函数式编程初探](http://www.ruanyifeng.com/blog/2012/04/functional_programming.html)
2. [functional_programming](https://en.wikipedia.org/wiki/Functional_programming)
3. [函数式和命令式不同](https://hcyue.me/2016/05/14/%E4%BB%80%E4%B9%88%E6%98%AF%E5%87%BD%E6%95%B0%E5%BC%8F%E7%BC%96%E7%A8%8B%E6%80%9D%E7%BB%B4/)
4. [why functional programming](https://stackoverflow.com/questions/36504/why-functional-languages)
