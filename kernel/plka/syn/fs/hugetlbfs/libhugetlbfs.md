https://github.com/libhugetlbfs/libhugetlbfs 源代码分析

```c
int hugetlbfs_unlinked_fd(void)
{
	long hpage_size = gethugepagesize(); // 询问默认大小
	if (hpage_size > 0)
		return hugetlbfs_unlinked_fd_for_size(hpage_size); // 
	else
		return -1;
}


int hugetlbfs_unlinked_fd_for_size(long page_size)
{
	const char *path;
	char name[PATH_MAX+1];
	int fd;

	path = hugetlbfs_find_path_for_size(page_size); // ?
	if (!path)
		return -1;

	name[sizeof(name)-1] = '\0';

	strcpy(name, path);
	strncat(name, "/libhugetlbfs.tmp.XXXXXX", sizeof(name)-1);
	/* FIXME: deal with overflows */

  // 创建文件
	fd = mkstemp64(name);

	if (fd < 0) {
		ERROR("mkstemp() failed: %s\n", strerror(errno));
		return -1;
	}

  // If the name was the last link to a file but any processes still have the file open, the file will remain in existence until the last file descriptor referring to it is closed.
  // 立刻隐藏下来
	unlink(name);

	return fd;
}

const char *hugetlbfs_find_path_for_size(long page_size)
{
	char *path;
	int idx;

	idx = hpage_size_to_index(page_size);
	if (idx >= 0) {
		path = hpage_sizes[idx].mount; // 所以这个 path 没有被处理 ?

		if (strlen(path))
			return path;
	}
	return NULL;
}

/* Multiple huge page size support */
struct hpage_size {
	unsigned long pagesize;
	char mount[PATH_MAX+1];
};
```
