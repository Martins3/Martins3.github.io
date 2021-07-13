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
