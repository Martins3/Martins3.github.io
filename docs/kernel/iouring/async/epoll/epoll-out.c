/*
 * epoll-out.c
 *
 * 回答: EPOLLOUT 在什么场景下是"必须"监听的?
 *
 * 背景事实:
 *   EPOLLOUT 表示 "send buffer 里有空闲空间, 可以 write 而不阻塞"。
 *   对绝大多数 request/response 应用(nginx/redis 等), 响应很小, 对端读得也快,
 *   send buffer 几乎永远不会满, write() 总是直接成功 --- 所以它们从不注册
 *   EPOLLOUT (水平触发下, socket 几乎总是可写, 注册了反而会让 epoll_wait
 *   每次都立刻返回, 变成 100% CPU 的空转循环)。
 *
 *   但当"发送方"要推的数据量 > send buffer 容量、且对端读得慢时, write() 会
 *   返回 EAGAIN。此时没有任何其他事件能告诉你 "空间释放了", 唯一的通知渠道
 *   就是 EPOLLOUT。不监听它, 发送就永远停在原地 --- 死锁。
 *
 * 本 demo 一次运行演示 EPOLLOUT 的两个必须场景:
 *   1. client 端: 非阻塞 connect() 返回 EINPROGRESS 后, 连接建立成功与否
 *      只能通过 EPOLLOUT (成功) / EPOLLERR (失败, SO_ERROR 取错误码) 得知。
 *   2. server 端: 向一个故意不读的 client 推 64 MiB 数据, send buffer 很快
 *      写满, write() 返回 EAGAIN, 之后每一字节都必须靠 EPOLLOUT 唤醒才能发出。
 *
 * 运行:
 *   ./epoll-out.out      # 正常路径: server 按需注册 EPOLLOUT, 传输完成
 *   ./epoll-out.out -n   # 反证: server 拒绝注册 EPOLLOUT, 传输永远无法完成
 *
 * fork 模型: 父进程是 server, 子进程是 client, 一次运行即可看到完整过程。
 */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define HOST "127.0.0.1"

#define TOTAL_BYTES (64UL * 1024 * 1024) /* server 想发送的总字节数 */
#define WRITE_CHUNK (128 * 1024)	 /* 单次 write 的块大小 */
#define READ_CHUNK (1024 * 1024)	 /* client 单次 read 的块大小 */
#define BUF_SIZE (64 * 1024)		 /* 故意调小的 send/recv buffer */
#define CLIENT_IDLE_S 3			 /* client 前 3 秒拒绝读取 */
#define STALL_TIMEOUT_MS 2000		 /* 超过该时长没有新数据 = 判定卡死 */

static int g_no_epollout; /* -n: server 故意不注册 EPOLLOUT */

static void die(const char *what)
{
	perror(what);
	exit(1);
}

static void set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		die("fcntl O_NONBLOCK");
}

static void epoll_add(int epfd, int fd, uint32_t events)
{
	struct epoll_event ev = { .events = events, .data.fd = fd };
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
		die("epoll_ctl ADD");
}

static void epoll_mod(int epfd, int fd, uint32_t events)
{
	struct epoll_event ev = { .events = events, .data.fd = fd };
	if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
		die("epoll_ctl MOD");
}

/*
 * server 端主循环。核心代码是 flood():
 *
 *   write() -> 直到 EAGAIN (send buffer 满)
 *           -> 此时才按需注册 EPOLLOUT  (EPOLL_CTL_MOD)
 *           -> epoll_wait 等到 EPOLLOUT 再继续 write
 *           -> 发完后立刻把 EPOLLOUT 摘掉
 *
 * 这就是"为什么大多数代码不监听 EPOLLOUT"的答案:
 * EPOLLOUT 只应该在 write() 真的撞上 EAGAIN 之后才注册, 发完立刻摘除。
 */
static void flood(int epfd, int fd)
{
	static char buf[WRITE_CHUNK];
	size_t sent = 0;
	size_t phase1 = 0; /* 撞上 EAGAIN 之前, 无需 EPOLLOUT 就发掉的字节数 */
	unsigned wakeups = 0;
	int need_out = 0;

	printf("[server] 尝试发送 %zu MiB, 每块 %d KiB\n",
	       TOTAL_BYTES / 1024 / 1024, WRITE_CHUNK / 1024);

	while (sent < TOTAL_BYTES) {
		size_t want = TOTAL_BYTES - sent;
		if (want > WRITE_CHUNK)
			want = WRITE_CHUNK;
		ssize_t n = write(fd, buf, want);
		if (n > 0) {
			sent += n;
			continue;
		}
		if (n < 0 && errno == EAGAIN) {
			if (phase1 == 0) {
				phase1 = sent;
				printf("[server] --- 关键点 ---\n");
				printf("[server] 直接 write() 只推了 %zu 字节就返回 EAGAIN: "
				       "send buffer 满了\n",
				       phase1);
				printf("[server] 剩余 %zu 字节若还想发出去, 唯一途径就是等 "
				       "EPOLLOUT 通知空间释放\n",
				       TOTAL_BYTES - phase1);
				if (g_no_epollout)
					printf("[server] (-n 模式) 但我拒绝注册 EPOLLOUT, 只等 "
					       "EPOLLIN/EPOLLRDHUP\n");
			}

			if (!g_no_epollout && !need_out) {
				need_out = 1;
				printf("[server] 按需注册 EPOLLOUT (EPOLL_CTL_MOD), 然后阻塞在 "
				       "epoll_wait\n");
				epoll_mod(epfd, fd,
					  EPOLLIN | EPOLLRDHUP | EPOLLOUT);
			}

			struct epoll_event evs[1];
			int nfds = epoll_wait(epfd, evs, 1, -1);
			if (nfds < 0)
				die("epoll_wait");
			uint32_t e = evs[0].events;

			if (e & EPOLLOUT) {
				wakeups++;
				if (wakeups <= 5 || wakeups % 100 == 0)
					printf("[server] EPOLLOUT 第 %u 次唤醒: 已发 %zu / %zu\n",
					       wakeups, sent, TOTAL_BYTES);
				continue; /* 回去继续 write, 直到再次 EAGAIN */
			}
			if (e & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
				/* -n 模式下这是 client 放弃后关闭连接 */
				printf("[server] 收到 0x%x: client 关闭/出错\n", e);
				if (g_no_epollout) {
					printf("[server] ============ 反证成立 ============\n");
					printf("[server] 卡死在 %zu 字节 (phase1=%zu)。没有监听 "
					       "EPOLLOUT, 即使 client 愿意读, 也永远不会有 "
					       "任何事件来唤醒我。\n",
					       sent, phase1);
					exit(0);
				}
				return;
			}
			continue;
		}
		if (n < 0) {
			perror("write");
			return;
		}
	}

	if (!g_no_epollout && need_out) {
		printf("[server] 全部发完, 把 EPOLLOUT 摘掉 (回到只监听 EPOLLIN)\n");
		epoll_mod(epfd, fd, EPOLLIN | EPOLLRDHUP);
	}
	printf("[server] ============ 结果 ============\n");
	printf("[server] 共发送 %zu 字节; 其中 phase1=%zu 字节不依赖 EPOLLOUT;\n",
	       TOTAL_BYTES, phase1);
	printf("[server] 剩余 %zu 字节完全靠 %u 次 EPOLLOUT 唤醒才得以发出。\n",
	       TOTAL_BYTES - phase1, wakeups);
}

static void run_server(int listen_fd)
{
	signal(SIGPIPE, SIG_IGN);

	int epfd = epoll_create1(0);
	epoll_add(epfd, listen_fd, EPOLLIN);

	struct epoll_event evs[1];
	printf("[server] 等待 client 连接...\n");
	if (epoll_wait(epfd, evs, 1, -1) < 0)
		die("epoll_wait listen");
	int conn = accept(listen_fd, NULL, NULL);
	if (conn < 0)
		die("accept");
	set_nonblocking(conn);

	/* 把 send buffer 调小, 让 EAGAIN 尽快出现, demo 更快更可控 */
	int sz = BUF_SIZE;
	if (setsockopt(conn, SOL_SOCKET, SO_SNDBUF, &sz, sizeof(sz)) < 0)
		die("SO_SNDBUF");
	socklen_t slen = sizeof(sz);
	getsockopt(conn, SOL_SOCKET, SO_SNDBUF, &sz, &slen);
	printf("[server] 接受连接; 实际 send buffer ≈ %d 字节 (内核会把设置值加倍)\n",
	       sz);
	close(listen_fd);

	epoll_add(epfd, conn, EPOLLIN | EPOLLRDHUP);
	flood(epfd, conn);
	close(conn);
}

static void run_client(int port)
{
	signal(SIGPIPE, SIG_IGN);

	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		die("socket");
	/* 同样把接收缓冲调小, 限制 TCP 飞行中的数据量 */
	int sz = BUF_SIZE;
	setsockopt(s, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz));
	set_nonblocking(s);

	struct sockaddr_in a = { 0 };
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = inet_addr(HOST);
	a.sin_port = htons(port);

	/* 场景 1: 非阻塞 connect 的完成只能靠 EPOLLOUT 得知 */
	int r = connect(s, (struct sockaddr *)&a, sizeof(a));
	if (r == 0) {
		printf("[client] connect() 同步完成(少见)\n");
	} else if (errno == EINPROGRESS) {
		int epfd = epoll_create1(0);
		epoll_add(epfd, s, EPOLLOUT);
		printf("[client] 非阻塞 connect() 返回 EINPROGRESS; 连接是否建立, "
		       "只能等 EPOLLOUT 通知\n");
		struct epoll_event evs[1];
		if (epoll_wait(epfd, evs, 1, 5000) < 0)
			die("epoll_wait connect");
		printf("[client] epoll_wait 被唤醒, events=0x%x\n", evs[0].events);

		int err = 0;
		socklen_t elen = sizeof(err);
		getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &elen);
		if (err != 0) {
			printf("[client] SO_ERROR=%d (%s): 连接失败也通过 EPOLLOUT/EPOLLERR 报告\n",
			       err, strerror(err));
			exit(1);
		}
		printf("[client] EPOLLOUT + SO_ERROR==0 => 连接建立成功\n");

		/*
		 * 水平触发下, 建立连接后 send buffer 永远是空的, EPOLLOUT 永远
		 * ready。下面用 timeout=0 的 epoll_wait 证明: 如果不摘掉 EPOLLOUT,
		 * 它会每次都立刻返回, 造成空转。
		 */
		struct epoll_event chk;
		int nn = epoll_wait(epfd, &chk, 1, 0);
		printf("[client] (证明) EPOLLOUT 还注册着时 epoll_wait(timeout=0) "
		       "立刻返回 %d 次 => 可写状态永远成立\n", nn);
		printf("[client] 所以 connect 完成后马上摘掉 EPOLLOUT, 只留 "
		       "EPOLLIN (读方不需要它)\n");
		epoll_mod(epfd, s, EPOLLIN | EPOLLRDHUP);
		close(epfd);
	} else {
		die("connect");
	}

	/* 场景 2 的铺垫: 先睡 3 秒不读, 让 server 的 send buffer 被填满 */
	printf("[client] 我先睡 %d 秒不读数据: server 会在这期间把 send buffer 写满并撞上 "
	       "EAGAIN\n", CLIENT_IDLE_S);
	sleep(CLIENT_IDLE_S);

	size_t got = 0;
	static char rbuf[READ_CHUNK];
	for (;;) {
		struct pollfd p = { .fd = s, .events = POLLIN };
		int pr = poll(&p, 1, STALL_TIMEOUT_MS);
		if (pr == 0) {
			printf("[client] 连续 %d ms 没有新数据: 只收到 %zu 字节。\n",
			       STALL_TIMEOUT_MS, got);
			if (g_no_epollout)
				printf("[client] 这正是 (-n) 反证: server 没监听 EPOLLOUT, "
				       "剩余数据永远不会再来, 我放弃\n");
			exit(1);
		}
		ssize_t n = read(s, rbuf, sizeof(rbuf));
		if (n > 0) {
			got += n;
			continue;
		}
		if (n == 0) {
			printf("[client] 收到 EOF, 共 %zu 字节 == 服务器发送总量: "
			       "传输完整完成\n", got);
			break;
		}
		if (n < 0 && errno == EAGAIN)
			continue;
		perror("read");
		break;
	}
	close(s);
	exit(0);
}

int main(int argc, char *argv[])
{
	int opt;
	while ((opt = getopt(argc, argv, "n")) != -1) {
		switch (opt) {
		case 'n':
			g_no_epollout = 1;
			break;
		default:
			printf("usage: %s [-n]\n  -n: server 拒绝注册 EPOLLOUT, 演示死锁\n",
			       argv[0]);
			exit(1);
		}
	}

	/* 先 bind/listen 拿到端口, 再通过 socketpair 把端口告诉子进程 */
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
		die("socket listen");
	int one = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	struct sockaddr_in a = { 0 };
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = inet_addr(HOST);
	a.sin_port = 0;
	if (bind(listen_fd, (struct sockaddr *)&a, sizeof(a)) < 0)
		die("bind");
	socklen_t alen = sizeof(a);
	getsockname(listen_fd, (struct sockaddr *)&a, &alen);
	if (listen(listen_fd, 8) < 0)
		die("listen");

	int sp[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sp) < 0)
		die("socketpair");
	int port = ntohs(a.sin_port);
	if (write(sp[0], &port, sizeof(port)) != sizeof(port))
		die("write port");
	fflush(stdout);

	pid_t pid = fork();
	if (pid < 0)
		die("fork");

	if (pid == 0) {
		/* child: client */
		close(sp[0]);
		close(listen_fd);
		int p;
		if (read(sp[1], &p, sizeof(p)) != sizeof(p))
			die("read port");
		close(sp[1]);
		run_client(p);
		_exit(0);
	}

	/* parent: server */
	close(sp[1]);
	close(sp[0]);
	run_server(listen_fd);
	waitpid(pid, NULL, 0);
	return 0;
}
