# ipc/shm.md
> @todo 根据前面的分析，猜测 shm 的实现就是 获取一个 id ，然后得到内核分配的内存，然后拷贝到自己的空间中间 ?
> 太过于轻视这个东西了

process A 和 process B 共享的时候，需要三块还是一块内存 ? (经验告诉我，需要三块)


## TODO
1. shmat
2. shmget
