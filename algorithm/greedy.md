# Huffman Code
收集两个最大值， 然后将两个整合

## 寻找的连续的最大的求和
只要是当前的求和没有成为一个负数，那么就继续求和。
```c
/**
 * 算法思路:当发现local_sum求和小于0的时候，left指针右边移动
 */
int continuousMaxSum(vector<int> const & arr){
    int sum = 0;
    int right = 0;
    int localSum = 0;

    while(right < arr.size()){
        localSum += arr[right];
        sum = max(sum, localSum);
        if(localSum < 0){
            localSum = 0;
        }
        right++;
    }
    return sum;
}
```

## 安排课程表
按照结束的时间排序


类似问题有 452.用最少数量的箭引爆气球（中等）

##  跳跃游戏
leetcode 44 和 45 两个

1. 能否达到
2. 最少跳转步数如何达到

想要跳转到最远的位置，那么只需要跳到节点 X
并且保证 X 是这些候选人中最远的位置的。

# 分割数组为连续子序列
https://labuladong.gitee.io/algo/5/34/

将数组排序
1. 总是将新的元素加入现有的队列中间
2. 如果无法加入就
