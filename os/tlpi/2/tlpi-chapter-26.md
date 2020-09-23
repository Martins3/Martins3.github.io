# Linux Programming Interface: Monitor Child Process

In many application designs, a parent process needs to know when one of its child
processes changes state—when the child terminates or is stopped by a signal. This
chapter describes two techniques used to monitor child processes: the `wait()` system
call (and its variants) and the use of the `SIGCHLD` signal.

## 26.1 Waiting on a Child Process
> skip

## 26.2 Orphans and Zombies
> parent 和 children 谁先阵亡，各自产生一种情况
> skip

## 26.3 The SIGCHLD Signal
To get around **these** problems, we can employ a handler for the `SIGCHLD` signal.

#### 26.3.1 Establishing a Handler for SIGCHLD
> 这一个信号难道不是用来实现 ptrace 的吗 ?

The `SIGCHLD` signal is sent to a parent process whenever one of its children terminates. By default, this signal is ignored, but we can catch it by installing a signal handler.
Within the signal handler, we can use `wait()` (or similar) to reap the zombie child.
However, there is a subtlety to consider in this approach.

The **solution** is to loop inside the `SIGCHLD` handler, repeatedly calling `waitpid()`
with the `WNOHANG` flag until there are no more dead children to be reaped. Often, the
body of a `SIGCHLD` handler simply consists of the following code, which reaps any
dead children without checking their status:
```c
while (waitpid(-1, NULL, WNOHANG) > 0)
  continue;
```
The above loop continues until `waitpid()` returns either 0, indicating no more zombie
children, or –1, indicating an error (probably `ECHILD`, meaning that there are no more children).

* ***Design issues for SIGCHLD handlers***
> skip 

## 26.4 Summary
Using `wait()` and `waitpid()` (and other related functions), a parent process can
obtain the status of its terminated and stopped children. **This status indicates
whether a child process terminated normally (with an exit status indicating either
success or failure), terminated abnormally, was stopped by a signal, or was resumed
by a `SIGCONT` signal.**
> wait() 居然还可以获取知道 process 是不是 stopped by a signal 以及 resumed by a signal 

A common way of reaping dead child processes is to establish a handler for the
`SIGCHLD` signal. This signal is delivered to a parent process whenever one of its children terminates,
and optionally when a child is stopped by a signal. Alternatively,
but somewhat less portably, *a process may elect to set the disposition of SIGCHLD to
`SIG_IGN`, in which case the status of terminated children is immediately discarded (and
thus can’t later be retrieved by the parent), and the children don’t become zombies.*
> @todo 哪里分析过的 ?


