#include "mount3.h"
#include "nfs3.h"
#include "user_nfsd_core.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <rpc/rpc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_NFS_PORT 3049
#define DEFAULT_MOUNT_PORT 3050
#define DEFAULT_MODE 0644

extern void nfs_program_3(struct svc_req *rqstp, SVCXPRT *transp);
extern void mountprog_3(struct svc_req *rqstp, SVCXPRT *transp);

static char export_root[PATH_MAX];
static char *read_buf;
static entry3 readdir_entries[256];
static char readdir_names[256][NFS3_MAXNAMLEN + 1];

static nfsstat3 errno_to_nfs3(int err)
{
	switch (err) {
	case 0:
		return NFS3_OK;
	case EPERM:
		return NFS3ERR_PERM;
	case ENOENT:
		return NFS3ERR_NOENT;
	case EACCES:
		return NFS3ERR_ACCES;
	case EEXIST:
		return NFS3ERR_EXIST;
	case EXDEV:
		return NFS3ERR_XDEV;
	case ENODEV:
		return NFS3ERR_NODEV;
	case ENOTDIR:
		return NFS3ERR_NOTDIR;
	case EISDIR:
		return NFS3ERR_ISDIR;
	case EINVAL:
		return NFS3ERR_INVAL;
	case EFBIG:
		return NFS3ERR_FBIG;
	case ENOSPC:
		return NFS3ERR_NOSPC;
	case EROFS:
		return NFS3ERR_ROFS;
	case EMLINK:
		return NFS3ERR_MLINK;
	case ENAMETOOLONG:
		return NFS3ERR_NAMETOOLONG;
	case ENOTEMPTY:
		return NFS3ERR_NOTEMPTY;
	case EOPNOTSUPP:
		return NFS3ERR_NOTSUPP;
	default:
		return NFS3ERR_IO;
	}
}

static mountstat3 errno_to_mount3(int err)
{
	switch (err) {
	case 0:
		return MNT3_OK;
	case EPERM:
		return MNT3ERR_PERM;
	case ENOENT:
		return MNT3ERR_NOENT;
	case EACCES:
		return MNT3ERR_ACCES;
	case ENOTDIR:
		return MNT3ERR_NOTDIR;
	case EINVAL:
		return MNT3ERR_INVAL;
	case ENAMETOOLONG:
		return MNT3ERR_NAMETOOLONG;
	default:
		return MNT3ERR_IO;
	}
}

static void nfstime_from_stat(nfstime3 *dst, const struct timespec *src)
{
	dst->seconds = (uint32)src->tv_sec;
	dst->nseconds = (uint32)src->tv_nsec;
}

static ftype3 ftype_from_mode(mode_t mode)
{
	if (S_ISREG(mode))
		return NF3REG;
	if (S_ISDIR(mode))
		return NF3DIR;
	if (S_ISBLK(mode))
		return NF3BLK;
	if (S_ISCHR(mode))
		return NF3CHR;
	if (S_ISLNK(mode))
		return NF3LNK;
	if (S_ISSOCK(mode))
		return NF3SOCK;
	if (S_ISFIFO(mode))
		return NF3FIFO;
	return NF3REG;
}

static void fill_fattr(const struct stat *st, fattr3 *attr)
{
	memset(attr, 0, sizeof(*attr));
	attr->type = ftype_from_mode(st->st_mode);
	attr->mode = (uint32)(st->st_mode & 07777);
	attr->nlink = (uint32)st->st_nlink;
	attr->uid = (uint32)st->st_uid;
	attr->gid = (uint32)st->st_gid;
	attr->size = (uint64)st->st_size;
	attr->used = (uint64)st->st_blocks * 512;
	attr->fsid = (uint64)st->st_dev;
	attr->fileid = (uint64)st->st_ino;
	nfstime_from_stat(&attr->atime, &st->st_atim);
	nfstime_from_stat(&attr->mtime, &st->st_mtim);
	nfstime_from_stat(&attr->ctime, &st->st_ctim);
}

static void fill_post_attr(const char *path, post_op_attr *attr)
{
	struct stat st;

	if (lstat(path, &st) == 0) {
		attr->attributes_follow = TRUE;
		fill_fattr(&st, &attr->post_op_attr_u.attributes);
	} else {
		attr->attributes_follow = FALSE;
	}
}

static void fill_pre_attr_from_stat(const struct stat *st, pre_op_attr *attr)
{
	attr->attributes_follow = TRUE;
	attr->pre_op_attr_u.attributes.size = (uint64)st->st_size;
	nfstime_from_stat(&attr->pre_op_attr_u.attributes.mtime, &st->st_mtim);
	nfstime_from_stat(&attr->pre_op_attr_u.attributes.ctime, &st->st_ctim);
}

static void fill_wcc(const char *path, const struct stat *before,
		     bool have_before, wcc_data *wcc)
{
	if (have_before)
		fill_pre_attr_from_stat(before, &wcc->before);
	else
		wcc->before.attributes_follow = FALSE;
	fill_post_attr(path, &wcc->after);
}

static int nfsfh_to_relpath(const nfs_fh3 *fh, char *relpath, size_t relpath_len)
{
	struct unfs_fh core;

	if (!fh || fh->data.data_len > UNFS_FHSIZE)
		return -EINVAL;

	memset(&core, 0, sizeof(core));
	core.len = fh->data.data_len;
	if (core.len > 0)
		memcpy(core.data, fh->data.data_val, core.len);
	return unfs_path_from_fh(&core, relpath, relpath_len);
}

static int relpath_to_core_fh(const char *relpath, struct unfs_fh *fh)
{
	return unfs_fh_from_path(relpath, fh);
}

static void set_nfsfh(nfs_fh3 *dst, struct unfs_fh *src)
{
	dst->data.data_len = src->len;
	dst->data.data_val = (char *)src->data;
}

static int fullpath_from_fh(const nfs_fh3 *fh, char *relpath, size_t relpath_len,
			    char *fullpath, size_t fullpath_len)
{
	int ret;

	ret = nfsfh_to_relpath(fh, relpath, relpath_len);
	if (ret < 0)
		return ret;
	return unfs_join_export_path(export_root, relpath, fullpath, fullpath_len);
}

static int validate_name(const char *name)
{
	if (!name || name[0] == '\0')
		return -EINVAL;
	if (strchr(name, '/'))
		return -EINVAL;
	if (strcmp(name, "..") == 0)
		return -EINVAL;
	return 0;
}

static int child_relpath(const char *dir, const char *name, char *out,
			 size_t out_len)
{
	int written;

	if (validate_name(name) < 0)
		return -EINVAL;
	if (strcmp(name, ".") == 0)
		written = snprintf(out, out_len, "%s", dir);
	else if (dir[0] == '\0')
		written = snprintf(out, out_len, "%s", name);
	else
		written = snprintf(out, out_len, "%s/%s", dir, name);

	if (written < 0 || (size_t)written >= out_len)
		return -ENAMETOOLONG;
	if (!unfs_path_is_safe(out))
		return -EINVAL;
	return 0;
}

static mode_t create_mode_from_args(const CREATE3args *arg)
{
	const sattr3 *attr = NULL;

	if (arg->how.mode == UNCHECKED)
		attr = &arg->how.createhow3_u.obj_attributes;
	else if (arg->how.mode == GUARDED)
		attr = &arg->how.createhow3_u.g_obj_attributes;

	if (attr && attr->mode.set_it)
		return (mode_t)(attr->mode.set_mode3_u.mode & 07777);
	return DEFAULT_MODE;
}

bool_t nfsproc3_null_3_svc(void *result, struct svc_req *rqstp)
{
	(void)result;
	(void)rqstp;
	return TRUE;
}

bool_t nfsproc3_getattr_3_svc(GETATTR3args arg, GETATTR3res *res,
			      struct svc_req *rqstp)
{
	char rel[UNFS_MAX_PATH];
	char full[PATH_MAX];
	struct stat st;
	int ret;

	(void)rqstp;
	memset(res, 0, sizeof(*res));
	ret = fullpath_from_fh(&arg.object, rel, sizeof(rel), full, sizeof(full));
	if (ret < 0) {
		res->status = NFS3ERR_BADHANDLE;
		return TRUE;
	}
	if (lstat(full, &st) < 0) {
		res->status = errno_to_nfs3(errno);
		return TRUE;
	}

	res->status = NFS3_OK;
	fill_fattr(&st, &res->GETATTR3res_u.attributes);
	return TRUE;
}

static void timespec_from_nfs(nfstime3 src, struct timespec *dst)
{
	dst->tv_sec = (time_t)src.seconds;
	dst->tv_nsec = (long)src.nseconds;
}

bool_t nfsproc3_setattr_3_svc(SETATTR3args arg, SETATTR3res *res,
			      struct svc_req *rqstp)
{
	char rel[UNFS_MAX_PATH];
	char full[PATH_MAX];
	struct stat before;
	bool have_before;
	struct timespec times[2];
	bool change_time = false;
	int ret;

	(void)rqstp;
	memset(res, 0, sizeof(*res));
	ret = fullpath_from_fh(&arg.object, rel, sizeof(rel), full, sizeof(full));
	if (ret < 0) {
		res->status = NFS3ERR_BADHANDLE;
		return TRUE;
	}

	have_before = lstat(full, &before) == 0;
	if (!have_before) {
		res->status = errno_to_nfs3(errno);
		fill_wcc(full, &before, false, &res->SETATTR3res_u.resfail.obj_wcc);
		return TRUE;
	}

	if (arg.guard.check &&
	    ((uint32)before.st_ctim.tv_sec != arg.guard.sattrguard3_u.obj_ctime.seconds ||
	     (uint32)before.st_ctim.tv_nsec != arg.guard.sattrguard3_u.obj_ctime.nseconds)) {
		res->status = NFS3ERR_NOT_SYNC;
		fill_wcc(full, &before, have_before,
			 &res->SETATTR3res_u.resfail.obj_wcc);
		return TRUE;
	}

	if (arg.new_attributes.mode.set_it &&
	    chmod(full, (mode_t)(arg.new_attributes.mode.set_mode3_u.mode & 07777)) < 0)
		goto fail;
	if ((arg.new_attributes.uid.set_it || arg.new_attributes.gid.set_it) &&
	    chown(full,
		  arg.new_attributes.uid.set_it ?
			  (uid_t)arg.new_attributes.uid.set_uid3_u.uid : (uid_t)-1,
		  arg.new_attributes.gid.set_it ?
			  (gid_t)arg.new_attributes.gid.set_gid3_u.gid : (gid_t)-1) < 0)
		goto fail;
	if (arg.new_attributes.size.set_it &&
	    truncate(full, (off_t)arg.new_attributes.size.set_size3_u.size) < 0)
		goto fail;

	times[0].tv_nsec = UTIME_OMIT;
	times[1].tv_nsec = UTIME_OMIT;
	if (arg.new_attributes.atime.set_it == SET_TO_SERVER_TIME) {
		times[0].tv_nsec = UTIME_NOW;
		change_time = true;
	} else if (arg.new_attributes.atime.set_it == SET_TO_CLIENT_TIME) {
		timespec_from_nfs(arg.new_attributes.atime.set_atime_u.atime, &times[0]);
		change_time = true;
	}
	if (arg.new_attributes.mtime.set_it == SET_TO_SERVER_TIME) {
		times[1].tv_nsec = UTIME_NOW;
		change_time = true;
	} else if (arg.new_attributes.mtime.set_it == SET_TO_CLIENT_TIME) {
		timespec_from_nfs(arg.new_attributes.mtime.set_mtime_u.mtime, &times[1]);
		change_time = true;
	}
	if (change_time && utimensat(AT_FDCWD, full, times, AT_SYMLINK_NOFOLLOW) < 0)
		goto fail;

	res->status = NFS3_OK;
	fill_wcc(full, &before, have_before, &res->SETATTR3res_u.resok.obj_wcc);
	return TRUE;

fail:
	res->status = errno_to_nfs3(errno);
	fill_wcc(full, &before, have_before, &res->SETATTR3res_u.resfail.obj_wcc);
	return TRUE;
}

bool_t nfsproc3_lookup_3_svc(LOOKUP3args arg, LOOKUP3res *res,
			     struct svc_req *rqstp)
{
	static struct unfs_fh object_fh;
	char dir_rel[UNFS_MAX_PATH];
	char child_rel[UNFS_MAX_PATH];
	char dir_full[PATH_MAX];
	char child_full[PATH_MAX];
	struct stat st;
	int ret;

	(void)rqstp;
	memset(res, 0, sizeof(*res));
	ret = fullpath_from_fh(&arg.what.dir, dir_rel, sizeof(dir_rel),
			       dir_full, sizeof(dir_full));
	if (ret < 0) {
		res->status = NFS3ERR_BADHANDLE;
		return TRUE;
	}

	ret = child_relpath(dir_rel, arg.what.name, child_rel, sizeof(child_rel));
	if (ret < 0) {
		res->status = errno_to_nfs3(-ret);
		fill_post_attr(dir_full, &res->LOOKUP3res_u.resfail.dir_attributes);
		return TRUE;
	}
	ret = unfs_join_export_path(export_root, child_rel, child_full,
				    sizeof(child_full));
	if (ret < 0) {
		res->status = errno_to_nfs3(-ret);
		fill_post_attr(dir_full, &res->LOOKUP3res_u.resfail.dir_attributes);
		return TRUE;
	}
	if (lstat(child_full, &st) < 0) {
		res->status = errno_to_nfs3(errno);
		fill_post_attr(dir_full, &res->LOOKUP3res_u.resfail.dir_attributes);
		return TRUE;
	}
	ret = relpath_to_core_fh(child_rel, &object_fh);
	if (ret < 0) {
		res->status = errno_to_nfs3(-ret);
		fill_post_attr(dir_full, &res->LOOKUP3res_u.resfail.dir_attributes);
		return TRUE;
	}

	res->status = NFS3_OK;
	set_nfsfh(&res->LOOKUP3res_u.resok.object, &object_fh);
	res->LOOKUP3res_u.resok.obj_attributes.attributes_follow = TRUE;
	fill_fattr(&st, &res->LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes);
	fill_post_attr(dir_full, &res->LOOKUP3res_u.resok.dir_attributes);
	return TRUE;
}

bool_t nfsproc3_access_3_svc(ACCESS3args arg, ACCESS3res *res,
			     struct svc_req *rqstp)
{
	char rel[UNFS_MAX_PATH];
	char full[PATH_MAX];
	uint32 granted = 0;
	int ret;

	(void)rqstp;
	memset(res, 0, sizeof(*res));
	ret = fullpath_from_fh(&arg.object, rel, sizeof(rel), full, sizeof(full));
	if (ret < 0) {
		res->status = NFS3ERR_BADHANDLE;
		return TRUE;
	}
	if (access(full, F_OK) < 0) {
		res->status = errno_to_nfs3(errno);
		fill_post_attr(full, &res->ACCESS3res_u.resfail.obj_attributes);
		return TRUE;
	}

	if ((arg.access & ACCESS3_READ) && access(full, R_OK) == 0)
		granted |= ACCESS3_READ;
	if ((arg.access & ACCESS3_LOOKUP) && access(full, X_OK) == 0)
		granted |= ACCESS3_LOOKUP;
	if ((arg.access & ACCESS3_MODIFY) && access(full, W_OK) == 0)
		granted |= ACCESS3_MODIFY;
	if ((arg.access & ACCESS3_EXTEND) && access(full, W_OK) == 0)
		granted |= ACCESS3_EXTEND;
	if ((arg.access & ACCESS3_DELETE) && access(full, W_OK) == 0)
		granted |= ACCESS3_DELETE;
	if ((arg.access & ACCESS3_EXECUTE) && access(full, X_OK) == 0)
		granted |= ACCESS3_EXECUTE;

	res->status = NFS3_OK;
	res->ACCESS3res_u.resok.access = granted;
	fill_post_attr(full, &res->ACCESS3res_u.resok.obj_attributes);
	return TRUE;
}

bool_t nfsproc3_read_3_svc(READ3args arg, READ3res *res, struct svc_req *rqstp)
{
	char rel[UNFS_MAX_PATH];
	char full[PATH_MAX];
	struct stat st;
	size_t count;
	ssize_t nread;
	int fd;
	int ret;

	(void)rqstp;
	memset(res, 0, sizeof(*res));
	free(read_buf);
	read_buf = NULL;

	ret = fullpath_from_fh(&arg.file, rel, sizeof(rel), full, sizeof(full));
	if (ret < 0) {
		res->status = NFS3ERR_BADHANDLE;
		return TRUE;
	}
	fd = open(full, O_RDONLY);
	if (fd < 0) {
		res->status = errno_to_nfs3(errno);
		fill_post_attr(full, &res->READ3res_u.resfail.file_attributes);
		return TRUE;
	}

	count = arg.count > NFS3_MAXDATA ? NFS3_MAXDATA : arg.count;
	read_buf = malloc(count ? count : 1);
	if (!read_buf) {
		close(fd);
		res->status = NFS3ERR_IO;
		fill_post_attr(full, &res->READ3res_u.resfail.file_attributes);
		return TRUE;
	}
	nread = pread(fd, read_buf, count, (off_t)arg.offset);
	ret = fstat(fd, &st);
	close(fd);
	if (nread < 0 || ret < 0) {
		res->status = errno_to_nfs3(errno);
		fill_post_attr(full, &res->READ3res_u.resfail.file_attributes);
		return TRUE;
	}

	res->status = NFS3_OK;
	res->READ3res_u.resok.file_attributes.attributes_follow = TRUE;
	fill_fattr(&st, &res->READ3res_u.resok.file_attributes.post_op_attr_u.attributes);
	res->READ3res_u.resok.count = (uint32)nread;
	res->READ3res_u.resok.eof = (arg.offset + (uint64)nread) >= (uint64)st.st_size;
	res->READ3res_u.resok.data.data_len = (u_int)nread;
	res->READ3res_u.resok.data.data_val = read_buf;
	return TRUE;
}

bool_t nfsproc3_write_3_svc(WRITE3args arg, WRITE3res *res,
			    struct svc_req *rqstp)
{
	char rel[UNFS_MAX_PATH];
	char full[PATH_MAX];
	struct stat before;
	bool have_before;
	ssize_t nwritten;
	size_t count;
	int fd;
	int ret;

	(void)rqstp;
	memset(res, 0, sizeof(*res));
	ret = fullpath_from_fh(&arg.file, rel, sizeof(rel), full, sizeof(full));
	if (ret < 0) {
		res->status = NFS3ERR_BADHANDLE;
		return TRUE;
	}

	have_before = lstat(full, &before) == 0;
	fd = open(full, O_WRONLY);
	if (fd < 0) {
		res->status = errno_to_nfs3(errno);
		fill_wcc(full, &before, have_before, &res->WRITE3res_u.resfail.file_wcc);
		return TRUE;
	}

	count = arg.count;
	if (count > arg.data.data_len)
		count = arg.data.data_len;
	nwritten = pwrite(fd, arg.data.data_val, count, (off_t)arg.offset);
	if (nwritten >= 0 && fsync(fd) < 0)
		nwritten = -1;
	close(fd);
	if (nwritten < 0) {
		res->status = errno_to_nfs3(errno);
		fill_wcc(full, &before, have_before, &res->WRITE3res_u.resfail.file_wcc);
		return TRUE;
	}

	res->status = NFS3_OK;
	fill_wcc(full, &before, have_before, &res->WRITE3res_u.resok.file_wcc);
	res->WRITE3res_u.resok.count = (uint32)nwritten;
	res->WRITE3res_u.resok.committed = FILE_SYNC;
	memset(res->WRITE3res_u.resok.verf, 0x42, sizeof(res->WRITE3res_u.resok.verf));
	return TRUE;
}

bool_t nfsproc3_create_3_svc(CREATE3args arg, CREATE3res *res,
			     struct svc_req *rqstp)
{
	static struct unfs_fh object_fh;
	char dir_rel[UNFS_MAX_PATH];
	char child_rel[UNFS_MAX_PATH];
	char dir_full[PATH_MAX];
	char child_full[PATH_MAX];
	struct stat before;
	struct stat child_st;
	bool have_before;
	mode_t mode;
	int flags;
	int fd;
	int ret;

	(void)rqstp;
	memset(res, 0, sizeof(*res));
	ret = fullpath_from_fh(&arg.where.dir, dir_rel, sizeof(dir_rel),
			       dir_full, sizeof(dir_full));
	if (ret < 0) {
		res->status = NFS3ERR_BADHANDLE;
		return TRUE;
	}
	have_before = lstat(dir_full, &before) == 0;

	ret = child_relpath(dir_rel, arg.where.name, child_rel, sizeof(child_rel));
	if (ret < 0 || strcmp(arg.where.name, ".") == 0) {
		res->status = ret < 0 ? errno_to_nfs3(-ret) : NFS3ERR_EXIST;
		fill_wcc(dir_full, &before, have_before,
			 &res->CREATE3res_u.resfail.dir_wcc);
		return TRUE;
	}
	ret = unfs_join_export_path(export_root, child_rel, child_full,
				    sizeof(child_full));
	if (ret < 0) {
		res->status = errno_to_nfs3(-ret);
		fill_wcc(dir_full, &before, have_before,
			 &res->CREATE3res_u.resfail.dir_wcc);
		return TRUE;
	}

	mode = create_mode_from_args(&arg);
	flags = O_WRONLY | O_CREAT;
	if (arg.how.mode == GUARDED || arg.how.mode == EXCLUSIVE)
		flags |= O_EXCL;
	fd = open(child_full, flags, mode);
	if (fd < 0) {
		res->status = errno_to_nfs3(errno);
		fill_wcc(dir_full, &before, have_before,
			 &res->CREATE3res_u.resfail.dir_wcc);
		return TRUE;
	}
	close(fd);
	if (lstat(child_full, &child_st) < 0) {
		res->status = errno_to_nfs3(errno);
		fill_wcc(dir_full, &before, have_before,
			 &res->CREATE3res_u.resfail.dir_wcc);
		return TRUE;
	}
	ret = relpath_to_core_fh(child_rel, &object_fh);
	if (ret < 0) {
		res->status = errno_to_nfs3(-ret);
		fill_wcc(dir_full, &before, have_before,
			 &res->CREATE3res_u.resfail.dir_wcc);
		return TRUE;
	}

	res->status = NFS3_OK;
	res->CREATE3res_u.resok.obj.handle_follows = TRUE;
	set_nfsfh(&res->CREATE3res_u.resok.obj.post_op_fh3_u.handle, &object_fh);
	res->CREATE3res_u.resok.obj_attributes.attributes_follow = TRUE;
	fill_fattr(&child_st,
		   &res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes);
	fill_wcc(dir_full, &before, have_before,
		 &res->CREATE3res_u.resok.dir_wcc);
	return TRUE;
}

bool_t nfsproc3_remove_3_svc(REMOVE3args arg, REMOVE3res *res,
			     struct svc_req *rqstp)
{
	char dir_rel[UNFS_MAX_PATH];
	char child_rel[UNFS_MAX_PATH];
	char dir_full[PATH_MAX];
	char child_full[PATH_MAX];
	struct stat before;
	bool have_before;
	int ret;

	(void)rqstp;
	memset(res, 0, sizeof(*res));
	ret = fullpath_from_fh(&arg.object.dir, dir_rel, sizeof(dir_rel),
			       dir_full, sizeof(dir_full));
	if (ret < 0) {
		res->status = NFS3ERR_BADHANDLE;
		return TRUE;
	}
	have_before = lstat(dir_full, &before) == 0;

	ret = child_relpath(dir_rel, arg.object.name, child_rel, sizeof(child_rel));
	if (ret < 0 || strcmp(arg.object.name, ".") == 0) {
		res->status = ret < 0 ? errno_to_nfs3(-ret) : NFS3ERR_INVAL;
		fill_wcc(dir_full, &before, have_before,
			 &res->REMOVE3res_u.resfail.dir_wcc);
		return TRUE;
	}
	ret = unfs_join_export_path(export_root, child_rel, child_full,
				    sizeof(child_full));
	if (ret < 0) {
		res->status = errno_to_nfs3(-ret);
		fill_wcc(dir_full, &before, have_before,
			 &res->REMOVE3res_u.resfail.dir_wcc);
		return TRUE;
	}

	if (unlink(child_full) < 0) {
		res->status = errno_to_nfs3(errno);
		fill_wcc(dir_full, &before, have_before,
			 &res->REMOVE3res_u.resfail.dir_wcc);
		return TRUE;
	}

	res->status = NFS3_OK;
	fill_wcc(dir_full, &before, have_before,
		 &res->REMOVE3res_u.resok.dir_wcc);
	return TRUE;
}

bool_t nfsproc3_readdir_3_svc(READDIR3args arg, READDIR3res *res,
			      struct svc_req *rqstp)
{
	char rel[UNFS_MAX_PATH];
	char full[PATH_MAX];
	DIR *dir;
	struct dirent *de;
	size_t idx = 0;
	size_t max_entries;
	uint64 seq = 0;
	int ret;

	(void)rqstp;
	memset(res, 0, sizeof(*res));
	ret = fullpath_from_fh(&arg.dir, rel, sizeof(rel), full, sizeof(full));
	if (ret < 0) {
		res->status = NFS3ERR_BADHANDLE;
		return TRUE;
	}

	dir = opendir(full);
	if (!dir) {
		res->status = errno_to_nfs3(errno);
		fill_post_attr(full, &res->READDIR3res_u.resfail.dir_attributes);
		return TRUE;
	}

	max_entries = arg.count / 32;
	if (max_entries == 0)
		max_entries = 1;
	if (max_entries > sizeof(readdir_entries) / sizeof(readdir_entries[0]))
		max_entries = sizeof(readdir_entries) / sizeof(readdir_entries[0]);

	while ((de = readdir(dir)) != NULL) {
		seq++;
		if (seq <= arg.cookie)
			continue;
		if (idx == max_entries)
			break;

		readdir_entries[idx].fileid = (uint64)de->d_ino;
		snprintf(readdir_names[idx], sizeof(readdir_names[idx]), "%s", de->d_name);
		readdir_entries[idx].name = readdir_names[idx];
		readdir_entries[idx].cookie = seq;
		readdir_entries[idx].nextentry = NULL;
		if (idx > 0)
			readdir_entries[idx - 1].nextentry = &readdir_entries[idx];
		idx++;
	}

	res->status = NFS3_OK;
	fill_post_attr(full, &res->READDIR3res_u.resok.dir_attributes);
	memset(res->READDIR3res_u.resok.cookieverf, 0,
	       sizeof(res->READDIR3res_u.resok.cookieverf));
	res->READDIR3res_u.resok.reply.entries = idx ? &readdir_entries[0] : NULL;
	res->READDIR3res_u.resok.reply.eof = de == NULL;
	closedir(dir);
	return TRUE;
}

bool_t nfsproc3_fsinfo_3_svc(FSINFO3args arg, FSINFO3res *res,
			     struct svc_req *rqstp)
{
	char rel[UNFS_MAX_PATH];
	char full[PATH_MAX];
	int ret;

	(void)rqstp;
	memset(res, 0, sizeof(*res));
	ret = fullpath_from_fh(&arg.fsroot, rel, sizeof(rel), full, sizeof(full));
	if (ret < 0) {
		res->status = NFS3ERR_BADHANDLE;
		return TRUE;
	}

	res->status = NFS3_OK;
	fill_post_attr(full, &res->FSINFO3res_u.resok.obj_attributes);
	res->FSINFO3res_u.resok.rtmax = NFS3_MAXDATA;
	res->FSINFO3res_u.resok.rtpref = 65536;
	res->FSINFO3res_u.resok.rtmult = 4096;
	res->FSINFO3res_u.resok.wtmax = NFS3_MAXDATA;
	res->FSINFO3res_u.resok.wtpref = 65536;
	res->FSINFO3res_u.resok.wtmult = 4096;
	res->FSINFO3res_u.resok.dtpref = 4096;
	res->FSINFO3res_u.resok.maxfilesize = (uint64)INT64_MAX;
	res->FSINFO3res_u.resok.time_delta.seconds = 0;
	res->FSINFO3res_u.resok.time_delta.nseconds = 1;
	res->FSINFO3res_u.resok.properties = 0;
	return TRUE;
}

static bool_t unsupported(DUMMY3res *res)
{
	memset(res, 0, sizeof(*res));
	res->status = NFS3ERR_NOTSUPP;
	return TRUE;
}

bool_t nfsproc3_readlink_3_svc(DUMMY3args arg, DUMMY3res *res, struct svc_req *rqstp)
{
	(void)arg;
	(void)rqstp;
	return unsupported(res);
}

bool_t nfsproc3_mkdir_3_svc(DUMMY3args arg, DUMMY3res *res, struct svc_req *rqstp)
{
	(void)arg;
	(void)rqstp;
	return unsupported(res);
}

bool_t nfsproc3_symlink_3_svc(DUMMY3args arg, DUMMY3res *res, struct svc_req *rqstp)
{
	(void)arg;
	(void)rqstp;
	return unsupported(res);
}

bool_t nfsproc3_mknod_3_svc(DUMMY3args arg, DUMMY3res *res, struct svc_req *rqstp)
{
	(void)arg;
	(void)rqstp;
	return unsupported(res);
}

bool_t nfsproc3_rmdir_3_svc(DUMMY3args arg, DUMMY3res *res, struct svc_req *rqstp)
{
	(void)arg;
	(void)rqstp;
	return unsupported(res);
}

bool_t nfsproc3_rename_3_svc(DUMMY3args arg, DUMMY3res *res, struct svc_req *rqstp)
{
	(void)arg;
	(void)rqstp;
	return unsupported(res);
}

bool_t nfsproc3_link_3_svc(DUMMY3args arg, DUMMY3res *res, struct svc_req *rqstp)
{
	(void)arg;
	(void)rqstp;
	return unsupported(res);
}

bool_t nfsproc3_readdirplus_3_svc(READDIRPLUS3args arg, READDIRPLUS3res *res,
				  struct svc_req *rqstp)
{
	char rel[UNFS_MAX_PATH];
	char full[PATH_MAX];
	int ret;

	(void)rqstp;
	memset(res, 0, sizeof(*res));
	ret = fullpath_from_fh(&arg.dir, rel, sizeof(rel), full, sizeof(full));
	res->status = NFS3ERR_NOTSUPP;
	if (ret == 0)
		fill_post_attr(full, &res->READDIRPLUS3res_u.resfail.dir_attributes);
	else
		res->READDIRPLUS3res_u.resfail.dir_attributes.attributes_follow = FALSE;
	return TRUE;
}

bool_t nfsproc3_fsstat_3_svc(DUMMY3args arg, DUMMY3res *res, struct svc_req *rqstp)
{
	(void)arg;
	(void)rqstp;
	return unsupported(res);
}

bool_t nfsproc3_pathconf_3_svc(DUMMY3args arg, DUMMY3res *res, struct svc_req *rqstp)
{
	(void)arg;
	(void)rqstp;
	return unsupported(res);
}

bool_t nfsproc3_commit_3_svc(DUMMY3args arg, DUMMY3res *res, struct svc_req *rqstp)
{
	(void)arg;
	(void)rqstp;
	return unsupported(res);
}

bool_t mountproc3_null_3_svc(void *result, struct svc_req *rqstp)
{
	(void)result;
	(void)rqstp;
	return TRUE;
}

bool_t mountproc3_mnt_3_svc(dirpath path, mountres3 *res, struct svc_req *rqstp)
{
	static struct unfs_fh root_fh;
	static int auth_flavors[] = { 1 };
	struct stat st;
	int ret;

	(void)rqstp;
	memset(res, 0, sizeof(*res));
	if (!path || strcmp(path, "/") != 0) {
		res->fhs_status = MNT3ERR_NOENT;
		return TRUE;
	}
	if (stat(export_root, &st) < 0 || !S_ISDIR(st.st_mode)) {
		res->fhs_status = errno_to_mount3(errno ? errno : ENOTDIR);
		return TRUE;
	}
	ret = relpath_to_core_fh("", &root_fh);
	if (ret < 0) {
		res->fhs_status = errno_to_mount3(-ret);
		return TRUE;
	}

	res->fhs_status = MNT3_OK;
	res->mountres3_u.mountinfo.fhandle.fhandle3_len = root_fh.len;
	res->mountres3_u.mountinfo.fhandle.fhandle3_val = (char *)root_fh.data;
	res->mountres3_u.mountinfo.auth_flavors.auth_flavors_len = 1;
	res->mountres3_u.mountinfo.auth_flavors.auth_flavors_val = auth_flavors;
	return TRUE;
}

bool_t mountproc3_umnt_3_svc(dirpath path, void *result, struct svc_req *rqstp)
{
	(void)path;
	(void)result;
	(void)rqstp;
	return TRUE;
}

int nfs_program_3_freeresult(SVCXPRT *transp, xdrproc_t xdr_result,
			     caddr_t result)
{
	(void)transp;
	(void)xdr_result;
	(void)result;
	return 1;
}

int mountprog_3_freeresult(SVCXPRT *transp, xdrproc_t xdr_result,
			   caddr_t result)
{
	(void)transp;
	(void)xdr_result;
	(void)result;
	return 1;
}

static int make_listener(unsigned short port)
{
	struct sockaddr_in addr;
	int fd;
	int one = 1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		close(fd);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(fd);
		return -1;
	}
	if (listen(fd, SOMAXCONN) < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

static int register_tcp_service(unsigned short port, rpcprog_t prog, rpcvers_t vers,
				void (*dispatch)(struct svc_req *, SVCXPRT *))
{
	SVCXPRT *xprt;
	int fd;

	fd = make_listener(port);
	if (fd < 0)
		return -1;
	xprt = svctcp_create(fd, 0, 0);
	if (!xprt) {
		close(fd);
		return -1;
	}
	if (!svc_register(xprt, prog, vers, dispatch, 0)) {
		svc_destroy(xprt);
		return -1;
	}
	return 0;
}

static int parse_port(const char *text, unsigned short *port)
{
	char *end;
	long value;

	errno = 0;
	value = strtol(text, &end, 10);
	if (errno || *end || value <= 0 || value > 65535)
		return -1;
	*port = (unsigned short)value;
	return 0;
}

static void usage(const char *prog)
{
	fprintf(stderr,
		"usage: %s [--nfs-port port] [--mount-port port] <export-root>\n",
		prog);
}

int main(int argc, char **argv)
{
	unsigned short nfs_port = DEFAULT_NFS_PORT;
	unsigned short mount_port = DEFAULT_MOUNT_PORT;
	const char *root_arg = NULL;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--nfs-port") == 0 && i + 1 < argc) {
			if (parse_port(argv[++i], &nfs_port) < 0) {
				usage(argv[0]);
				return 2;
			}
		} else if (strcmp(argv[i], "--mount-port") == 0 && i + 1 < argc) {
			if (parse_port(argv[++i], &mount_port) < 0) {
				usage(argv[0]);
				return 2;
			}
		} else if (!root_arg) {
			root_arg = argv[i];
		} else {
			usage(argv[0]);
			return 2;
		}
	}

	if (!root_arg) {
		usage(argv[0]);
		return 2;
	}
	if (!realpath(root_arg, export_root)) {
		perror("realpath export root");
		return 1;
	}
	if (register_tcp_service(nfs_port, NFS_PROGRAM, NFS_V3, nfs_program_3) < 0) {
		perror("register nfs");
		return 1;
	}
	if (register_tcp_service(mount_port, MOUNTPROG, MOUNTVERS3, mountprog_3) < 0) {
		perror("register mountd");
		return 1;
	}

	printf("exporting %s via nfs tcp/%u and mount tcp/%u\n",
	       export_root, nfs_port, mount_port);
	fflush(stdout);
	svc_run();
	fprintf(stderr, "svc_run returned unexpectedly\n");
	return 1;
}
