1. C 的所有输入函数
    2. printf 读取整数　字符串的策略是什么

    ```
    1. 整数中间包含字母是不是自动略过去
    ```

2. C 实现重定向操作

3. 看一看math的头文件中间的函数

4. scanf 遇到无法处理的符号无法自动跳跃过去
    1. 比如　在读入整数的时候, '.' 就是无法处理的符号


// 需要处理
5. 复杂的巅峰
    1. quick sort
    2. heap
    3. stack queue

```
#include <stdio.h>
#include <stdlib.h>
int comp (const void * elem1, const void * elem2)
{
    int f = *((int*)elem1);
    int s = *((int*)elem2);
    if (f > s) return  1;
    if (f < s) return -1;
    return 0;
}
int main(int argc, char* argv[])
{
    int x[] = {4,5,2,3,1,0,9,8,6,7};
    qsort (x, sizeof(x)/sizeof(*x), sizeof(*x), comp);

    for (int i = 0 ; i < 10 ; i++)
        printf ("%d ", x[i]);
    return 0;
}
```

6. 时间函数

7. qsort指针的含义是移动指针，那么对于可以对于数组排序吗?
    1. 并不是，由于是对于qsort含有元素大小参数，所以可以对于数组排序，但是有点划不算

6. Euler 路径似乎依旧不知道如何处理

7. 使用stack 处理　逆波兰式
