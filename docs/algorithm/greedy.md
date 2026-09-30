## Huffman Code
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


## 跳跃游戏
leetcode 44 和 45 两个

1. 能否达到
2. 最少跳转步数如何达到

想要跳转到最远的位置，那么只需要跳到节点 X
并且保证 X 是这些候选人中最远的位置的。

## 659. 分割数组为连续子序列

将数组排序
1. 总是将新的元素加入现有的队列中间
2. 如果无法加入就

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
