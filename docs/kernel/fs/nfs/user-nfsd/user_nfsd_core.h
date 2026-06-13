#ifndef USER_NFSD_CORE_H
#define USER_NFSD_CORE_H

#include <stddef.h>

#define UNFS_MAX_PATH 1024
#define UNFS_FHSIZE 64

struct unfs_fh {
	unsigned int len;
	unsigned char data[UNFS_FHSIZE];
};

int unfs_fh_from_path(const char *path, struct unfs_fh *fh);
int unfs_path_from_fh(const struct unfs_fh *fh, char *path, size_t path_len);
int unfs_join_export_path(const char *export_root, const char *relpath,
			  char *out, size_t out_len);
int unfs_path_is_safe(const char *path);

#endif
