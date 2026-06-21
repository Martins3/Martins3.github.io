// 拷贝于 https://liujunming.top/2022/07/24/Introduction-to-userfaultfd-mechanism/
// 他的源头是:
// https://brieflyx.me/2020/linux-tools/userfaultfd-internals/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>

#include <unistd.h>
#include <asm/unistd.h>

#include <poll.h>
#include <pthread.h>
#include <linux/userfaultfd.h>

static void errExit(const char *msg)
{
	perror(msg);
	exit(1);
}

unsigned long page_size;
void *page;
int fault_cnt;
void *userfaultfd_area;

static void register_area(int uffd, unsigned long addr, unsigned long len)
{
	/* Register the memory range of the mapping we just created for
          handling by the userfaultfd object. In mode, we request to track
          missing pages (i.e., pages that have not yet been faulted in). */
	struct uffdio_register uffdio_register;
	uffdio_register.range.start = (unsigned long)addr;
	uffdio_register.range.len = len;
	uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
	if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
		errExit("ioctl UFFDIO_REGISTER");
}

static void handle_fault_with_copy(int uffd, unsigned long dst)
{
	memset(page, 'A' + fault_cnt % 26, page_size);

	struct uffdio_copy uffdio_copy;
	uffdio_copy.src = (unsigned long)page;
	uffdio_copy.dst = dst;
	uffdio_copy.len = page_size;
	// TODO 这里的 mode 的含义是什么，发现如果 mode 是 UFFDIO_REGISTER_MODE_MISSING
	// 那么 ioctl 会成功，但是什么其实什么都不做的
	uffdio_copy.mode = 0;

	/* We need to handle page faults in units of pages(!).
	 * So, round faulting address down to page boundary
	 */
	if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy))
		errExit("ioctl UFFDIO_COPY");
}

// tools/testing/selftests/mm/uffd-common.c
static void handle_fault_with_move(int uffd, unsigned long dst)
{
	void *move_src = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
			      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (move_src == MAP_FAILED)
		errExit("mmap");

	memset(move_src, 'B' + fault_cnt % 26, page_size);

	struct uffdio_move uffdio_move;
	uffdio_move.src = (unsigned long)move_src;
	uffdio_move.dst = dst;
	uffdio_move.len = page_size;
	uffdio_move.mode = UFFDIO_MOVE_MODE_ALLOW_SRC_HOLES;
	uffdio_move.move = 0;
	if (ioctl(uffd, UFFDIO_MOVE, &uffdio_move))
		errExit("ioctl UFFDIO_MOVE");
}

static void handle_fault(int uffd, struct uffd_msg *msg)
{
	unsigned long dst = (unsigned long)msg->arg.pagefault.address &
			    ~(page_size - 1);
	switch ((dst - (unsigned long)userfaultfd_area) / page_size) {
	case 0:
		printf("handle COPY\n");
		handle_fault_with_copy(uffd, dst);
		printf("handle COPY finished\n");
		break;
	case 1:
		printf("handle MOVE\n");
		handle_fault_with_move(uffd, dst);
		break;
	default:
		errExit("impossible\n");
	}
	fault_cnt++;
}

static void *monitor_thread(void *arg)
{
	int nread;
	struct uffd_msg msg;
	int uffd = *(int *)arg;
	for (;;) {
		/* See what poll() tells us about the userfaultfd */

		struct pollfd pollfd;
		int nready;
		pollfd.fd = uffd;
		pollfd.events = POLLIN;
		nready = poll(&pollfd, 1, -1);
		if (nready == -1)
			errExit("poll");

		printf("\nfault_handler_thread():\n");
		printf("    poll() returns: nready = %d; "
		       "POLLIN = %d; POLLERR = %d\n",
		       nready, (pollfd.revents & POLLIN) != 0,
		       (pollfd.revents & POLLERR) != 0);

		/* Read an event from the userfaultfd */
		nread = read(uffd, &msg, sizeof(msg));
		if (nread == 0) {
			printf("EOF on userfaultfd!\n");
			exit(EXIT_FAILURE);
		}

		if (nread == -1)
			errExit("read");

		handle_fault(uffd, &msg);
	}
	return NULL;
}

static int create()
{
	int uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
	if (uffd == -1)
		errExit("userfaultfd");

	// TODO 原来还需要 ioctl 吗?
	struct uffdio_api uffdio_api;
	uffdio_api.api = UFFD_API;
	uffdio_api.features = 0;
	if (ioctl(uffd, UFFDIO_API, &uffdio_api))
		errExit("ioctl UFFDIO_API");

	return uffd;
}

int main(int argc, char *argv[])
{
	page_size = sysconf(_SC_PAGESIZE);
	int uffd = create();
	// 分配两个 page ，一个用 copy ，一个用 move
	userfaultfd_area = mmap(NULL, page_size * 2, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (userfaultfd_area == MAP_FAILED)
		errExit("mmap");
	page = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (page == MAP_FAILED)
		errExit("mmap");
	register_area(uffd, (unsigned long)userfaultfd_area, page_size * 2);

	pthread_t thr;
	int s = pthread_create(&thr, NULL, monitor_thread, (void *)&uffd);
	if (s != 0) {
		errno = s;
		errExit("pthread_create");
	}

	// 在 main thred 中进行测试
	char *a;

	printf("[martins3:%s:%d] %lx\n", __FUNCTION__, __LINE__,
	       (unsigned long)userfaultfd_area);
	a = (char *)(userfaultfd_area + page_size);
	*(a + 10) = '\0';
	printf("%s\n", a);

	a = (char *)userfaultfd_area;
	*(a + 10) = '\0';
	printf("%s\n", a);

	printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return EXIT_SUCCESS;
}
