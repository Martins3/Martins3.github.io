# Linux Programming Interface: Time


## 10.1 Calendar Time

The reason for the existence of two system calls (time() and gettimeofday()) with
essentially the same purpose is historical. Early UNIX implementations provided time(). 4.3BSD added the more precise gettimeofday() system call. The
existence of time() as a system call is now redundant; it could be implemented
as a library function that calls gettimeofday().
> time() 和 gettimeofday() 两者是


## 10.2 Time-Conversion Functions
Figure 10-1 总结到位

> 至于其他的内容，装换，只是用户态的内容，现在没有兴趣。

## 10.5 Updating the System Clock

Linux also provides the stime() system call for setting the system clock. The difference between settimeofday() and stime() is that the latter call allows the new
calendar time to be expressed with a precision of only 1 second. As with time()
and gettimeofday(), the reason for the existence of both stime() and settimeofday()
is historical: the latter, more precise call was added by 4.3BSD.
> stime() 和 settimeofday() 

Abrupt changes in the system time of the sort caused by calls to settimeofday() can
have deleterious effects on applications (e.g., make(1), a database system using
timestamps, or time-stamped log files) that depend on a monotonically increasing
system clock. For this reason, when making small changes to the time (of the order
of a few seconds), it is usually preferable to use the adjtime() library function, which
causes the system clock to gradually adjust to the desired value.

**It may be that an incomplete clock adjustment was in progress at the time of the
adjtime() call**. In this case, the amount of remaining, unadjusted time is returned in
the timeval structure pointed to by olddelta. If we are not interested in this value, we
can specify olddelta as NULL. Conversely, if we are interested only in knowing the currently outstanding time correction to be made, and don’t want to change the value,
we can specify the delta argument as NULL.

On Linux, adjtime() is implemented on top of a more general (and complex)
Linux-specific system call, `adjtimex()`. This system call is employed by the
Network Time Protocol (NTP) daemon. For further information, refer to the
Linux source code, the Linux adjtimex(2) manual page, and the NTP specification ([Mills, 1992])

## 10.6 The Software Clock (Jiffies)
> 控制时钟中断发生的次数的

## 10.7 Process Time
The times() system call retrieves process time information, returning it in the structure pointed to by `buf`

The clock() function provides a simpler interface for retrieving the process
time. It returns a single value that measures the total (i.e., user plus system) CPU
time used by the calling process.
