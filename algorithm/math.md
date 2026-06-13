---
title: Math in OJ
date: 2018-06-10 21:58:55
tags: algorithm
---

> 收集数学常用的代码段
```
bool isPrime(int num){
  num = abs(num);
  if (num < 2) return false;
  if (num == 2) return true;
  if (num % 2 == 0) return false;
  for (int i = 3; i * i <= num; i += 2)
    if (num % i == 0) return false;
  return true;
}
```

```
int getPrimeNum(){
    // to-do
    使用欧拉筛法：非常消耗内存， 基本思想是一旦找到质数， 那么立刻清楚分解包含其
    的所有数值， 并且加以清除。
}
```
