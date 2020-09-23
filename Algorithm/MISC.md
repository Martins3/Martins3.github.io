---
title: Amazing Algorithms
date: 2018-7-15 16:21:40
tags: Algorithm
---

# Summary
http://www.pythontip.com/acm/problemCategory
http://blog.csdn.net/a1dark/article/details/11714009

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

```
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

# LCA
需要利用Union-Find
    1. 可以查询任意的一组中间
    2. dfs遍历全部的树

只有被遍历结束之后才会被合并。所以当处于不同的分支的时候，只有lca才被合并起来。

```
Tarjan(u)//marge和find为并查集合并函数和查找函数
{
    for each(u,v)    //访问所有u子节点v
    {
        Tarjan(v);        //继续往下遍历
        marge(u,v);    //合并v到u上
        标记v被访问过;
    }
    for each(u,e)    //访问所有和u有询问关系的e
    {
        如果e被访问过;
        u,e的最近公共祖先为find(e);
    }
}
```
# Disjoint Set
```
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

