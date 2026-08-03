#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../user/sysfs.h"
#include "../user/lib.h"

static void user_test_iter_pte()
{
	long pagesize = sysconf(_SC_PAGESIZE);
	void *ptr = mmap(NULL, pagesize, PROT_READ | PROT_WRITE,
			 MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
	para_write(0, (long)ptr);
	// pte hole
	sysfs_write(2);

	// 是只有 cache 不命中才会，还是 tlb 不命中才会配置 idle ?
	char *m = (char *)ptr;
	*m = 'a';
	// pte access
	sysfs_write(2);
	// pte idle
	sysfs_write(2);

	// 之后即便是 access 过，这里也是 idle 的，应该是需要 flush TLB 才可以
	for (size_t i = 0; i < 3; i++) {
		*m = 'a' + i;
		sysfs_write(2);
	}

	sleep(1);
}

int main(int argc, char *argv[])
{
	int test = atoi(argv[1]);
	if (test == 2)
		user_test_iter_pte();

	return 0;
}
