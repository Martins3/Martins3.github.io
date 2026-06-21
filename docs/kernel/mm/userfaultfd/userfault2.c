/*
 * copied from : https://www.cons.org/cracauer/cracauer-userfaultfd.html
 */

/*
 * Example program about using userfaultfd(2) for garbage collection.
 *
 * This establishes a couple pages, all of which are filled from
 * compressed files on disk when first accessed.  For simplicity these are
 * one file per page.  Files are written at the beginning of the program.
 *
 * Later, this program demonstrates the use of write protection to get
 * a notification on write access, analogous to using mprotect(!PROT_WRITE)
 * and doing the bookkeeping in a SIGSEGV handler.
 *
 */

#include <linux/userfaultfd.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <strings.h>

#include <unistd.h>
#include <asm/unistd.h>

#include <poll.h>
#include <pthread.h>

const int pagesize = 4096;

// Bookkeeping about pages.  Part of the GC, not of userfaultfd handling
struct page_attr {
	int has_been_brought_in_p;
	int has_been_written_when_wp_p;
};

// Random parameters we pass to threads instead of using globals
struct pass2uffd_thread {
	int fd;
	long long *begin;
	size_t size;
	size_t n_pages;
	struct page_attr *pages;
};

void floatsleep(float seconds)
{
	struct timespec req;
	req.tv_sec = floor(seconds);
	req.tv_nsec = (float)(seconds - req.tv_sec) * 1000000000;
	nanosleep(&req, NULL);
}

// This is doing the work in the uffd handler thread
void *handler(void *data)
{
	struct pass2uffd_thread *params = data;
	int fd = params->fd;

	printf("\tthread: fd is %d\n", fd);

	for (;;) {
		struct uffd_msg msg;

		struct pollfd pollfd[1];
		pollfd[0].fd = params->fd;
		pollfd[0].events = POLLIN;
		int pollres;

		pollres = poll(pollfd, 1, -1);
		switch (pollres) {
		case -1:
			perror("poll userfaultfd");
			continue;
			break;
		case 0:
			continue;
			break;
		case 1:
			break;
		default:
			fprintf(stderr, "got %d fds out of poll\n", pollres);
			exit(2);
		}
		if (pollfd[0].revents & POLLERR) {
			fprintf(stderr, "POLLERR on userfaultfd\n");
			exit(1);
		}
		if (!(pollfd[0].revents & POLLIN)) {
			continue;
		}

		int readret;
		readret = read(fd, &msg, sizeof(msg));
		if (readret == -1) {
			if (errno == EAGAIN)
				continue;
			perror("read userfaultfd");
		}
		if (readret != sizeof(msg)) {
			fprintf(stderr, "short read, not expected, exiting\n");
			exit(1);
		}

		if (msg.event == UFFD_EVENT_PAGEFAULT)
			printf("\t==> Event is pagefault on %p flags 0x%llx write? 0x%llx wp? 0x%llx\n",
			       (void *)msg.arg.pagefault.address,
			       msg.arg.pagefault.flags,
			       msg.arg.pagefault.flags &
				       UFFD_PAGEFAULT_FLAG_WRITE,
			       msg.arg.pagefault.flags &
				       UFFD_PAGEFAULT_FLAG_WP);

		long long addr = msg.arg.pagefault.address;
		long long page_begin = addr - (addr % pagesize);
		long long whichpage =
			(page_begin - (long long)params->begin) / pagesize;
		if (whichpage > params->n_pages) {
			fprintf(stderr, "Page %lld too high\n", whichpage);
			exit(1);
		}
		printf("\tMessing with page %lld\n", whichpage);

		/*
		 * Proper sequence is important here.
     		 *
     		 * For the GC we expect that write-protected pages can only
     		 * be pages already backed by physical pages.
     		 * Regular writes into unprotected pages that come before
     		 * reads need the page be filled.
     		 *
     		 * So we do the WP case first and get it out of the way.
     		 * Then both of the other cases need the page read.
     		 */

		if (msg.arg.pagefault.flags & UFFD_PAGEFAULT_FLAG_WP) {
			//  send write unlock
			struct uffdio_writeprotect wp;
			wp.range.start = (long long)params->begin;
			wp.range.len = (long long)params->size;
			wp.mode = 0;
			/*
			 * 这里不能 wp.mode = UFFDIO_WRITEPROTECT_MODE_WP ，需要将 wp mode 解除
			 */
			printf("\tsending !UFFDIO_WRITEPROTECT event to userfaultfd\n");
			fflush(stdout);
			if (ioctl(fd, UFFDIO_WRITEPROTECT, &wp) == -1) {
				perror("ioctl(UFFDIO_WRITEPROTECT)");
			}
			params->pages[whichpage].has_been_written_when_wp_p = 1;
			continue;
		}
		// Page has never been filled, so do that now.
		// Note that this relies on user only write-protecting pages
		// after they have been filled.  That won't be the case
		// in a real GC.

		// TODO 非要使用这么复杂的 popen 来做测试吗?
		FILE *f;
		char cmdname[8192];
		snprintf(cmdname, sizeof(cmdname), "zcat tmp%lld.gz",
			 whichpage);
		f = popen(cmdname, "r");
		if (f == NULL) {
			perror("popen zcat");
			exit(1);
		}
		char buf[pagesize];
		if (fread(buf, pagesize, 1, f) == 0) {
			perror("fread");
			exit(1);
		}
		if (fclose(f)) {
			perror("fclose");
			exit(1);
		}

		struct uffdio_copy cp;
		cp.src = (long long)buf;
		cp.dst = (long long)addr;
		cp.len = (long long)pagesize;
		cp.mode = 0; // fixme - is there a symbol for this?
		printf("\tsending UFFDIO_COPY event to userfaultfd\n");
		fflush(stdout);
		if (ioctl(fd, UFFDIO_COPY, &cp) == -1) {
			perror("ioctl(UFFDIO_COPY)");
		}
	}
	return NULL;
}

// this function can be thread main body or directly run
void *do_some_work(void *data)
{
	struct pass2uffd_thread *params = data;

	long long *region = params->begin;

	floatsleep(0.2);
	printf("worker writing into write-protected area at %p\n", region);
	fflush(stdout);
	*region = 43;
	printf("I survived that\n");
	fflush(stdout);
	floatsleep(0.2);

	return NULL;
}

void write_testfiles()
{
	long long word = 0xDEADBEEFDEADBEEF;
	FILE *f;
	char *cmds[] = { "gzip > tmp0.gz", "gzip > tmp1.gz", "gzip > tmp2.gz",
			 "gzip > tmp3.gz", NULL };
	char **cmd;

	for (cmd = cmds; *cmd; cmd++) {
		f = popen(*cmd, "w");
		if (f == NULL) {
			perror("popen gzip");
		}
		int i;
		for (i = 0; i < pagesize / sizeof(word); i++) {
			if (!fwrite(&word, sizeof(word), 1, f)) {
				perror("fwrite");
				exit(1);
			}
		}
		if (fclose(f)) {
			perror("fclose");
			exit(1);
		}
	}
}

int main(int argc, char *argv[])
{
	long long *region;
	const int n_pages = 128;
	pthread_t uffd_thread;
	int uffd;

	write_testfiles();

	printf("userfaultf syscall #: %d\n", __NR_userfaultfd);

	/*
	 * 使用 UFFD_USER_MODE_ONLY 就可以实现
	 *
	 * uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK | UFFD_USER_MODE_ONLY);
	 *
	 * */
	uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
	if (uffd == -1) {
		perror("syscall");
		exit(2);
	}

	int uffd_flags;
	uffd_flags = fcntl(uffd, F_GETFD, NULL);
	printf("userfaultfd flags: 0x%llX, fd is %d\n", (long long)uffd_flags,
	       uffd);

	struct uffdio_api uffdio_api;
	uffdio_api.api = UFFD_API;
	uffdio_api.features = 0;
	if (ioctl(uffd, UFFDIO_API, &uffdio_api)) {
		fprintf(stderr, "UFFDIO_API\n");
		return 1;
	}
	printf("Features: 0x%llx\n", uffdio_api.features);
	if (uffdio_api.api != UFFD_API) {
		fprintf(stderr, "UFFDIO_API error %Lu\n", uffdio_api.api);
		return 1;
	}

#if 0
  printf("userfaultfd api: 0x%llX -> 0x%llX\n", UFFD_API, uffdio_api.api);
  printf("userfaultfd ioctls: 0x%llX (0x%llx)\n", uffdio_api.ioctls,
	 (long long)UFFDIO_REGISTER);
#endif

	region = (long long *)mmap(NULL, pagesize * n_pages,
				   PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANON, -1, 0);
	if (!region) {
		perror("mmap");
		exit(2);
	}
	if (posix_memalign((void **)region, pagesize, pagesize * n_pages)) {
		fprintf(stderr, "cannot align by pagesize %d\n", pagesize);
		exit(1);
	}
	printf("mapped at %p - %p\n", region, region + pagesize * n_pages);

	struct uffdio_register uffdio_register;
	uffdio_register.range.start = (unsigned long)region;
	uffdio_register.range.len = pagesize * n_pages;

	uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING |
			       UFFDIO_REGISTER_MODE_WP;

	if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1) {
		perror("ioctl(UFFDIO_REGISTER)");
		exit(1);
	}
	printf("userfaultfd ioctls: 0x%llx\n", uffdio_register.ioctls);

/* 安装的 kernel header 和  kernel 版本不一致 ?
 */
#ifdef SKIP
	const int expected = UFFD_API_RANGE_IOCTLS;
	if ((uffdio_register.ioctls & expected) != expected) {
		fprintf(stderr, "ioctl set is incorrect %llx vs %x \n",
			uffdio_register.ioctls, expected);
		exit(1);
	}
#endif

	// Our bookkeeping.  Part of the GC, has nothing to do with
	// userfaultfd.  Updated in the uffd thread.
	struct page_attr *pages;
	pages = malloc(n_pages * sizeof(struct page_attr));
	if (pages == NULL) {
		perror("malloc");
		exit(1);
	}
	bzero(pages, n_pages * sizeof(struct page_attr));

	/*
	 * Set up and start uffd thread.
         */
	struct pass2uffd_thread thr_params;
	thr_params.fd = uffd;
	thr_params.begin = region;
	thr_params.size = pagesize * n_pages;
	thr_params.n_pages = n_pages;
	thr_params.pages = pages;
	pthread_create(&uffd_thread, NULL, handler, &thr_params);

	printf("mainline testing read on page 0\n");
	fflush(stdout);
	printf("region first word currently is: [0x%llx]\n", *region);

	printf("mainline testing read on page 2\n");
	fflush(stdout);
	printf("region first word currently is: [0x%llx]\n",
	       *(region + 2 * pagesize / sizeof(*region)));

	printf("mainline writing writable page 1\n");
	fflush(stdout);
	*(region + pagesize / sizeof(*region)) = 0x42;

	// test write protect on first page
	struct uffdio_writeprotect wp;
	wp.range.start = (unsigned long)region;
	wp.range.len = pagesize * n_pages;
	wp.mode = UFFDIO_WRITEPROTECT_MODE_WP;
	if (ioctl(uffd, UFFDIO_WRITEPROTECT, &wp) == -1) {
		perror("ioctl(UFFDIO_WRITEPROTECT)");
		exit(1);
	}

	printf("worker read write-protected page 0\n");
	printf("page 0 : 0x%llx/0x%llx\n", *region, *(region + 1));

	printf("worker writing into write-protected page 0\n");
	fflush(stdout);
	*region = 0x43;
	printf("I survived that\n");
	fflush(stdout);

	if (ioctl(uffd, UFFDIO_UNREGISTER, &uffdio_register.range)) {
		fprintf(stderr, "ioctl unregister failure\n");
		return 1;
	}

	printf("pages first words currently are:\n"
	       "0x%llx/0x%llx 0x%llx/0x%llx 0x%llx/0x%llx \n",
	       *region, *(region + 1), *(region + pagesize / sizeof(*region)),
	       *(region + 1 + pagesize / sizeof(*region)),
	       *(region + 2 * pagesize / sizeof(*region)),
	       *(region + 1 + 2 * pagesize / sizeof(*region)));

	free(pages);
	// fixme - various other cleanup
	return 0;
}
