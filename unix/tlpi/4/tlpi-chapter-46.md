# Linux Programming Interface : Chapter 46 : SYSTEM V Message Queue
 Although message queues are similar to
pipes and FIFOs in some respects, they also differ in important ways:

1. The handle used to refer to a message queue is the identifier returned by a call
to `msgget()`. 
2. Communication via message queues is message-oriented; that is, the reader
receives whole messages, as written by the writer. 
3. Messages can
be retrieved from a queue in first-in, first-out order or retrieved by type

These limitations lead us to the conclusion that, where
possible, new applications should avoid the use of System V message queues in
favor of other IPC mechanisms such as FIFOs, POSIX message queues, and sockets.
> 好的，我溜了，去POSIX Message Queue了




