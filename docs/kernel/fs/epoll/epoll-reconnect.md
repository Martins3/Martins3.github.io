# epoll 断联之后，可以继续重连

关联测试 : docs/kernel/fs/epoll/epoll-reconnect.c

```txt
Socket disconnected (EPOLLERR/EPOLLHUP), attempting to reconnect...
Socket disconnected (EPOLLERR/EPOLLHUP), attempting to reconnect...
Socket disconnected (EPOLLERR/EPOLLHUP), attempting to reconnect...
Socket disconnected (EPOLLERR/EPOLLHUP), attempting to reconnect...
Socket disconnected (EPOLLERR/EPOLLHUP), attempting to reconnect...
Socket disconnected (EPOLLERR/EPOLLHUP), attempting to reconnect...
Socket disconnected (EPOLLERR/EPOLLHUP), attempting to reconnect...
Socket disconnected (EPOLLERR/EPOLLHUP), attempting to reconnect...
Socket disconnected (EPOLLERR/EPOLLHUP), attempting to reconnect...
client can connect to a broken unix domain socket
client can connect to a broken unix domain socket
client reconnect failed
client reconnect failed
client reconnect failed
client reconnect failed
client reconnect failed
client reconnect failed
client reconnect failed
Server process terminated
```

关闭 socket 的基本流程为:
```txt
exit_files()                                      →  kernel/exit.c
    └─> __fput()                                  →  fs/file_table.c
        └─> sock_close()                          →  net/socket.c
            └─> sock_release()                    →  net/socket.c
                └─> sk->sk_prot->release()        →  对于 AF_UNIX: unix_release() → net/unix/af_unix.c:1330
                    └─> unix_release_sock(sk)
                        ├─> unix_state_lock(peer_sk)
                        ├─> peer_sk->sk_state = TCP_CLOSE
                        ├─> unix_state_unlock(peer_sk)
                        └─> sk_wake_async(peer_sk, SOCK_WAKE_WAITD, POLLHUP)   ← 关键！
                            └─> sock_def_readable()         →  net/core/sock.c:2430
                                ├─> __wake_up_sync(&sk->sk_sleep, TASK_NORMAL, 1)
                                └─> wake_up_interruptible_poll(&wq_head, EPOLLIN | EPOLLHUP)
```
unix_release_sock 中会调用 unix_remove_bsd_socket(sk) ，将 socket 从 hash table 中移除掉。

如果重连，connect syscall 会调用到 unix_find_socket_byinode 中，会由于没有查询到，那么 connect syscall 失败。

当 server 关闭的时候，会自动关闭掉所有的 socekt :
1. server 的 listen socket ，通过 connect listen socket 来建立连接，获取 socket 通信
2. 和 client 通信的 socket ，这个会在 unix_release_sock 中调用 sk_wake_async ，让 client 的 epoll 接受到信号。

server 关闭 socket 的顺序是未定义的，
如果不去先 close server listen socket ，当 server 关闭掉 和 client 通信的 socket ，
那么 client 就会收到 server 挂掉的消息，然后发起重连。

解决办法:
1. 对于可控重连，在 server 结束的时候，关闭掉 server socket
2. 对于不可控重连，qemu 并不会存在立刻来连接，而是会等待一段时间，qemu 不会使用 epoll 来监听:
```txt
    ``reconnect-ms`` sets the timeout for reconnecting on non-server
    sockets when the remote end goes away. qemu will delay this many
    milliseconds and then attempt to reconnect. Zero disables reconnecting,
    and is the default.
```

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
