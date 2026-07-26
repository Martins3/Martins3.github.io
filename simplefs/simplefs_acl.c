#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/posix_acl.h>
#include <linux/posix_acl_xattr.h>

#include "simplefs.h"

struct posix_acl *simplefs_get_acl(struct inode *inode, int type, bool rcu)
{
	const char *name;
	void *value = NULL;
	struct posix_acl *acl;
	int retval;

	if (rcu)
		return ERR_PTR(-ECHILD);

	name = posix_acl_xattr_name(type);

	retval = simplefs_xattr_get(inode, name, NULL, 0);
	if (retval > 0) {
		value = kmalloc(retval, GFP_KERNEL);
		if (!value)
			return ERR_PTR(-ENOMEM);
		retval = simplefs_xattr_get(inode, name, value, retval);
	}
	if (retval > 0)
		acl = posix_acl_from_xattr(&init_user_ns, value, retval);
	else if (retval == -ENODATA)
		acl = NULL;
	else
		acl = ERR_PTR(retval);

	kfree(value);
	return acl;
}

static int __simplefs_set_acl(struct inode *inode, struct posix_acl *acl,
			      int type)
{
	const char *name;
	void *value = NULL;
	size_t acl_size = 0;
	int error;

	name = posix_acl_xattr_name(type);

	if (type == ACL_TYPE_DEFAULT && !S_ISDIR(inode->i_mode))
		return acl ? -EACCES : 0;

	if (acl) {
		/* ACL 超出单个 xattr 块的容量上限时按 EINVAL 拒绝
		 * （generic/026 期望 "Invalid argument"），而不是等到
		 * 存储层才发现放不下返回 ENOSPC。这里只按理论上限
		 * 拦截"永远放不下"的请求；与其他 xattr 竞争导致的
		 * 暂时放不下仍由 xattr_set 返回 ENOSPC。
		 */
		size_t max_size = SIMPLEFS_BLOCK_SIZE -
			sizeof(struct simplefs_xattr_header) -
			SIMPLEFS_XATTR_ENTRY_SIZE(strlen(name));

		value = posix_acl_to_xattr(&init_user_ns, acl, &acl_size,
					    GFP_KERNEL);
		if (!value)
			return -ENOMEM;
		if (acl_size > max_size) {
			kfree(value);
			return -EINVAL;
		}
		error = simplefs_xattr_set(inode, name, value, acl_size, 0);
	} else {
		error = simplefs_xattr_set(inode, name, NULL, 0, XATTR_REPLACE);
		if (error == -ENODATA)
			error = 0;
	}

	kfree(value);
	if (!error)
		set_cached_acl(inode, type, acl);
	return error;
}

int simplefs_set_acl(struct mnt_idmap *idmap, struct dentry *dentry,
		     struct posix_acl *acl, int type)
{
	struct inode *inode = d_inode(dentry);
	int error;
	int update_mode = 0;
	umode_t mode = inode->i_mode;

	if (type == ACL_TYPE_ACCESS && acl) {
		error = posix_acl_update_mode(&nop_mnt_idmap, inode, &mode,
					      &acl);
		if (error)
			return error;
		update_mode = 1;
	}

	error = __simplefs_set_acl(inode, acl, type);
	if (!error && update_mode) {
		inode->i_mode = mode;
		inode_set_ctime_current(inode);
		mark_inode_dirty(inode);
	}
	return error;
}

int simplefs_init_acl(struct inode *inode, struct inode *dir)
{
	struct posix_acl *default_acl, *acl;
	int error;

	error = posix_acl_create(dir, &inode->i_mode, &default_acl, &acl);
	if (error)
		return error;

	if (default_acl) {
		error = __simplefs_set_acl(inode, default_acl, ACL_TYPE_DEFAULT);
		posix_acl_release(default_acl);
	}
	if (acl) {
		if (!error)
			error = __simplefs_set_acl(inode, acl, ACL_TYPE_ACCESS);
		posix_acl_release(acl);
	}
	return error;
}
