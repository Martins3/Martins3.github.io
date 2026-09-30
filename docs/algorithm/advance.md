# Summary

http://www.pythontip.com/acm/problemCategory

1. [2-SAT](http://blog.sina.com.cn/s/blog_64675f540100k2xj.html)
2. [String](https://www.cnblogs.com/hate13/p/4622141.html)

## 综合教程
- https://github.com/labuladong/fucking-algorithm : 不要好高骛远，先吧这里的东西一个个的专题搞懂就可以了
  - 类似的书籍仓库
    - https://github.com/geekxh/hello-algorithm
- https://github.com/hzwer/shareOI : fucking 看完之后，可以开始分析这一个，作为进阶
- https://github.com/OI-wiki/OI-wiki
- https://github.com/julycoding/The-Art-Of-Programming-By-July/blob/master/ebook/zh/05.03.md : 卖书的，可以稍稍吧

## 树状数组
- 讲解了一个图，所以，整个过程是很清晰的，关于下标，注意数值是从 1 开始
    - https://www.cnblogs.com/xenny/p/9739600.html

## when boring
- https://github.com/jamesroutley/write-a-hash-table : 很短的教程，讲解 hash table
- https://github.com/mrpandey/d3graphTheory : 制作的非常精美，其中还是存在很多东西还是不知道的

## misc
- https://github.com/aoapc-book/aoapc-bac2nd : 刘汝佳的书

## blog
如何更好地理解和掌握 KMP 算法? - 阮行止的回答 - 知乎
https://www.zhihu.com/question/21923021/answer/1032665486

## 使用这个复习后缀数组
- https://visualgo.net/zh

## BinaryIndexTree

原理： C[i] = sum{A[j] | i - 2^k + 1 <= j <= i } 其中`k`为lowBit，
也就是二进制表示的低位连续的0的个数。

求和的含义:
由于每一个位置上面都是，每一个数值上求和是从该点到开始，每一个点控制求和为
lowBit

add的含义：找到所有包含的了此位置的数值，然后加上v 就可以了

注意: 第一个位置被空出来了的。

```c
class BinaryIndexTree{
private:
    std::vector<int> arr;
    int lowBit(int x){
        return (x) & (-x);
    }

public:
    int sum(int x){
        int ans = 0;
        while(x != 0){
            ans += arr[x];
            x -= lowBit(x);
        }
        return ans;
    }

    void add(int x, int v){
        for(int i = x ; i < arr.size(); i += lowBit(i)){
            arr[i] += v;
        }
    }

    BinaryIndexTree(int size): arr(size + 1){}
};
```

## KMP 算法

kmp处理pattern。 next数组记录的是：最长公共子前缀的长度 如何计算: 利用动态规划

## SegmentTree

lazy 修改。

1. 使用数组表示树
   1. buildTree 递归处理，开始时候不注入数值。
2. 查询
3. 修改

## lowest common ancestor

需要利用Union-Find 1. 可以查询任意的一组中间 2. dfs遍历全部的树

只有被遍历结束之后才会被合并。所以当处于不同的分支的时候，只有lca才被合并起来。

这是可以统计任意两个节点的.

节点 u 在访问完成其子节点 v 之后，包括 v 在内的所有节点都会认为自己的 ancestor
是 u, u 作为其他点的最低点去访问其他的子节点。

```txt
function TarjanOLCA(u) is
    MakeSet(u)
    u.ancestor := u
    for each v in u.children do
        TarjanOLCA(v)
        Union(u, v)
        Find(u).ancestor := u //
    u.color := black
    for each v such that {u, v} in P do
        if v.color == black then
            print "Tarjan's Lowest Common Ancestor of " + u +
                  " and " + v + " is " + Find(v).ancestor + "."
```

```txt
function MakeSet(x) is
    x.parent := x
    x.rank   := 1

function Union(x, y) is
    xRoot := Find(x)
    yRoot := Find(y)
    if xRoot.rank > yRoot.rank then
        yRoot.parent := xRoot
    else if xRoot.rank < yRoot.rank then
        xRoot.parent := yRoot
    else if xRoot.rank == yRoot.rank then
        yRoot.parent := xRoot
        xRoot.rank := xRoot.rank + 1

function Find(x) is
    if x.parent != x then
       x.parent := Find(x.parent)
    return x.parent
```

[算法的写法](https://en.wikipedia.org/wiki/Tarjan%27s_off-line_lowest_common_ancestors_algorithm)

[算法描述](https://stackoverflow.com/questions/19262341/tarjans-lowest-common-ancestor-algorithm-explanation)


如果是 Java 选手，可以从 [Algorithm](https://algs4.cs.princeton.edu/home/) 这本书入手

[各大 OJ 分类](http://www.pythontip.com/acm/problemCategory)


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
