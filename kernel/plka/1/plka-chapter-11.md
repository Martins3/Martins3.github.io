# Professional Linux Kernel Architecture : Extended Attributes and Access Control Lists

Additional features that go beyond the standard Unix file model often require
an extended set of attributes associated with every filesystem object.
What the kernel can provide,
however, is a framework that allows filesystem-specific extensions.
`Extended attributes (xattrs)` are
(more or less) arbitrary attributes that can be associated with a file. Since usually every file will
possess only a subset of all possible extended attributes, the attributes are stored outside the regular
inode data structure to avoid increasing its size in memory and wasting disk space.

1. One use of extended attributes is the implementation of access control lists that extend the Unix-style
permission model: They allow implementation of finer-grained access rights by not only using the
concept of the classes `user`, `group`, and `others`, but also by associating an explicit list of users and their
allowed operations on the file. Such lists fit naturally into the extended attribute model.

2. Another use of extended attributes is to provide labeling information for SE-Linux.

## 11.1 Extended Attributes
From the filesystem user’s point of view, an extended attribute is a name/value pair associated
with objects in the filesystem. While the name is given by a regular string, the kernel imposes no
restrictions on the contents of the value. It can be a text string, but may contain arbitrary binary data
as well. An attribute may be defined or not (this is the case if no attribute was associated with a file).
If it is defined, it may or may not have a value. No one can blame the kernel for not being liberal in
this respect.

Attribute names are subdivided into namespaces. This implies that addressing attributes are required to
list the namespace as well. As per notational convention, a dot is used to separate the namespace and
attribute

Attribute names are subdivided into namespaces. This implies that addressing attributes are required to
list the namespace as well. As per notational convention, a dot is used to separate the namespace and
attribute 
> 有超级多的`xattr.h`文件，`include/uapi/linux/attr.h`
```c
/* Namespaces */
#define XATTR_OS2_PREFIX "os2."
#define XATTR_OS2_PREFIX_LEN (sizeof(XATTR_OS2_PREFIX) - 1)

#define XATTR_MAC_OSX_PREFIX "osx."
#define XATTR_MAC_OSX_PREFIX_LEN (sizeof(XATTR_MAC_OSX_PREFIX) - 1)

#define XATTR_BTRFS_PREFIX "btrfs."
#define XATTR_BTRFS_PREFIX_LEN (sizeof(XATTR_BTRFS_PREFIX) - 1)

#define XATTR_SECURITY_PREFIX	"security."
#define XATTR_SECURITY_PREFIX_LEN (sizeof(XATTR_SECURITY_PREFIX) - 1)

#define XATTR_SYSTEM_PREFIX "system."
#define XATTR_SYSTEM_PREFIX_LEN (sizeof(XATTR_SYSTEM_PREFIX) - 1)

#define XATTR_TRUSTED_PREFIX "trusted."
#define XATTR_TRUSTED_PREFIX_LEN (sizeof(XATTR_TRUSTED_PREFIX) - 1)

#define XATTR_USER_PREFIX "user."
#define XATTR_USER_PREFIX_LEN (sizeof(XATTR_USER_PREFIX) - 1)
```

The kernel provides several system calls to read and manipulate extended attributes:
1. setxattr is used to set or replace the value of an extended attribute or to create a new one.
1. getxattr retrieves the value of an extended attribute.
1. removexattr removes an extended attribute.
1. listxattr provides a list of all extended attributes associated with a given filesystem object.

Note that all calls are also available with the prefix `l`; this variant does not follow symbolic links by
resolving them but operates on the extended attributes of the link itself. Prefixing the calls with `f` does
not work on a filename given by a string, but uses a file descriptor as the argument

#### 11.1.1 Interface to the Virtual Filesystem
**Data Structures**

```c
struct inode_operations {
  ...
  int (*setxattr) (struct dentry *, const char *,const void *,size_t,int);
  ssize_t (*getxattr) (struct dentry *, const char *, void *, size_t);
  ssize_t (*listxattr) (struct dentry *, char *, size_t);
  int (*removexattr) (struct dentry *, const char*);
  ...
}
```

For every class of extended attributes, functions that *transfer the information to
and from the block device are required*. They are encapsulated in the following structure:
```c
struct xattr_handler {
	const char *prefix;
	int flags;      /* fs private flags */
	size_t (*list)(const struct xattr_handler *, struct dentry *dentry,
		       char *list, size_t list_size, const char *name,
		       size_t name_len);
	int (*get)(const struct xattr_handler *, struct dentry *dentry,
		   const char *name, void *buffer, size_t size);
	int (*set)(const struct xattr_handler *, struct dentry *dentry,
		   const char *name, const void *buffer, size_t size,
		   int flags);
};
```


The superblock provides a link to an array of all supported handlers for the respective filesystem:
```c
struct super_block {
  ...
  struct xattr_handler **s_xattr;
  ...
}
```

**System Calls**
Recall that there are three system calls for each extended attribute operation (get, set, and list), which
differ in how the destination is specified. To avoid code duplication, the system calls are structured into
two parts:
1. Find the instance of `dentry` associated with the target object.
2. Delegate further processing to a function common to all three calls.

`fs/xattr.c`:

```c

/*
 * Find the handler for the prefix and dispatch its set() operation.
 */
int
generic_setxattr(struct dentry *dentry, const char *name, const void *value, size_t size, int flags)
{
	const struct xattr_handler *handler;

	if (size == 0)
		value = "";  /* empty EA, do not remove */
	handler = xattr_resolve_name(dentry->d_sb->s_xattr, &name);
	if (!handler)
		return -EOPNOTSUPP;
	return handler->set(handler, dentry, name, value, size, flags);
}
```


`xattr.c`
```c
/*
 * Extended attribute SET operations
 */
static long
setxattr(struct dentry *d, const char __user *name, const void __user *value,
	 size_t size, int flags)
{
	int error;
	void *kvalue = NULL;
	void *vvalue = NULL;	/* If non-NULL, we used vmalloc() */
	char kname[XATTR_NAME_MAX + 1];

	if (flags & ~(XATTR_CREATE|XATTR_REPLACE))
		return -EINVAL;

	error = strncpy_from_user(kname, name, sizeof(kname));
	if (error == 0 || error == sizeof(kname))
		error = -ERANGE;
	if (error < 0)
		return error;

	if (size) {
		if (size > XATTR_SIZE_MAX)
			return -E2BIG;
		kvalue = kmalloc(size, GFP_KERNEL | __GFP_NOWARN);
		if (!kvalue) {
			vvalue = vmalloc(size);
			if (!vvalue)
				return -ENOMEM;
			kvalue = vvalue;
		}
		if (copy_from_user(kvalue, value, size)) {
			error = -EFAULT;
			goto out;
		}
		if ((strcmp(kname, XATTR_NAME_POSIX_ACL_ACCESS) == 0) ||
		    (strcmp(kname, XATTR_NAME_POSIX_ACL_DEFAULT) == 0))
			posix_acl_fix_xattr_from_user(kvalue, size);
	}

	error = vfs_setxattr(d, kname, kvalue, size, flags);
out:
	if (vvalue)
		vfree(vvalue);
	else
		kfree(kvalue);
	return error;
}
```
> 书上将的很好，这一个函数两部分，复制参数，调用 vfs_setxattr

```c
int
vfs_setxattr(struct dentry *dentry, const char *name, const void *value,
		size_t size, int flags)
{
	struct inode *inode = dentry->d_inode;
	int error;

	error = xattr_permission(inode, name, MAY_WRITE);
	if (error)
		return error;

	inode_lock(inode);
	error = security_inode_setxattr(dentry, name, value, size, flags);
	if (error)
		goto out;

	error = __vfs_setxattr_noperm(dentry, name, value, size, flags);

out:
	inode_unlock(inode);
	return error;
}
EXPORT_SYMBOL_GPL(vfs_setxattr);
```

> 上面一段关于 security 和 permission 真的莫名其妙，什么　kernel ignores these namespaces and delegates the choices to security module, 简直莫名奇妙啊!

If the inode passed the permission check, vfs_setxattr continues as follows:
1. If a filesystem-specific setxattr method is available in the inode operations, it is called to
perform the low-level interaction with the filesystem. After this, `fsnotify_xattr` uses the
inotify mechanism to inform the userland about the extended attribute change.
2. If no setxattr method is available (i.e., if the underlying filesystem does not support
extended attributes), but the extended attribute in question belongs to the security namespace, then the kernel tries to use a function that can be provided by security frameworks like
SELinux. If no such framework is registered, the operation is denied.
This allows security labels on files that reside on filesystems without extended attribute support. It is the task of the security subsystem to store the information in a reasonable way
> SELinux 又打来了什么新的复杂的东西

Since the implementation for the system calls `getxattr` and `removexattr` nearly completely follows the
scheme presented for `setxattr`, it is not necessary to discuss them in greater depth. The differences are
as follows:
1. getxattr does not need to use fnotify because nothing is modified.
2. removeattr need not copy an attribute value, but only the name from the userspace. No special
casing for the security handler is required.

The code for listing all extended attributes associated with a file differs more from this scheme, 
particularly because no function `vfs_listxattr` is used. All work is performed in `listxattr`.
The implementation proceeds in three easy steps:
1. Adapt the maximum size of the list as given by by the userspace program such that it is
not higher than the maximal size of an extended attribute list as allowed by the kernel with
XATTR_LIST_MAX, and allocate the required memory.
2. Call listxattr from inode_operations to fill the allocated space with name/value pairs.
3. Copy the result back to the userspace.

> 1. vfs_setxattr 最后调用到的内容是，也就是说对于一个文件系统中间，最终的处理方法都是需要依赖于 xattr_handler 的几个函数实现
> ```c
> 	return handler->set(handler, dentry, inode, name, value, size, flags);
> ```
> 2.  setxattr  vfs_setxattr xattr_handler 中间的set , 分别处理拷贝参数，检查permission 和 终极内容的实现
> 2. 所以 generic_setxattr 是什么东西 ?

**Generic Handler Functions**
> 1. 实际上，目前只有一个 generic_listxattr，其余的对称都已经不存在了.
> 2. 更加尴尬的是，使用generic_listxattr　只有在nfs一个地方

#### 11.1.2 Implementation in Ext3
This also
raises a question that has not been touched on: namely, how extended attributes are permanently stored
on disk.

**Data Structures**

```c
static const struct xattr_handler * const ext4_xattr_handler_map[] = {
	[EXT4_XATTR_INDEX_USER]		     = &ext4_xattr_user_handler,
#ifdef CONFIG_EXT4_FS_POSIX_ACL
	[EXT4_XATTR_INDEX_POSIX_ACL_ACCESS]  = &posix_acl_access_xattr_handler,
	[EXT4_XATTR_INDEX_POSIX_ACL_DEFAULT] = &posix_acl_default_xattr_handler,
#endif
	[EXT4_XATTR_INDEX_TRUSTED]	     = &ext4_xattr_trusted_handler,
#ifdef CONFIG_EXT4_FS_SECURITY
	[EXT4_XATTR_INDEX_SECURITY]	     = &ext4_xattr_security_handler,
#endif
};
```
> 1. 通过如此定义，实现多组 xattr_handler 的实现
> 2. 当前的位置更加下层，vfs_setxattr之类的东西已经分析不到了

```c
struct ext4_xattr_header {
	__le32	h_magic;	/* magic number for identification */
	__le32	h_refcount;	/* reference count */
	__le32	h_blocks;	/* number of disk blocks used */
	__le32	h_hash;		/* hash value of all attributes */
	__le32	h_checksum;	/* crc32c(uuid+id+xattrblock) */
				/* id = inum if refcount=1, blknum otherwise */
	__u32	h_reserved[3];	/* zero right now */
};

struct ext4_xattr_ibody_header {
	__le32	h_magic;	/* magic number for identification */
};

struct ext4_xattr_entry {
	__u8	e_name_len;	/* length of name */
	__u8	e_name_index;	/* attribute name index */
	__le16	e_value_offs;	/* offset in disk block of value */
	__le32	e_value_inum;	/* inode in which the value is stored */
	__le32	e_value_size;	/* size of attribute value */
	__le32	e_hash;		/* hash value of name and value */
	char	e_name[0];	/* attribute name */
};
```

> @todo on-disk format 和 ext4_xattr_header 
**Implementation**

Since the handler implementation is quite similar for different attribute namespaces, 
the following discussion is restricted to the implementation for the user namespace; the handler functions for the other
namespaces differ only little or not at all.
> 不同的namespace 类似，只分析最简单user的版本

```c
const struct xattr_handler ext4_xattr_user_handler = {
	.prefix	= XATTR_USER_PREFIX,
	.list	= ext4_xattr_user_list,
	.get	= ext4_xattr_user_get,
	.set	= ext4_xattr_user_set,
};


static int
ext4_xattr_user_set(const struct xattr_handler *handler,
		    struct dentry *unused, struct inode *inode,
		    const char *name, const void *value,
		    size_t size, int flags)
{
	if (!test_opt(inode->i_sb, XATTR_USER))
		return -EOPNOTSUPP;
	return ext4_xattr_set(inode, EXT4_XATTR_INDEX_USER,
			      name, value, size, flags);
}
```

> 大幕渐开

**Retrieving Extended Attributes**
> @todo 所以这三个函数为什么不是对称的啊! 问题是每一个还贼鸡巴复杂

**Setting Extended Attributes**
> @todo

**Listing Extended Attributes**
> @todo

#### 11.1.3 Implementation in Ext2
> @todo only half a page

## 11.2 Access Control Lists
> @todo about ten pages, some what boring !
