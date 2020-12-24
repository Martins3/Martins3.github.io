# Linux Programming Interface: Chapter 24

- vfork 和 fork 的区别 ? 一般上来说，vfork 和 exec 配合使用

Except where speed is absolutely critical, new programs should avoid the use of vfork() in favor of fork(). 

SUSv3 marks vfork() as obsolete, and SUSv4 goes further, removing the specification of vfork(). 
> 所以，道理很简单，弃用 vfork


fork 机制对于 parent 还是 chilren 谁先运行，并没有任何保证，但是书上利用 signal 机制实现
