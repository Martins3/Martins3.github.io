# Linux Kernel Development : An Introduction to Kernel Synchronization

Code that is safe from concurrent access from an interrupt handler is said to be
**interrupt-safe**. Code that is safe from concurrency on symmetrical multiprocessing
machines is **SMP-safe**. Code that is safe from concurrency with kernel preemption is **preempt-safe**.

Many locking issues disappear on
uniprocessor machines; consequently, when **CONFIG\_SMP** is unset, unnecessary code is not
compiled into the kernel image
