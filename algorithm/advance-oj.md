# Summary
http://www.pythontip.com/acm/problemCategory

1. [2-SAT](http://blog.sina.com.cn/s/blog_64675f540100k2xj.html)
2. [String](https://www.cnblogs.com/hate13/p/4622141.html)

# BinaryIndexTree
原理：
C[i] = sum{A[j] |  i - 2^k + 1 <= j <= i }
其中`k`为lowBit， 也就是二进制表示的低位连续的0的个数。

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
# KMP 算法
kmp处理pattern。
next数组记录的是：最长公共子前缀的长度
如何计算: 利用动态规划

# SegmentTree
lazy 修改。
1. 使用数组表示树
    1. buildTree 递归处理，开始时候不注入数值。
2. 查询
3. 修改

# lowest common ancestor
需要利用Union-Find
    1. 可以查询任意的一组中间
    2. dfs遍历全部的树

只有被遍历结束之后才会被合并。所以当处于不同的分支的时候，只有lca才被合并起来。


这是可以统计任意两个节点的.

节点 u 在访问完成其子节点 v 之后，包括 v 在内的所有节点都会认为自己的 ancestor 是 u,
u 作为其他点的最低点去访问其他的子节点。

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

# Disjoint Set
```txt
class DisjointSet{
public:
    int * arr;

    DisjointSet(int size){
        arr = new int[size + 1];
        memset(arr, 0, sizeof(int) * (size + 1));
    }

    int find(int x){
        if(arr[x]) return arr[x] = find(arr[x]);
        return x;
    }

    void union_pair(int x, int y){
        x = find(x);
        y = find(y);
        arr[y] = x;
    }
};
```
