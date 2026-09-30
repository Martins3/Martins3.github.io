/*
 * 回答两个问题:
 *   1. 手里只有 pty master fd(/proc/PID/fd 里只显示 /dev/ptmx),它对应哪个 /dev/pts/N?
 *   2. 这个 /dev/pts/N 到底是谁在用?
 *
 * 做法:扫一遍目标进程的 /proc/PID/fd,把每个 /dev/ptmx fd "偷" 到自己进程里
 * (pidfd_getfd(2))再 TIOCGPTN 拿到 N;然后扫一遍全机 /proc,谁的 fd 开在
 * /dev/pts/N 上、谁拿它当控制终端,谁就是使用者。
 * 不用 gdb attach,不用改目标进程代码,也不用事先知道 fd 号。
 *
 *   ./tiocgptn.out <pid>
 *
 * 实测:
 *   $ ./tiocgptn.out 2399319
 *   pid 2399319 fd 13 (/dev/ptmx) -> /dev/pts/121
 *   	pid 76644    (zsh)             fds 0,1,2,10  ctty
 *   	pid 1978060  (node)            fds 0,1,2  ctty
 *   	pid 1978245  (node_repl)         ctty
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_FDS 32 /* 一个进程对同一个 pts 开这么多 fd 够看了 */

struct master {
	pid_t pid;
	int fd;
	unsigned pts;
};

struct user {
	pid_t pid;
	unsigned pts;
	int ctty; /* 拿它当控制终端 */
	int fds[MAX_FDS];
	int nfds;
	char comm[16];
};

static struct master masters[256];
static int nmaster;
static struct user users[1024];
static int nuser;

/* 让输出按 fd 号排,不然 readdir 的顺序是乱的 */
static int fd_cmp(const struct dirent **a, const struct dirent **b)
{
	return atoi((*a)->d_name) - atoi((*b)->d_name);
}

static int user_cmp(const void *a, const void *b)
{
	const struct user *x = a, *y = b;
	if (x->pts != y->pts)
		return x->pts - y->pts;
	return x->pid - y->pid;
}

/* 只关心命令行那几个进程手里的 pts */
static int wanted(unsigned pts)
{
	for (int i = 0; i < nmaster; i++)
		if (masters[i].pts == pts)
			return 1;
	return 0;
}

static void comm_of(pid_t pid, char *out, size_t sz)
{
	char path[64], buf[64];

	sprintf(path, "/proc/%d/comm", (int)pid);
	int fd = open(path, O_RDONLY);
	ssize_t n = fd < 0 ? -1 : read(fd, buf, sizeof(buf) - 1);
	if (fd >= 0)
		close(fd);
	if (n <= 0) {
		snprintf(out, sz, "?");
		return;
	}
	buf[n] = '\0';
	buf[strcspn(buf, "\n")] = '\0';
	snprintf(out, sz, "%s", buf);
}

/*
 * proc/pid/stat 第 7 个字段 tty_nr;
 *
 * man proc_pid_stat(5) 中:
 *
 *               (7) tty_nr %d
 *                    The controlling terminal of the process.  (The minor
 *                    device number is contained in the combination of
 *                    bits 31 to 20 and 7 to 0; the major device number is
 *                    in bits 15 to 8.)
 *
 * 内核中的实现为:
 * 			tty_nr = new_encode_dev(tty_devnum(sig->tty));
 */
static int tty_nr_of(pid_t pid)
{
	char path[64], buf[512];

	sprintf(path, "/proc/%d/stat", (int)pid);
	int fd = open(path, O_RDONLY);
	ssize_t n = fd < 0 ? -1 : read(fd, buf, sizeof(buf) - 1);
	if (fd >= 0)
		close(fd);
	if (n <= 0)
		return -1;
	buf[n] = '\0';
	char *p = strrchr(buf, ')');
	if (!p)
		return -1;
	/*
	 * comm 里可能有空格和括号,从最后一个 ')' 往后数
	 * ')' 后面是 state ppid pgrp session tty_nr ...
	 */
	int tty = -1;
	sscanf(p, ") %*c %*d %*d %*d %d", &tty);
	return tty;
}

/* /proc 里的 tty_nr 是 huge_encode_dev() 的结果:pts 的 major 固定是 136 */
static unsigned pts_dev(unsigned n)
{
	unsigned major = 136, minor = n;

	return (minor & 0xff) | (major << 8) | ((minor & ~0xff) << 12);
}

static struct user *user_get(pid_t pid, unsigned pts)
{
	for (int i = 0; i < nuser; i++)
		if (users[i].pid == pid && users[i].pts == pts)
			return &users[i];
	if (nuser >= (int)(sizeof(users) / sizeof(users[0])))
		return NULL;
	struct user *u = &users[nuser++];

	memset(u, 0, sizeof(*u));
	u->pid = pid;
	u->pts = pts;
	comm_of(pid, u->comm, sizeof(u->comm));
	return u;
}

static void user_add_fd(struct user *u, int fd)
{
	if (!u || u->nfds >= MAX_FDS)
		return;
	int i = u->nfds++;

	while (i > 0 && u->fds[i - 1] > fd) {
		u->fds[i] = u->fds[i - 1];
		i--;
	}
	u->fds[i] = fd;
}

/* 找 master:目标进程每个 ptmx fd 都偷过来问一句 TIOCGPTN */
static void collect_masters(pid_t pid)
{
	/* 先拿到 pidfd,之后从它里面往外偷 fd */
	int pidfd = syscall(SYS_pidfd_open, pid, 0);
	if (pidfd < 0) {
		perror("pidfd_open");
		return;
	}

	char path[64];
	sprintf(path, "/proc/%d/fd", (int)pid);

	struct dirent **fds = NULL;
	int nfds = scandir(path, &fds, NULL, fd_cmp);
	if (nfds < 0) {
		perror("scandir");
		close(pidfd);
		return;
	}

	int found = 0;
	for (int i = 0; i < nfds; i++) {
		int target_fd = atoi(fds[i]->d_name);
		char link[128], fdpath[64];

		sprintf(fdpath, "/proc/%d/fd/%d", (int)pid, target_fd);
		ssize_t len = readlink(fdpath, link, sizeof(link) - 1);
		if (len < 0)
			goto next;
		link[len] = '\0';
		/* master 打开的是 devpts 的共享节点,proc 里只显示 /dev/ptmx */
		if (!strstr(link, "ptmx"))
			goto next;

		int fd = syscall(SYS_pidfd_getfd, pidfd, target_fd, 0);
		if (fd < 0) {
			perror("pidfd_getfd");
			goto next;
		}

		/* TIOCGPTN 在 drivers/tty/pty.c 里就是 put_user(tty->index, ...)
		 * 只有 master 答得上来(slave 返回 ENOTTY) */
		unsigned int n = 0;
		if (ioctl(fd, TIOCGPTN, &n) < 0) {
			perror("TIOCGPTN");
			close(fd);
			goto next;
		}
		close(fd);

		if (nmaster < (int)(sizeof(masters) / sizeof(masters[0])))
			masters[nmaster++] =
				(struct master){ pid, target_fd, n };
		found = 1;
next:
		free(fds[i]);
	}
	free(fds);
	close(pidfd);

	if (!found)
		printf("pid %d: no ptmx fd\n", (int)pid);
}

/* 扫全机 /proc:fd 开在 /dev/pts/N 上的,和拿 /dev/pts/N 当控制终端的 */
static void collect_users(void)
{
	DIR *proc = opendir("/proc");
	struct dirent *e;

	if (!proc) {
		perror("opendir /proc");
		return;
	}
	while ((e = readdir(proc))) {
		if (e->d_name[0] < '0' || e->d_name[0] > '9')
			continue;
		pid_t pid = atoi(e->d_name);

		/* 控制终端:连 stdio 都重定向走了的进程也躲不掉 */
		int tty = tty_nr_of(pid);
		for (int i = 0; i < nmaster; i++) {
			struct user *u;

			if (tty != (int)pts_dev(masters[i].pts))
				continue;
			u = user_get(pid, masters[i].pts);
			if (u)
				u->ctty = 1;
		}

		/* fd 表 */
		char fddir[64];
		sprintf(fddir, "/proc/%d/fd", (int)pid);
		DIR *fds = opendir(fddir);
		struct dirent *f;

		if (!fds)
			continue;
		while ((f = readdir(fds))) {
			if (f->d_name[0] < '0' || f->d_name[0] > '9')
				continue;
			int target_fd = atoi(f->d_name);
			char link[128], fdpath[80];

			sprintf(fdpath, "%s/%d", fddir, target_fd);
			ssize_t len = readlink(fdpath, link, sizeof(link) - 1);
			if (len < 0)
				continue;
			link[len] = '\0';
			if (strncmp(link, "/dev/pts/", 9) != 0)
				continue;
			unsigned pts = atoi(link + 9);
			if (wanted(pts))
				user_add_fd(user_get(pid, pts), target_fd);
		}
		closedir(fds);
	}
	closedir(proc);
	qsort(users, nuser, sizeof(users[0]), user_cmp);
}

static void print_users(unsigned pts)
{
	int found = 0;

	for (int i = 0; i < nuser; i++) {
		struct user *u = &users[i];
		char who[32];

		if (u->pts != pts)
			continue;
		snprintf(who, sizeof(who), "(%s)", u->comm);
		printf("\tpid %-8d %-18s", (int)u->pid, who);
		if (u->nfds) {
			printf("fds ");
			for (int j = 0; j < u->nfds; j++)
				printf("%s%d", j ? "," : "", u->fds[j]);
		}
		if (u->ctty)
			printf("  ctty");
		putchar('\n');
		found = 1;
	}
	if (!found)
		printf("\t(no user)\n");
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <pid>\n", argv[0]);
		return 1;
	}

	pid_t pid = atoi(argv[1]);

	collect_masters(pid);
	collect_users();

	for (int i = 0; i < nmaster; i++) {
		printf("pid %d fd %d (/dev/ptmx) -> /dev/pts/%u\n",
		       (int)masters[i].pid, masters[i].fd, masters[i].pts);
		print_users(masters[i].pts);
	}
	return 0;
}
