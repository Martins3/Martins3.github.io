#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <hugetlbfs.h>

char *find_hugetlbfs(char *fsmount, int len)
{
	char format[256];
	char fstype[256];
	char *ret = NULL;
	FILE *fd;

	snprintf(format, 255, "%%*s %%%ds %%255s %%*s %%*d %%*d", len);

	fd = fopen("/proc/mounts", "r");
	if (!fd) {
		perror("fopen");
		return NULL;
	}

	while (fscanf(fd, format, fsmount, fstype) == 2) {
		if (!strcmp(fstype, "hugetlbfs")) {
      printf("%s\n", fsmount);
			ret = fsmount;
		}
	}

	fclose(fd);
	return ret;
}

int main() {
	char buffer[PATH_MAX+1];
  printf("default huge tlb size : %ld\n", gethugepagesize());
  printf("default find path : %s\n", hugetlbfs_find_path()); // 按道理来说，应该是可以正确返回才对的
	printf("hugetlbfs mounted at %s\n", find_hugetlbfs(buffer, PATH_MAX));
	return 0;
}
