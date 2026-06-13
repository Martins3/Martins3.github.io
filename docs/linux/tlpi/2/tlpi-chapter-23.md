# Timers and Sleeping

## 23.1 Interval Timers
skip

## 23.2 Scheduling and Accuracy of Timers
> For example, if a real-time timer is set to expire
every 2 seconds, then the delivery of individual timer events may be subject to the
delays just described, but the scheduling of subsequent expirations will nevertheless be at exactly the next 2-second interval. In other words, interval timers are not
subject to creeping errors.

> If this support is enabled (via the `CONFIG_HIGH_RES_TIMERS` kernel configuration option), then the accuracy of the various timer and sleep interfaces that we describe in this chapter is no longer constrained by the size of the kernel jiffy.

## 23.3 Setting Timeouts on Blocking Operations
Use alarm() to interupt blocked syscall

## 23.4 Suspending Execution for a Fixed Interval (Sleeping)
skip

## 23.5 POSIX Clocks
skip

## 23.6 POSIX Interval Timers
skip

## 23.7 Timers That Notify via File Descriptors: the `timerfd` API
skip

## 23.8 Summary
skip

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
