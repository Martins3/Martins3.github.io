---
title: dp
date: 2018-07-10 14:04:28
tags: algorithm
---
> 念念不忘 必有回响

# 概述

# RMQ算法

# DAG上的动态规划

# 多阶段决策问题

# 背包问题
## 01背包
给定 ：容量V，物体价值，物体容量
二维数组 : 循环的时候对于每一个物体都是分析放入和不放入的价值。
f[i][v] 中间 物品为 编号小于等于 i 个， 容量为v(olume)的时候的价值。
所以转移公式是： w 表示价值
f[i][v] = max(f[i - 1][v], f[i - 1][v - c[i] + w[i]])
该转移公式表示
    1. 初始化的二维数组的大小是 **[V  物体数目]**
    2. 对于i = 1的时候初始化。
    3. 所有的采用容量作为限制。

一维化的方法:

# 组合数计算
C(n, m) = C(n - 1, m) + C(n - 1, m - 1)
初始化：
1. 所有求解依赖与数组左上角的数值和左边的数值
2. n >= m 对角矩阵
所以首先初始化对角线 + 上方的位置

# 找零钱问题

```
// Every 
int table[5001];
class Solution {
public:
    int change(int amount, vector<int>& coins) {
        memset(table, 0, sizeof(table));
        table[0] = 1;
        for (unsigned i = 0; i < coins.size(); ++i) {
            for (int j = coins[i]; j <= amount; ++j) {
                table[j] += table[j - coins[i]];
            }
        }
        return table[amount];
    }
};
```

# longest common string

# [Longest increasing subsequence](https://en.wikipedia.org/wiki/Longest_increasing_subsequence) 
https://stackoverflow.com/questions/2631726/how-to-determine-the-longest-increasing-subsequence-using-dynamic-programming

how to find the actural subsequence ?

store the index instead of the real number.

make a `parent` array




# 题目
1. [The Triangle](http://poj.org/problem?id=1163) 理解初步
2. [Chores](http://poj.org/problem?id=1949) DAG

