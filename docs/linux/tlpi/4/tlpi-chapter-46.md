# System V Message Queues
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
> 好的，我溜了，去 POSIX Message Queue 了

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
