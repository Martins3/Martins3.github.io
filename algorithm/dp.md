# 概述
- 总是，处理到第 i 个元素，可选的东西是什么 ?

- 归纳法 + 存储 (关键在于证明归纳法)

- 题目要求解的内容就是目标，然后对于子串、子数组的求解内容来组装出来当前内容
## RMQ算法

## DAG上的动态规划

## 多阶段决策问题

# 背包问题
#### 01背包
给定 ：容量V，物体价值，物体容量
二维数组 : 循环的时候对于每一个物体都是分析放入和不放入的价值。
`f[i][v]` 中间 物品为 编号小于等于 i 个， 容量为v(olume)的时候的价值。
所以转移公式是： w 表示价值
`f[i][v] = max(f[i - 1][v], f[i - 1][v - c[i] + w[i]])`
该转移公式表示
    1. 初始化的二维数组的大小是 **[V  物体数目]**
    2. 对于i = 1的时候初始化。
    3. 所有的采用容量作为限制。

一维化的方法:

## 组合数计算
C(n, m) = C(n - 1, m) + C(n - 1, m - 1)
初始化：
1. 所有求解依赖与数组左上角的数值和左边的数值
2. n >= m 对角矩阵
所以首先初始化对角线 + 上方的位置

## 518.找零钱问题 1 : 所有零钱的组合可能性
错误解法:
dp[i] : 当钱数为 i 的时候，所有零钱的组合可能性
best : i + 1 的可能性是前面所有添加一个零钱求和
```c
class Solution2 {
public:
  int change(int amount, vector<int> &coins) {
    memset(table, 0, sizeof(table));

    table[0] = 1;
    for (int j = 1; j <= amount; ++j) {
      for (unsigned i = 0; i < coins.size(); ++i) {
        if (j - coins[i] >= 0)
          table[j] += table[j - coins[i]];
      }
    }
    return table[amount];
  }
};
```
这让零钱的添加存在了顺序

实际上，这是一个背包问题啊!

这个问题的正确思考方法就是:
对于这些 coins,

dp[i][j] : 完成前面 i 个 coin 的选择，装备容量为 j 的的种类
转移函数 : `dp[i][j] = dp[i - 1][j] + dp[i][j - coins[i]]`


## 322.找零钱问题 2 : 找零钱使用的硬币数量
dp[i] = 当钱数为 i 的需要的硬币数量

best = i + 1 的最优解必然是之前的最优解增加一个导致的，所以，根据硬币的面值，找到前面的数值

## longest common string

## 300.最长上升子序列
dp[i] 记录的内容，当 i 作为序列终点的时候，当前的最长上升子序列

转移: dp[i] 对于前面的每一个数值比较，对于那些 arr[i] > arr[j], 就是接上了

[Longest increasing subsequence](https://en.wikipedia.org/wiki/Longest_increasing_subsequence)
https://stackoverflow.com/questions/2631726/how-to-determine-the-longest-increasing-subsequence-using-dynamic-programming

how to find the actural subsequence ?

store the index instead of the real number.

make a `parent` array

## 354.嵌套信封问题
关键问题是，如何装换为 最长上升子序列

https://labuladong.gitee.io/algo/3/24/65/ 给出来的解释，这个转换非常巧妙

## 53.最大子序和
只要累计不是 0 就继续，中途记录最大值

## 1143.最长公共子序列
dp[i][j] : 表示 [0, i] 和 [0, j] 的公共的长度
转移函数 : 如果 str1[i] 等于 str2[j]，那么 dp[i][j] = dp[i - 1][j - 1], 否则
dp[i][j] = max(dp[i][j - 1], dp[i - 1][j]), 因为两者不相等，子串必然不可能同时包含两者

## 583.两个字符串的删除操作的最小，让他们相等
dp[i][j] : 表示 [0, i] 和 [0, j] 的删除数量
转移函数 : 如果 str1[i] 等于 str2[j]，那么 dp[i][j] = dp[i - 1][j - 1], 否则
dp[i][j] = min(dp[i][j - 1], dp[i - 1][j]) + 1

## 712.两个字符串的最小ASCII删除和
dp[i][j] : 表示 [0, i] 和 [0, j] 的最小 ASCII 删除
转移函数: 和 583 的差别在于，之前是删除数值谁更小，现在是比较谁的删除量更
如果 str1[i] 不等于 str2[j] 的时候，最小的编辑量从 str1 或者 str2 中必然要删除一个数值。

**关于子串的这种问题，一个小技巧，dp[i][j] 表示长度而不是下标，这样边界条件很容易处理**

## 516.最长回文子序列
dp[i][j] : 范围在 i j 之间的最长回文子序列

## 416.分割等和子集
使用背包的思路

## 312.戳气球
太狡猾了，居然是采用使用最后一个来作为分割点

# 题目
1. [The Triangle](http://poj.org/problem?id=1163) 理解初步
2. [Chores](http://poj.org/problem?id=1949) DAG

## 别人的总结
- https://github.com/ninechapter-algorithm/linghu-algorithm-templete/blob/master/%E7%AE%97%E6%B3%95%E4%B8%8E%E6%95%B0%E6%8D%AE%E7%BB%93%E6%9E%84/%E5%8A%A8%E6%80%81%E8%A7%84%E5%88%92%E5%8D%81%E9%97%AE%E5%8D%81%E7%AD%94.md
