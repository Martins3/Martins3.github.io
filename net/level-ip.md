# [level-ip](https://github.com/saminiir/level-ip)

TCP close : client and server do a "active" finished and receive the corresponding ack, then they are closed, this is diagram.[^1]
![](https://benohead.com/wp-content/uploads/2013/07/TCP-CLOSE_WAIT.png)

```c
static struct net_family *families[128] = {
    [AF_INET] = &inet,
};

struct net_family inet = {
    .create = inet_create,
};

static struct sock_type inet_ops[] = {
    {
        .sock_ops = &inet_stream_ops,
        .net_ops = &tcp_ops,
        .type = SOCK_STREAM,
        .protocol = IPPROTO_TCP,
    }
};

static struct sock_ops inet_stream_ops = {
    .connect = &inet_stream_connect,
    .write = &inet_write,
    .read = &inet_read,
    .close = &inet_close,
    .free = &inet_free,
    .abort = &inet_abort,
    .getpeername = &inet_getpeername,
    .getsockname = &inet_getsockname,
};
```

- [ ] Why we need unix socket to handle it ?

- `_read` : this is almost the
  - inet_read
    - tcp_read
      - tcp_receive
        - tcp_data_dequeue

[^1]: https://benohead.com/blog/2013/07/21/tcp-about-fin_wait_2-time_wait-and-close_wait/
