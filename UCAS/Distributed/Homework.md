1.Eratosthene筛法计算素数的个数用openmp如何实现？(2分)

ANS: 实现思路，将范围内的整数划分为长度相同的区域，然后对于每一个区域并行的使用Eratosthene方法.
对于sqrt(to) 的所有的数值进行除法检查，如果可以整除那么标记为false。最后统计个数。
```cpp
// 参考实现 : https://github.com/stbrumme/eratosthenes
int eratosthenesOddSingleBlock(const Number from, const Number to)
{
  const Number memorySize = (to - from + 1);

  char* isPrime = new char[memorySize];
  for (Number i = 0; i < memorySize; i++)
    isPrime[i] = 1;

  for (Number i = 2; i*i <= to; i ++)
  {
    Number minJ = ((from+i-1)/i)*i;
    if (minJ < i*i)
      minJ = i*i;

    for (Number j = minJ; j <= to; j += i)
    {
      Number index = j - from;
      isPrime[index] = 0;
    }
  }

  int found = 0;
  for (Number i = 0; i < memorySize; i++)
    found += isPrime[i];

  if (from <= 2)
    found++;

  delete[] isPrime;
  return found;
}


int eratosthenesBlockwise(Number lastNumber, Number sliceSize, bool useOpenMP)
{
  omp_set_num_threads(useOpenMP ? omp_get_num_procs() : 1);

  int found = 0;
#pragma omp parallel for reduction(+:found)
  for (Number from = 2; from <= lastNumber; from += sliceSize)
  {
    Number to = from + sliceSize;
    if (to > lastNumber)
      to = lastNumber;

    found += eratosthenesOddSingleBlock(from + 1, to);
  }

  return found + 1;
}
```


2.讲义第24页所列Work stealing的不同任务steal方案（窃取一半vs窃取一个，大的任务粒度vs小的任务粒度）分别适合什么样的程序和什么样的系统？你能设计新的steal方法吗？(3分)

ANS:
* 窃取一半，导致传递消息的容量大，适合通信的并发度高的系统，或者通信可以流水线化，即为在整个消息没有传递的完成的时候，接收方已经可以使用
* 窃取一个，导致大量的窃取操作出现，适合通信的速率高的系统。
* 大的任务粒度，适合系统中间可提供worker 数目不多而任务量大的系统，因为如果每一个worker始终保持工作，那么粒度过大造成的worker空闲影响降低。
* 小的任务粒度，由于系统运行结束的时间取决于最慢的，所以尽可能希望所有的同时完成，所以对于实时系统，小粒度更合适。

新的steal方法:
每一个worker记录自己被偷窃的频率和全局的偷窃的比较，如果比较高，那么可以提高拒绝的概率。
