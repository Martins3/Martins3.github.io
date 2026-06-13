# Threads: Further Details

## 33.2 Threads and Signals
The differences between the signal and thread models mean that combining
signals and threads is complex, and should be avoided whenever possible.

> signal 在设计的时候根本没有考虑 thread 的使用，不要混用两者

- Signal actions are **process-wide**. If any unhandled signal whose default action is
stop or terminate is delivered to any thread in a process, then all of the threads
in the process are stopped or terminated.
- Signal dispositions are **process-wide**;
- A signal may be directed to either the process as a whole or to a specific thread.
- When a signal is delivered to a multithreaded process that has established a signal handler, the kernel arbitrarily selects one thread in the process to which to
deliver the signal and invokes the handler in that thread.
- The signal mask is per-thread.
- *The kernel maintains a record of the signals that are pending for the process as a
whole, as well as a record of the signals that are pending for each thread. A call to
sigpending() returns the union of the set of signals that are pending for the process and those that are pending for the calling thread. In a newly created thread,
the per-thread set of pending signals is initially empty. A thread-directed signal
can be delivered only to the target thread. If the thread is blocking the signal, it
will remain pending until the thread unblocks the signal (or terminates).*

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
