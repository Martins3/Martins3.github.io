# Linux Programming Interface: Chapter 52: POSIX Message Queue
This chapter describes POSIX message queues, which allow processes to exchange data in the form of messages

POSIX message queues are similar to their System V counterparts, in that data is exchanged in units of whole messages

 However, there
are also some notable differences:
1. POSIX message queues are reference counted. A queue that is marked for
deletion is removed only after it is closed by all processes that are currently
using it.
2. Each System V message has an integer type, and messages can be selected in a
variety of ways using `msgrcv()`. By contrast, POSIX messages have an associated priority, and messages are always strictly queued (and thus received) in priority order.
3. POSIX message queues provide a feature that allows a process to be **asynchronously** notified when a message is available on a queue.


## 52.1 Overview

The main functions in the POSIX message queue API are the following:
1. The `mq_open()` function creates a new message queue or opens an existing queue, returning a message queue descriptor for use in later calls.
2. The `mq_send()` function writes a message to a queue.
3. The `mq_receive()` function reads a message from a queue.
4. The `mq_close()` function closes a message queue that the process previously opened.
5. The `mq_unlink()` function removes a message queue name and marks the queue for deletion when all processes have closed it

The above functions all serve fairly obvious purposes. In addition, a couple of features are peculiar to the POSIX message queue API:
1. Each message queue has an associated set of attributes. Some of these
attributes can be set when the queue is created or opened using `mq_open()`. Two
functions are provided to retrieve and change queue attributes: `mq_getattr()` and
`mq_setattr()`.
2. The `mq_notify()` function allows a process to register for message notification
from a queue. After registering, the process is notified of the availability of a message by delivery of a signal or by the invocation of a function in a separate thread.


## 52.2 Opening, Closing, and Unlinking a Message Queue
However, if `O_CREAT` is specified in flags, two further arguments
are required: mode and attr. 
> mq_queue 两种模式，打开和创建

Upon successful completion, mq_open() returns a message queue descriptor, a value of
type `mqd_t`, which is used in subsequent calls to refer to this open message queue.
> 所以 message queue 不像其他的 IPC 机制那样，使用key_t 创建，而是使用 `char * name`


## 52.3 Relationship Between Descriptors and Message Queues
The relationship between a message queue descriptor and an open message queue
is analogous to the relationship between a file descriptor and an open file



On Linux, POSIX message queues are implemented as i-nodes in a virtual file
system, and message queue descriptors and open message queue descriptions
are implemented as file descriptors and open file descriptions, respectively

1. Two processes can hold message queue descriptors (descriptor x in the diagram)
that refer to the **same open message queue description**. This can occur because
a process opens a message queue and then calls fork(). These descriptors share
the state of the O_NONBLOCK flag.
2. Two processes can hold open message queue descriptors that refer to different
message queue descriptions that refer to the **same message queue** (e.g.,
descriptor z in process A and descriptor y in process B both refer to /mq-r). This
occurs because the two processes each used mq_open() to open the same queue.

## 52.4 Message Queue Attributes
```
struct mq_attr {
  long mq_flags;  /* Message queue description flags: 0 or
                    O_NONBLOCK [mq_getattr(), mq_setattr()] */
  long mq_maxmsg; /* Maximum number of messages on queue
                    [mq_open(), mq_getattr()] */
  long mq_msgsize; /* Maximum message size (in bytes)
                    [mq_open(), mq_getattr()] */
  long mq_curmsgs; /* Number of messages currently in queue
                    [mq_getattr()] */
}
```
> 接下来分析了几个例子

## 52.5 Exchanging Message
> 介绍mq_send 和 mq_receive 的使用，并且演示了一个例子，说明block 的含义

If the message queue is already full (i.e., the `mq_maxmsg` limit for the queue has
been reached), then a further `mq_send()` either blocks until space becomes available
in the queue, or, if the `O_NONBLOCK` flag is in effect, fails immediately with the error
`EAGAIN`.

The `mq_timedsend()` and `mq_timedreceive()` functions are exactly like `mq_send()` and
`mq_receive()`, except that if the operation can’t be performed immediately, and the
`O_NONBLOCK` flag is not in effect for the message queue description, then the
`abs_timeout` argument specifies a limit on the time for which the call will block.

## 52.6 Message Notification
The sigev_notify field of this structure is set to one of the following values:
> 下面分别讲解三个取值的含义 SIGEV_NONE  SIGEV_SIGNAL SIGEV_THREAD
> 但是没有看

#### 52.6.1 Receiving Notification via a Signal
> skip

#### 52.6.2 Receiving Notification via a Thread
> skip

## 问题
 If oflag includes `O_CREAT`, a new, empty
queue is created if one with the given name doesn’t already exist. If oflag specifies
both `O_CREAT` and `O_EXCL`, and a queue with the given name already exists, then
`mq_open()` fails.
> 似乎这两个标志反复的说明出现，但是有一种情况没有说明:
> 当queue已经出现, 没有O_EXCL 会出现什么情况。
> 创建两个queue ? 报错 ?(如何报错，那么O_EXCL就没有意义了


