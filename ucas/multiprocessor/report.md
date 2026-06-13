# 用于列车售票的可线性化并发数据结构


## 1 设计思路
本课程阐述的思想无疑是开创性，对于提供了一个新的思路来处理并发程序的设计，那就是可不可以不使用lock来实现，但是遗憾的是，当程序运行在一个现代的成熟的操作系统上的时候，这些并不能带来明显的优势。

通过分析性能测试的数据，我们发现读数量远远大于更新的数量，这让我们优化重点放在读操作。

## 1.1 为什么 wait-free 和 lock-free 并不能改善性能

wait-free 和 lock-free 带来的实质性的改变到底是什么? 其关键的影响在于，在利用lock实现并发控制的时候，当一个线程持有了互斥锁之后，然后由于某些原因，始终不释放锁，那么其他线程将永远无法进入到Critical Section中间，但是对于lock-free 和 wait-free 的进程不会发生这种事情，利用CAS等原子操作，不同的线程之间进程，当一个线程竞争成功之后，无论其之后运行情况如何，其他线程都是可以继续运行的。


使用如下程序作为测试了样本，分别使用lock 和 cas 来对于计数器进行累加，然后分别统计数值为 10000 100000 1000000 10000000 三种累加的方法的结果。

```java
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.locks.ReentrantLock;

/**
 * CompareLock
 */
public class CompareLock {

  // Initially value as 0
  AtomicLong counter;
  long c;

  private ReentrantLock reentrantLock;

  CompareLock() {
    counter = new AtomicLong(0);
    reentrantLock = new ReentrantLock();
  }

  synchronized void reentrantLock() {
    // reentrantLock.lock();
    c++;
    // reentrantLock.unlock();
  }

  void cas() {
    boolean succ = false;
    while (!succ) {
      long m = counter.get();
      succ = counter.compareAndSet(m, m + 1);
    }
  }

  public static void main(String[] args) throws InterruptedException{


    int threadnum = 8;
    long loop = 10000;
    final CompareLock c = new CompareLock();
    Thread[] threads = new Thread[threadnum];
    long start = System.currentTimeMillis();
    for (int i = 0; i < threadnum; i++) {
      threads[i] = new Thread(new Runnable() {
        public void run() {
          for (int i = 0; i < loop; i++) {
            c.reentrantLock();
          }
        }
      });
      threads[i].start();
    }

    for (int i = 0; i < threadnum; i++) {
      threads[i].join();
    }

    System.out.println("the accumulate : " + c.c);
    System.out.println(System.currentTimeMillis() - start);
    System.out.flush();

    start = System.currentTimeMillis();
    for (int i = 0; i < threadnum; i++) {
      threads[i] = new Thread(new Runnable() {
        public void run() {
          for (int i = 0; i < loop; i++) {
            c.cas();
          }
        }
      });
      threads[i].start();
    }
    for (int i = 0; i < threadnum; i++) {
      threads[i].join();
    }
    System.out.println("the accumulate : " + c.counter.get());
    System.out.println(System.currentTimeMillis() - start);

    start = System.currentTimeMillis();
    for (int i = 0; i < threadnum; i++) {
      threads[i] = new Thread(new Runnable() {
        public void run() {
          for (int i = 0; i < loop; i++) {
            // c.cas();
          }
        }
      });
      threads[i].start();
    }
    for (int i = 0; i < threadnum; i++) {
      threads[i].join();
    }
    System.out.println("Overhead ");
    System.out.println(System.currentTimeMillis() - start);
  }

}
```
| Iteration Number | Lock based | Lock-free | Overhead |
|------------------|------------|-----------|----------|
| 10000            | 66         | 20        | 3        |
| 100000           | 132        | 131       | 18       |
| 1000000          | 884        | 1136      | 19       |
| 10000000         | 14888      | 11849     | 43       |

首先，对于CS仅仅含有一刀两条指令而言，使用lock 是不公平，因为每一次进入和退出，都需要对于锁进行一次访问，随着CS中间指令数量增加，这种开销将会被分摊开来的，而lock-free 需要对于每一条指令进行CAS循环测试。其次，即便如此，两者依旧没有显著性差异。

正如The art of Multiprocessor Programming 中间的所说:

> There are so many variants... tagged pointers, epoch based reclamation, RCU/quiescent state, hazard pointers, general process-wide garbage collection, etc. All these strategies have performance implications, and some also place restrictions on how your application in general can be designed.

> If preemption in the middle of a critical section is sufficiently rare, then dependent blocking progress conditions are effectively indistinguishable from their nonblocking counterparts. If preemp�tion is common enough to cause concern, or if the cost of preemption-based delay is sufficiently high, then it is sensible to consider nonblocking progress conditions.

最后，从个人的编程经验而言，lock-free wait-free 的编程更加容易出错，而且理论验证更加复杂，所以本次试验，没有必要使用.

## 1.2 如何加快经常性事件 ?

> 修改一下。 性能测试负载为 20个车次，每个车次15节车厢，每节车厢100个座位，途经10个车站。共有96个线程并发执行，各个线程执行500000次操作，其中查询、购票和退票操作的比例是80 : 15 : 5。

1. 读的数量比写的数量更多。
2. 由于不同的车次之间是没有任何影响的，所以实际上多个进程竞争同一个数据平均为 `96/20`。既然如此，上锁的影响就从影响其他的96 - 1 个线程变成了 4 个线程。
3. 96 / 20 * 500000 * (15 - 5)% 相当于一共存在对于每一个车次的 50000 次购票操作，但是其中仅仅只有 1500 张票可以提供的，说明其中大多数购票操作只有读操作，然后立刻返回的。

经过以上三点的分析，我们可以得出结论，那就是查询指定区间的票据的操作需要尽可能块的返回，所以其对应的数据结构也是清晰的，那就是使用一个二维数组存储任意区间的票据剩余量，当退票和购票的时候更新整个二维数组。

## 1.3 为什么细粒度锁并不能带来性能的提升
显然，细粒度锁在一般情况下是可以提高性能的，但是在1.2节的分析，我们可以知道，需要保持原子操作的位置购票和退票中间的时候，需要对于二维数组进行更新，实现细粒度锁的方法就是对于需要更新的节点进行加锁，而不是对于整个数组上锁。

不采用这种细粒度锁的原因如下:
1. contension 平均值不高，正如前面分析的只有4.8
2. 每次需要更新数量至少为 10 ，但是该情况是仅仅购买一张从m 到 m + 1的票据，相对于容量为仅仅包含数个数值的TreeSet的查询，上锁和释放锁开销不容忽视。或者说，这种粒度的锁和overhead不容忽视，然而构建其他粒度的锁可能性不打，因为最粗和最细两者之间的差别不大。
3. 正如之前所有提到的，并发数据结构的核心在于正确性验证，粗粒度大大的减轻其中的难度。

## 2 正确性分析

## 2.1 串行的正确性验证
由于使用二维数组存储，其中(i,j)位置的元素含义是 从i到j中间剩余的车票。

购票逻辑: 购买(i,j)的票据，那么任何和其含有交集的区间都是需要清理该编号的车票的。

退票逻辑: 检查该位置在各个节点剩余量，然后重新配置该票据在各个区间的存在状态。

以上两个锁都是添加粗粒度锁，由于查询只需要保持静默一致性，所以不需要添加任何锁，值得注意的是，购票首先需要查询，其查询之时已经需要被锁保护。

由于程序的正确性验证缺乏系统统一的方法，现代程序一般都有通过测试验证的，本程序的串行执行连续被官方提供的测试程序运行了100次，均没有出现错误，所以可以认为本程序的正确性很高。

## 2.2 并行的正确性验证
由于使用的粗粒度锁，所以串行的正确性显然说明了并行的正确性。

## 3 性能评测
由于不同的处理器，处理器其他的负载状态会影响吞吐量的具体数值，此处，有一种不依赖于具体机器配置的比较方法，将所有的操作全部切换为查询操作，由于查询是一个指令完成的，而且是完全并行的，以此为基础，测量正常的性能测试。

| Basement | Benchmark |
|----------|-----------|
| 12       | 26        |
| 11       | 26        |
| 12       | 24        |

注意到购票的操作的复杂度和退票的复杂度远高于查询，所以相对于基准测试(只有查询的情况)，真正的性能测试效果非常不错。
