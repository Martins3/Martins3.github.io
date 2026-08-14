看看 cat /proc/4580/status 中是如何获取的:
```txt
voluntary_ctxt_switches:        21932
nonvoluntary_ctxt_switches:     52
```

不过思考下，这个意味着什么，如果数值不在增长?

如何理解 nonvoluntary_ctxt_switches 的含义

尝试构建自己的 scheduler 的参考:
1. https://stackoverflow.com/questions/54635617/what-scheduling-policy-does-each-return-int-value-from-sched-getschedulerpid-c
2. https://stackoverflow.com/questions/45692699/build-against-newer-linux-headers-than-libc-is-built-using
