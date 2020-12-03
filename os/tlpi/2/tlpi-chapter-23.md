# Linux Programming Interface: Timer and Sleeping

## 23.1 Interval Timers
skip

## 23.2 Scheduling and Accuracy of Timers
> For example, if a real-time timer is set to expire
every 2 seconds, then the delivery of individual timer events may be subject to the
delays just described, but the scheduling of subsequent expirations will nevertheless be at exactly the next 2-second interval. In other words, interval timers are not
subject to creeping errors.

huxueshi: the crazy English

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
