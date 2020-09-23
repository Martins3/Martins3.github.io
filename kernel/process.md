The kernel stores the list of processes in a circular doubly linked list called the task list

**process descriptor** of the type `struct task_struct`

describes the executing program—open files, the process’s address space, pending signals, the process’s state, and much more

3. prior to 2.6 `struct task_struct` was stored at the end of the kernel stack of each process

With the process descriptor now
dynamically created via the **slab allocator**, a new structure, `struct thread_info`,
was created that again lives at the bottom of the stack (for stacks that grow down) and at the top
of the stack (for stacks that grow up)

Each task’s `thread_info` structure is allocated at the end of its stack

> 通过`thread_info`找到对应的　`task_struct`，　但是为什么不是kernel 统一管理全部的数据，我认为此处含有指代不明的地方
> 应该是放置到kernel stack 中间的

This technique
delays the copying of each page in the address space until it is actually written to. In the
case that the pages are never written—for example, if exec() is called immediately after
fork()—they never need to be copied

> 立刻使用`fork`可以对于pages永远不写的操作
> `vfork`　无法理解，还是由于 空间复制的操作的原因
