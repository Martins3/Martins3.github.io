# homework

## 1
For each of the following, state whether it is a safety or liveness property. Identify the bad or good thing of interest.
1. Patrons are served in the order they arrive.
2. What goes up must come down.
3. If two or more processes are waiting to enter their critical sections, at least one succeeds.
4. If an interrupt occurs, then a message is printed within one second.
5. If an interrupt occurs, then a message is printed.
6. The cost of living never decreases.
7. Two things are certain: death and taxes.
8. You can always tell a Harvard man.

| state                                                                                         | property |
|-----------------------------------------------------------------------------------------------|----------|
| Patrons are served in the order they arrive.                                                  | safety   |
| What goes up must come down.                                                                  | liveness |
| If two or more processes are waiting to enter their critical sections, at least one succeeds. | liveness |
| If an interrupt occurs, then a message is printed within one second.                          | safety   |
| If an interrupt occurs, then a message is printed.                                            | liveness |
| The cost of living never decreases.                                                           | safety   |
| Two things are certain: death and taxes.                                                      | liveness |
| You can always tell a Harvard man.                                                            | safety   |

## 2
1.

Show that the `Filter` lock allows some threads to overtake others an arbitrary number of times.

假设对于Filter 算法处理thread 数目n = 2 分析，下面说明存在一种情况，线程0 可以不停进入，而线程1 无法进入，随着时间推移，
线程0进入的次数将会比线程1 进入的次数多任意次。
此时算法Filter 退化为 Bakery 算法 :

```java
public void lock() {
 flag[j] = true;
 victim  = j; 
 while (flag[i] && victim == j) {};
}
```

对于这一种情况，当线程1 flag 设置为 false 之后不在执行，线程 0 可以执行任意次数。

2.

In practice, almost all lock acquisitions are uncontended, so the most
practical measure of a lock’s performance is the number of steps needed for a
thread to acquire a lock when no other thread is concurrently trying to acquire
the lock.

Scientists at Cantaloupe-Melon University have devised the following “wrapper” for an arbitrary lock, shown in Fig. 2.16. They claim that if the base Lock
class provides mutual exclusion and is starvation-free, so does the **FastPath** lock,
but it can be acquired in a constant number of steps in the absence of contention.
Sketch an argument why they are right, or give a counter example.
```java
class FastPath implements Lock {
  private static ThreadLocal<Integer> myIndex;
  private Lock lock;
  private int x, y = -1;
  public void lock() {
    int i = myIndex.get();
    x = i; // I’m here
    while (y != -1) {} // is the lock free?
    y = i; // me again?
    if (x != i) // Am I still here?
      lock.lock(); // slow path
  }
  public void unlock() {
    y = -1;
    lock.unlock();
  }
}
```

并不能阻止两个线层同时进入到CS(critical section), 实现的方式如下:
| step | A                      | B                      |
|------|------------------------|------------------------|
| 0    | int i = myIndex.get(); |                        |
| 1    | x = i;                 |                        |
| 2    |                        | int i = myIndex.get(); |
| 3    |                        | x = i;                 |
| 4    | while (y != -1) {}     |                        |
| 5    |                        | while (y != -1) {}     |
| 6    | y = i;                 |                        |
| 7    |                        | y = i;                 |
| 8    | if (x != i)            |                        |
| 9    |                        | if (x != i)            |
| 10   |                        | enter the CS           |
| 11   | lock.lock();           |                        |
| 12   | enter CS               |                        |

## 3
1. Exercise 29. Is the following property equivalent to saying that object x is wait-free ?
> For every infinite history H of x, every thread that takes an infinite number of steps in H completes an infinite number of method calls.

Ans: 不能，因为可能存在一个执行了无穷多步的thread首先执行调用的函数都是在有限步内返回，但是最后一次调用执行了无数多步没有返回，而wait-free要求所有的调用都是有限步返回。

2. Exercise 30. Is the following property equivalent to saying that object x is lockfree?
> For every infinite history H of x, an infinite number of method calls are completed.

Ans: 可以保证，既然存在无穷多的发方法调用返回，这些方法必定都是有限步骤返回，而lock-free只需要保证存在有的函数调用有限步骤返回即可。

3. Exercise 32. This exercise examines a queue implementation (Fig. 3.17) whose
enq() method does not have a linearization point.
The queue stores its items in an items array, which for simplicity we will
assume is unbounded. The tail field is an AtomicInteger, initially zero. The
enq() method reserves a slot by incrementing tail, and then stores the item at
that location. Note that these two steps are not atomic: there is an interval after
tail has been incremented but before the item has been stored in the array.
The deq() method reads the value of tail, and then traverses the array in
ascending order from slot zero to the tail. For each slot, it swaps null with the
current contents, returning the first non-null item it finds. If all slots are null, the
procedure is restarted.
Give an example execution showing that the linearization point for enq() cannot occur at Line 15.
Hint: give an execution where two enq() calls are not linearized in the order
they execute Line 15.

```java
 public class HWQueue<T> {
 AtomicReference<T>[] items;
 AtomicInteger tail;
 static final int CAPACITY = 1024;

 public HWQueue() {
 items =(AtomicReference<T>[])Array.newInstance(AtomicReference.class,
 CAPACITY);
 for (int i = 0; i < items.length; i++) {
 items[i] = new AtomicReference<T>(null);
 }
 tail = new AtomicInteger(0);
 }
 public void enq(T x) {
 int i = tail.getAndIncrement();
 items[i].set(x);
 }
 public T deq() {
 while (true) {
 int range = tail.get();
 for (int i = 0; i < range; i++) {
 T value = items[i].getAndSet(null);
 if (value != null) {
 return value;
 }
 }
 }
 }
 }
```

Ans: 
1. 假设thread A 首先执行line 15，并且获取局部变量i = 0, 紧接着thread B 执行line 15, 获取局部变量 i = 1, 然后赋值b，此时thread C 调用deq() 将会得到 b, 然后thread A 执行 line 16 写入 a, 最后 thread C 执行deq() 得到 b.
由此可以看到，虽然A 首先执行line 15, 但是thread C deq() 首先得到是b
2. 假设thread A 首先执行line 15，并且获取局部变量i = 0, 紧接着thread B 执行line 15, 获取局部变量 i = 1,
然后执行line 15, 赋值b，之后thread A 执行line 16 赋值 a，如果thread C 执行deq() 将会依次得到a b 
3. 不能说明


## 4
Exercise 36. Consider the atomic MRSW construction shown in Fig. 4.12. True
or false: if we replace the atomic SRSW registers with regular SRSW registers,
then the construction still yields an atomic MRSW register. Justify your answer

False : 依旧使用Fig.4.12对应例子，当write 写入到第二个行的停止，如下所示
| t + 1 | t     | t | t |
| t     | t + 1 | t | t |
| t     | t     | t | t |
| t     | t     | t | t |

此时 thread 2 遍历第二列，假设其读到新值，返回t + 1，读完之后其结果为
| t + 1 | t     | t     | t     |
| t + 1 | t + 1 | t + 1 | t + 1 |
| t     | t     | t     | t     |
| t     | t     | t     | t     |

然后thread 3 遍历第三列，由于是regular 的原因，其读到位于(2,3)处的旧值，最后返回 t

综合以上内容，说明通过Regular SRSW 无法构造出来 Atomic MRSW 来。


Exercise 42. You learn that your competitor, the Acme Atomic Register Company, has developed a way to use Boolean (single-bit) atomic registers to construct
an efficient write-once single-reader single-writer atomic register. Through your
spies, you acquire the code fragment shown in Fig. 4.23, which is unfortunately
missing the code for read(). Your job is to devise a read() method that works for
this class, and to justify (informally) why it works. (Remember that the register
is write-once, meaning that your read will overlap at most one write.)
```java
class AcmeRegister implements Register{
// N is the length of the register
// Atomic multi-reader single-writer registers
private BoolRegister[] b = new BoolMRSWRegister[3 * N];
  public void write(int x) {
      boolean[] v = intToBooleanArray(x);
      // copy v[i] to b[i] in ascending order of i
      for (int i = 0; i < N; i++)
       b[i].write(v[i]);
       // copy v[i] to b[N+i] in ascending order of i
       for (int i = 0; i < N; i++)
        b[N+i].write(v[i]);
       // copy v[i] to b[2N+i] in ascending order of i
       for (int i = 0; i < N; i++)
        b[(2*N)+i].write(v[i]);
  }

  public int read() {
  // missing code
  }
}
```

答:

```java
  public int read() {
      int res_1;
      for (int i = 0; i < N; i++)
        if (b[i].read()){
          res_1 = i;
        }

      int res_2;
      for (int i = 0; i < N; i++)
        if (b[i].read()){
          res_1 = i;
        }

      int res_3;
      for (int i = 0; i < N; i++)
        if (b[i].read()){
          res_3 = i;
        }

      if(res_1 == res_2)
          return res_1;
      }

      if(res_2 == res_3)
          return res_2;
      }
      
      return -1;
  }
```

当read 仅仅和一个write 重复的时候，最多只会含有一个数组分区的数值出现处于被两个write改动，
总是可以保证两个数值是相同的，所以当三个数组分区中间读到的数值都是不同的时候，那么就是出现多余一个write 重叠。

## 5
1. Exercise 53. The Stack class provides two methods: push(x) pushes a value onto
the top of the stack, and pop() removes and returns the most recently pushed
value. Prove that the Stack class has consensus number exactly two.

ANS: 证明Stack其consensus number 恰好为2，其方法和Qeueu类似。
首先，我们构造如下Stack 类来证明 Stack 的consensus number至少为2.
```java
public class StackConsensus<T> extends ConsensusProtocol<T> {
  private static final int WIN = 0; // first thread
  private static final int LOSE = 1; // second thread
  Queue queue;
  // initialize queue with two items
  public QueueConsensus() {
    queue = new Queue();
    queue.enq(WIN);
    queue.enq(LOSE);
  }
 // figure out which thread was first
 public T decide(T Value) {
   propose(value);
   int status = queue.deq();
   int i = ThreadID.get();
   if (status == WIN)
    return proposed[i];
   else
    return proposed[1-i];
 }
}
```
以上构造可以提供当一个thread调用decide的时候，获胜者可以返回自己的输入，而失败者返回其他者的发输入.

下面证明其 consensus number 最多为2.
使用反证法证明，假设存在一个处理3个thread A, B, C 的 consensus 协议，显然该协议中间含有cirtical region，假设为s.
假设状态s1表示如下执行的状态:
1. thread A 和 thread B enqueue 元素a b
2. 运行A直到其 dequeue 元素a
3. 在A进一步运行之前，B dequeue 元素b

假设状态s2表示如下执行的状态:
1. thread A 和 thread B enqueue 元素a b
2. 运行A直到其 dequeue 元素b
3. 在A进一步运行之前，B dequeue 元素a

s1是0-价的, s2是1-价的，对于C而言，其s1和s2是区别的。






2. Exercise 54. Suppose we augment the FIFO Queue class with a peek() method
that returns but does not remove the first element in the queue. Show that the
augmented queue has infinite consensus number.

ANS: 证明类似于5.8.1 形成如下构造内容:
```java
class QueueConsensus extends ConsensusProtocol {
  private final int FIRST = -1;
  private AtomicInteger r = new AtomicInteger(FIRST);
  public Object decide(Object value) {
    propose(value);
    int i = ThreadID.get();
    if (r.compareAndSet(FIRST, i)) // I won
      return proposed[i];
    else // I lost
     return proposed[r.get()];
  }
}
```
当任意整数n个thread，该构造通过依赖于 AtomicInteger 的 compareAndSet 函数来确定使用哪一个线程来使用其输入。



3. Exercise 68. `Fig. 5.17` shows a FIFO queue implemented with read, write, getAndSet() (that is, swap) and getAndIncrement() methods. You may assume
this queue is linearizable, and wait-free as long as deq() is never applied to an
empty queue. Consider the following sequence of statements.
    * Both getAndSet() and getAndIncrement() methods have consensus number 2.
    * We can add a peek() simply by taking a snapshot of the queue (using the
methods studied earlier in the course) and returning the item at the head of
the queue.
    * Using the protocol devised for Exercise 54, we can use the resulting queue to
solve n-consensus for any n.
We have just constructed an n-thread consensus protocol using only objects with
consensus number 2. Identify the faulty step in this chain of reasoning, and
explain what went wrong


4. Exercise 78. In both the lock-free and wait-free universal constructions, the
sequence number of the sentinel node at the tail of the list is initially set to 1.
Which of these algorithms, if any, would cease to work correctly if the sentinel
node’s sequence number was initially set to 0?
ANS: lock-free universal construction

5. Exercise 80. In the construction shown here, each thread first looks for another
thread to help, and then tries to to append its own node.
Suppose instead, each thread first tries to append its own node, and then tries
to help the other thread. Explain whether this alternative approach works. Justify
your answer.
ANS: 不能，将会导致共识对象的重用。



## 6
Exercise 85. Fig. 7.33 shows an alternative implementation of CLHLock in which
a thread reuses its own node instead of its predecessor node. Explain how this
implementation can go wrong.

```java
public class BadCLHLock implements Lock {
// most recent lock holder
AtomicReference<Qnode> tail = new Qnode();
// thread-local variable
ThreadLocal<Qnode> myNode;
  public void lock() {
    Qnode qnode = myNode.get();
    qnode.locked = true; // I’m not done
    // Make me the new tail, and find my predecessor
    Qnode pred = tail.getAndSet(qnode);
    // spin while predecessor holds lock
    while (pred.locked) {}
  }

  public void unlock() {
    // reuse my node next time
    myNode.get().locked = false;
  }

  static class Qnode { // Queue node inner class
    public boolean locked = false;
  }
}
// Figure 7.33 An incorrect attempt to implement a CLHLock.
```
Ans: 某一个线程的myNode 和 tail 可能导致指向同一个 node，当下一次上锁的时候，
tail.getAndSet(qnode) 获取到就是刚刚赋值为true的node,进而导致死锁。
比如当只有一个线程的时候。

Exercise 91. Design an isLocked() method that tests whether any thread is holding a lock (but does not acquire the lock).
Give implementations for
1. Any testAndSet() spin lock
2. The CLH queue lock,
3. and The MCS queue lock

Ans:
设计的基本思想是复用lock的调用，如果出现无法上锁，那么返回false，否则进行unlock操作并且返回true.

1. `testAndSet()`
```java
public class TASLock implements Lock {
  public boolean isLocked() {
    if (state.getAndSet(true)) {
      return true;
    }else{
      state.set(false);
      return false;
    }
  }
}
```

2. `CLH` lock
```java
public boolean isLocked() {
  QNode qnode = myNode.get();
  qnode.locked = true;
  QNode pred = tail.getAndSet(qnode);
  myPred.set(pred);
  if (pred.locked) {
    return true;
  }else{
    unlock();
    return false;
  }
}

public void unlock() {
  QNode qnode = myNode.get();
  qnode.locked = false;
  myNode.set(myPred.get());
}
```

3. MCS
```java
public void lock() {
  QNode qnode = myNode.get();
  QNode pred = tail.getAndSet(qnode);
  if (pred != null) {
  qnode.locked = true;
  pred.next = qnode;
  // wait until predecessor gives up the lock
  if (qnode.locked) {}
    return true;
  }else{
    unlock();
  }
}

```





# ref
https://www.chegg.com/homework-help/the-art-of-multiprocessor-programming-0th-edition-solutions-9780123705914


