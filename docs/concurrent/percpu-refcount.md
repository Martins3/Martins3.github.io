## [Per-CPU reference counts](https://lwn.net/Articles/557478/)
这个介绍的很清晰

继续分析一下其中的 rcu 问题吧

## https://docs.kernel.org/RCU/rcuref.html

## percpu_ref_kill

- percpu_ref_exit 似乎是真正的结束
