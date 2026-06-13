# Linux Programming Interface: POSIX Semaphores
The oflag argument is a bit mask that determines whether we are opening an existing semaphore or creating and opening a new semaphore. If oflag is 0, we are accessing
an existing semaphore. If O_CREAT is specified in oflag, then a new semaphore is created
if one with the given name doesn’t already exist. If oflag specifies both O_CREAT and
O_EXCL, and a semaphore with the given name already exists, then sem_open() fails.
> 所以当， 名称已经重复而且出现了，而且没有设置O_EXCL 并且设置了O_CREAT的时候，那么如何办。

If a blocked sem_wait() call is interrupted by a signal handler, then it fails with
the error EINTR, regardless of whether the SA_RESTART flag was used when establishing the signal handler with sigaction().
# Linux Programming Interface: Chapter 53
