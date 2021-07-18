# 快排
- 不要老是想着骚操作，使用滑动窗口的想法，i 左侧都是 <= 的，i j 是大于的，j 右侧是需要扫描的

```c
void quick_sort(vector<int> &sort, const int a, const int b) {
  if (a >= b - 1)
    return; // 右边不包括

  const int flag = sort[a];
  // 左边都是小于等于，右边是大于的
  // 问题: 1. 如果所有的数值都是小于等于的，如何 ? 简化处理，至少保持一个数值

  int l = a + 1;
  int r = b;

  // 其实是划分为三个部分

  while (true) {

    for (; l < r; ++l) {
      if (sort[l] > flag)
        break;
    }
    if (l == r)
      break; // 所有人都是小于我, 后面不用看了

    for (; r > l; --r) {
      if (sort[r] <= flag)
        break;
    }

    if (l == r)
      break;

    int tmp = sort[l];
    sort[l] = sort[r];
    sort[r] = tmp;
  }

  // 找到将 pivot 插入的位置, 找到第一个函数
  int flag_pos = a;
  for (int i = a; i < b; ++i) {
    if (sort[i] < flag) {
      flag_pos = i;
    }
  }

  int tmp = sort[flag_pos];
  sort[flag_pos] = sort[a];
  sort[a] = tmp;
  printf("[%d - %d] %d\n", a, b, flag_pos);

  assert(l == r);

  quick_sort(sort, a, flag_pos);
  quick_sort(sort, flag_pos + 1, b);
}

// 还是这个最简单了，滑动，找到中间的数值
/*
 algorithm quicksort(A, lo, hi) is
    if lo < hi then
        p := partition(A, lo, hi)
        quicksort(A, lo, p - 1)
        quicksort(A, p + 1, hi)

algorithm partition(A, lo, hi) is
    pivot := A[hi]
    i := lo
    for j := lo to hi do
        if A[j] < pivot then
            swap A[i] with A[j]
            i := i + 1
    swap A[i] with A[hi]
    return i
 */
```

## heap sort
1. 使用数组
```
Arr[(i-1)/2]	Returns the parent node
Arr[(2*i)+1]	Returns the left child node
Arr[(2*i)+2]	Returns the right child node
```
2. 增加，从尾端增加，逐步向上
3. 删除，将尾端的放到最上面，逐步向下, 超出范围的就不是 valid


## 单调栈
其实专门用于分析 Next Greater Number 的

对于一个数组 arr = [2, 3, 4, 1, 8] 反方向 push stack
放入的元素 e 和 stack.pop() 比较:
1. e > stack.pop(), 一路 pop 直到可以
2. 如果小鱼，那么这就是 e 的 Next Greater Number
