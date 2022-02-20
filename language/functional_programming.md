## SML
### 测试一下SML
SML中间含有list类型，基本原理是链表，而C++的STL库文件含有一个stack
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
```cpp
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

## 参考资料
1. [函数式编程初探](http://www.ruanyifeng.com/blog/2012/04/functional_programming.html)
2. [functional_programming](https://en.wikipedia.org/wiki/Functional_programming)
3. [函数式和命令式不同](https://hcyue.me/2016/05/14/%E4%BB%80%E4%B9%88%E6%98%AF%E5%87%BD%E6%95%B0%E5%BC%8F%E7%BC%96%E7%A8%8B%E6%80%9D%E7%BB%B4/)
4. [why functional programming](https://stackoverflow.com/questions/36504/why-functional-languages)

## 文摘
- https://ycombinator.chibicode.com/functional-programming-emojis
  - 先去看看 Church encoding 和 Y combinator 然后再去看看这个文章的解释，搞一堆 emoji，信息量非常低
- [An Introduction to Type-Level Programming](https://rebeccaskinner.net/posts/2021-08-25-introduction-to-type-level-programming.html)
