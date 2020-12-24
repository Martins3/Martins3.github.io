# Linux Programming Interface: Chapter 25

P532 感觉很简单的东西，就是看不懂，难受。

> There is one circumstance in which calling exit() and returning from main() are
> not equivalent. If any steps performed during exit processing access variables
> local to main(), then doing a return from main() results in undefined behavior.
> For example, this could occur if a variable that is local to main() is specified in
> a call to setvbuf() or setbuf() (Section 13.2).

25.2 Details of Process Termination 这一节可以关注一下。


The following actions are performed by exit():
- Exit handlers (functions registered with atexit() and on_exit()) are called, in
reverse order of their registration (Section 25.3).
- The stdio stream buffers are flushed.
- The `_exit()` system call is invoked, using the value supplied in status.

An exit handler is a programmer-supplied function that is registered at some
point during the life of the process and is then automatically called during normal
process termination via exit(). Exit handlers are not called if a program calls `_exit()`
directly or if the process is terminated abnormally by a signal.


A child process created via fork() inherits a copy of its parent’s exit handler registrations. When a process performs an exec(), all exit handler registrations are
removed. 

To address these limitations, glibc provides a (nonstandard) alternative method
of registering exit handlers: on_exit().
> on_exit 比 atexit 多出了参数和返回值



