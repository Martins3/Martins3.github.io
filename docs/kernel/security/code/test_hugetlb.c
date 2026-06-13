#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>

int main(void)
{
	int shmid;
	size_t size = 2 * 1024 * 1024; // 2MB, one hugepage

	printf("uid=%d gid=%d euid=%d egid=%d\n", getuid(), getgid(), geteuid(),
	       getegid());
	printf("hugetlb_shm_group = ");
	fflush(stdout);
	system("cat /proc/sys/vm/hugetlb_shm_group");

	shmid = shmget(IPC_PRIVATE, size, SHM_HUGETLB | 0600);
	if (shmid == -1) {
		printf("shmget(SHM_HUGETLB) FAILED: %s (errno=%d)\n",
		       strerror(errno), errno);
		return 1;
	}

	printf("shmget(SHM_HUGETLB) SUCCESS: shmid=%d\n", shmid);

	if (shmctl(shmid, IPC_RMID, NULL) == -1) {
		perror("shmctl IPC_RMID");
		return 1;
	}
	printf("shmctl IPC_RMID ok\n");
	return 0;
}
