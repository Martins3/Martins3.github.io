```txt
[root@nixos:/sys/kernel/debug/tracing]# cat available_tracers
blk function_graph wakeup_dl wakeup_rt wakeup function nop
```
- 可以勉强读读的内容:
  - https://static.lwn.net/images/conf/rtlws11/papers/proc/p02.pdf

这个是存在具体代码的跟踪的:
- https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2020/08/05/tracing-basic
