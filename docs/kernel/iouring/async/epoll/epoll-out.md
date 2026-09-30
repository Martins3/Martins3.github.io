# EPOLLOUT 什么时候是"必须"监听的?

关联测试 : ./epoll-out.c (本目录)

问题: 很多网络程序(nginx、redis、各种 echo server)从不监听 EPOLLOUT, 为什么?
反过来, 什么场景下 EPOLLOUT 是"必须"的?

## 先看结论

EPOLLOUT 的含义是: send buffer 里有空间, write() 不会阻塞。

对 request/response 型应用:
- 响应小、对端读得快, send buffer 几乎永远不满
- write() 总是直接成功, 根本轮不到 EPOLLOUT 出场

所以"不监听 EPOLLOUT"不是偷懒, 而是工程纪律:

> EPOLLOUT 只应该"按需"注册: 当 write() 返回 EAGAIN 之后才把 EPOLLOUT 加进
> interest list, 数据发完立刻摘除。

如果反过来"一直注册着 EPOLLOUT", 在水平触发(LT)下会立刻出问题:
socket 的 send buffer 常态是空的, 也就是"可写"状态永远成立, epoll_wait 会
每次都立刻返回 EPOLLOUT, 变成 100% CPU 的空转循环。
(epoll-out.c 的 client 端用 `epoll_wait(timeout=0)` 实测证明了这一点)

## 必须监听 EPOLLOUT 的两个场景

### 1. 非阻塞 connect() 的完成通知

```c
sockfd = socket(...);
fcntl(sockfd, F_SETFL, ... | O_NONBLOCK);
connect(sockfd, ...);   // 返回 -1, errno == EINPROGRESS
```

连接建立的握手在内核里异步进行, 没有任何其他渠道告诉你"好了", 只能:

```c
epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, EPOLLOUT);
epoll_wait(...);                     // EPOLLOUT 到来
getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, ...);
// err == 0          -> 连接成功
// err == ECONNREFUSED 等 -> 连接失败(失败也通过 EPOLLOUT/EPOLLERR 报出来)
```

注意 connect 完成后要立刻把 EPOLLOUT 摘掉, 原因见上面的"空转循环"。

### 2. 发送方比接收方快: send buffer 被写满

demo 里 server 向一个故意 3 秒不读的 client 推 64 MiB 数据:

```txt
[server] 直接 write() 只推了 174231 字节就返回 EAGAIN: send buffer 满了
```

实测的 send buffer 只有 ~128 KiB(内核会把 SO_SNDBUF 的设置值加倍)。
之后每一字节都要经过:

```txt
write() -> EAGAIN(send buffer 满)
        -> EPOLL_CTL_MOD 按需加上 EPOLLOUT
        -> epoll_wait 睡下去
        -> 对端读走数据、ACK 回来、sndbuf 释放空间 -> 内核唤醒(EPOLLOUT)
        -> 继续 write() -> 再 EAGAIN -> ...
        -> 发完, 把 EPOLLOUT 摘掉
```

不带 EPOLLOUT 跑一次(`./epoll-out.out -n`)是反证:
client 愿意读, server 也还有 66 MiB 要发, 但 server 收不到任何通知,
传输永远停在 174231 字节 —— 死锁。两次运行 phase1 一模一样, 差异只在于
"有没有监听 EPOLLOUT"。

## 内核侧: EPOLLOUT 是怎么产生的

socket 的 poll 回调中, 可写条件由 `sk_stream_write_space()` / `sock_def_write_space()`
驱动: TCP 收到 ACK、write queue 出队腾出空间时, 会唤醒等待队列并置上
EPOLLOUT 位, 对应到 epoll 就是 `ep_poll_callback` 把 fd 挂到就绪链表。

也就是说: EPOLLOUT 本质上是"从满到有空位"的边界通知。
缓冲区常年有空位 -> 永远 ready -> LT 下空转(所以不能常挂);
缓冲区真的满了 -> 只有它能叫醒你(所以必须挂)。

## demo 运行输出

正常路径 `./epoll-out.out`: 按需注册 EPOLLOUT, 66 MiB 全部发出, 靠 105 次
EPOLLOUT 唤醒。

```txt
[client] 非阻塞 connect() 返回 EINPROGRESS; 连接是否建立, 只能等 EPOLLOUT 通知
[client] epoll_wait 被唤醒, events=0x4
[client] EPOLLOUT + SO_ERROR==0 => 连接建立成功
[client] (证明) EPOLLOUT 还注册着时 epoll_wait(timeout=0) 立刻返回 1 次 => 可写状态永远成立
[client] 所以 connect 完成后马上摘掉 EPOLLOUT, 只留 EPOLLIN (读方不需要它)
[client] 我先睡 3 秒不读数据: server 会在这期间把 send buffer 写满并撞上 EAGAIN
[client] 收到 EOF, 共 67108864 字节 == 服务器发送总量: 传输完整完成
[server] 接受连接; 实际 send buffer ≈ 131072 字节 (内核会把设置值加倍)
[server] --- 关键点 ---
[server] 直接 write() 只推了 174231 字节就返回 EAGAIN: send buffer 满了
[server] 剩余 66934633 字节若还想发出去, 唯一途径就是等 EPOLLOUT 通知空间释放
[server] 按需注册 EPOLLOUT (EPOLL_CTL_MOD), 然后阻塞在 epoll_wait
[server] EPOLLOUT 第 1 次唤醒: 已发 174231 / 67108864
...
[server] EPOLLOUT 第 100 次唤醒: 已发 65036534 / 67108864
[server] 全部发完, 把 EPOLLOUT 摘掉 (回到只监听 EPOLLIN)
[server] ============ 结果 ============
[server] 共发送 67108864 字节; 其中 phase1=174231 字节不依赖 EPOLLOUT;
[server] 剩余 66934633 字节完全靠 105 次 EPOLLOUT 唤醒才得以发出。
```

反证 `./epoll-out.out -n`: server 拒绝注册 EPOLLOUT, 传输卡死。

```txt
[client] 连续 2000 ms 没有新数据: 只收到 174231 字节。
[client] 这正是 (-n) 反证: server 没监听 EPOLLOUT, 剩余数据永远不会再来, 我放弃
[server] 收到 0x2001: client 关闭/出错
[server] ============ 反证成立 ============
[server] 卡死在 174231 字节 (phase1=174231)。没有监听 EPOLLOUT, 即使 client 愿意读, 也永远不会有 任何事件来唤醒我。
```

## 一句话总结

- 常态: send buffer 有空位 -> EPOLLOUT 永远 ready -> 注册了就是空转, 所以不监听
- 必须: connect 完成通知、write() 撞上 EAGAIN 之后 -> 只能靠 EPOLLOUT, 所以按需监听
- 判断标准就一条: 你上一次 write() 有没有返回 EAGAIN? 没有就根本不需要 EPOLLOUT

## 真实实现: libnbd 是怎么解决 EPOLLOUT 问题的

参考源码: /home/martins3/data/libnbd (状态机源码在 generator/states*.c, 生成的
运行代码在 lib/states.c / lib/states-run.c)

libnbd 把上面那套纪律做成了一个库: 它不要求调用方"静态注册 EPOLLOUT", 而是
把"现在该等读还是等写"变成状态机的输出, 事件循环每轮重新询问。

### 1. 非阻塞 socket + 单 fd 状态机

所有 I/O 都在状态机里完成, socket 永远是非阻塞的。发数据用
`generator/states.c` 中的 `send_from_wbuf()`, 收数据用 `recv_into_rbuf()`:

```c
r = send(h->sock, h->wbuf, h->wlen, ...);
if (r == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return 1;   /* 能发多少发多少, EAGAIN 就停, 绝不重试自旋 */
    return -1;
}
h->wbuf += r;   /* 剩余字节留在 wbuf/wlen 里, 下次唤醒从断点续发 */
h->wlen -= r;
```

run 循环怎么知道"该停下来了"? 靠 `lib/states.c` 中的两个宏:

```c
#define SET_NEXT_STATE(s)         (*blocked = false, *next_state = (s)) /* 推进 */
#define SET_NEXT_STATE_AND_BLOCK(s) (*next_state = (s))                  /* 停在 s, 等外部事件 */
```

撞上 EAGAIN 时不推进, `blocked` 保持 true, 状态机退出, 等 poll/epoll。

### 2. 方向 API: nbd_aio_get_direction()

每个状态在 `nbd_internal_aio_get_direction()` (lib/states-run.c) 里登记方向:

| 状态                                                                        | 方向        | 含义                                                                 |
| --------------------------------------------------------------------------- | ----------- | -------------------------------------------------------------------- |
| `READY`                                                                     | READ        | 等 server 回复/EOF                                                   |
| `ISSUE_COMMAND.SEND_REQUEST` / `SEND_WRITE_PAYLOAD` / `SEND_WRITE_SHUTDOWN` | WRITE\|READ | 半截请求卡在 send buffer (BOTH!): 等 EPOLLOUT 续传, 同时还能收 reply |
| `CONNECT.CONNECTING` / `CONNECT_TCP.*`                                      | WRITE       | 非阻塞 connect 完成通知                                              |
| 握手/option 的 `*_SEND`                                                     | WRITE       | 要往外发数据                                                         |
| 握手/option 的 `*_RECV_*`                                                   | READ        | 要收数据                                                             |
| `DEAD` / `CLOSED`                                                           | NONE        | 没有可等的东西                                                       |

调用方的事件循环每轮都问一遍 (examples/batched-read-write.c 的写法):

其实核心流程是:
```c
// 根据当前的事件的状态来检查到底监听什么事件
dir = nbd_aio_get_direction (nbd);
fds[0].events = 0;
if ((dir & LIBNBD_AIO_DIRECTION_READ) != 0)
    fds[0].events |= POLLIN;
if ((dir & LIBNBD_AIO_DIRECTION_WRITE) != 0)
    fds[0].events |= POLLOUT;

// 开始监听
poll (fds, 1, -1);

// 如果发现事件就绪，根据事件内容，驱动需要执行的任务
if (POLLIN 就绪)  nbd_aio_notify_read (nbd);
if (POLLOUT 就绪) nbd_aio_notify_write (nbd);
```

EPOLLOUT 只在库"确实有字节要发"时才出现在返回的方向里 —— 这就是我们前面说的
"EPOLLOUT 按需注册"的 API 化。空闲时方向恒为 READ(文档甚至提醒: 单线程下没有
命令在途时去 poll 是浪费, 因为不会有事件来)。

### 3. 为什么卡在发送时方向是 BOTH 而不是 WRITE

这是最值得抄的一笔。当 NBD_CMD_WRITE 的大 payload 塞满 send buffer 时, 如果
只等 EPOLLOUT 会死锁: server 可能是串行处理命令的, 它要先回完前面的 reply 才
肯读我们的下一个命令; 而它的 send buffer 也可能满了, 两边都堵着等对方读。

所以 SEND_* 状态的 direction 是 WRITE|READ: 等 EPOLLOUT 续传的同时, 一有
POLLIN 就先去把 reply 读掉, 腾出 server 的空间。examples/batched-read-write.c
就是专门测这个场景的(同时把大 pread + 大 pwrite 押进队列, 还挂了 SIGALRM
防死锁)。POLLIN 与 POLLOUT 同时到达时优先 notify_read, 因为处理 reply 可能
让"继续写"变得不必要。

### 4. connect 完成与断开检测

- 连接建立: generator/states-connect.c 的 CONNECT.START 里直接调非阻塞
  connect(), 返回 EINPROGRESS 就停在 CONNECTING, 方向 = WRITE; EPOLLOUT
  到来后 notify_write 推进到 CONNECT.CONNECTING(), 里面
  getsockopt(SOL_SOCKET, SO_ERROR) 判定成败 (失败则换下一个地址重试)。
  这和 demo 里 client 端的做法一模一样。
- 对端断开: libnbd 不靠 EPOLLOUT 检测。就绪后方向 = READ, poll 返回
  POLLIN|POLLHUP 就 notify_read, recv 返回 0 -> 进入 CLOSED 状态。
  (lib/poll.c 中 do_poll() 的处理: POLLIN|POLLHUP 归 notify_read, 其次才是
  POLLOUT)

小结: libnbd 的"方向"机制 = 我们 demo 结论的工程化 —— EPOLLOUT 不该是事件
循环里一个常驻的注册项, 而应该是"状态机下一步需要写"时的瞬时结果。

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
