# 到底存在那些线程

```txt
🧀  ps -aux  | wc -l
576

🧀  cat /proc/loadavg
0.97 0.91 1.79 2/2576 278329
```

px -aux 是通过遍历 /proc/ 目录的

现实的内容是:
kernel/fork.c
```c
int nr_threads;			/* The idle threads do not count.. */
```

- [ ] 既然 nr_threads 中 idle threads 没有统计，到底是什么意思？

- copy_process  ++

* wait_task_zombie
* find_child_reaper
* exit_notify
* de_thread
- release_task
  - `__exit_signal`
    - `__unhash_process` --

man proc 中可以看到:

> The value after the slash is the number of kernel scheduling entities that currently exist on the system.

感觉就是当前的进程的数量，难道是 /proc/cmdline 分析的不对吗?

有时候多，有时候少！

因为 namespace 的影响? 我本身就是在 root 啊
