## fork
> fork 完成的事情非常简单：拷贝 wakeup
> > 拷贝的内容比想象的多，和 init 类似，都是众多内容的合集，按照书上的内容走吧!
>
> dup_task_struct : 分配 current 的进程控制块以及复制 task_struct 和 kernel stack

- [ ] pthread_atfork : 在 multithread 的情况下进行 fork 的时候，整个地址空间都会拷贝，但是存在一些恶心的事情
  - The problem is that fork() only copies the calling thread, and any mutexes held in child threads will be forever locked in the forked child. The pthread solution was the pthread_atfork() handlers. [^1]
  - [x] 在 multithread 中间执行 exec 的时候，也是存在很诡异的问题，其他的 threads 的地址空间被删除了，所以只有当 thread_group 只有一个 thread 才可以，细节在 de_thread 中间

- 通过 vfork 可以避免页表的拷贝， the parent is suspended until the child terminates or calls exec()
在进行 exec 的时候，会创建新的地址空间出来，exec_mmap 调用 exec_mm_release 来, 并且 mmput 之前的 mm_struct 减少引用计数
    - bprm_mm_init : 创建 mm_struct
    - exec_mmap : 切换新的 mm_struct


[^1]: https://stackoverflow.com/questions/6078712/is-it-safe-to-fork-from-within-a-thread
