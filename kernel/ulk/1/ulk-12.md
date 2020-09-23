# Understand Linux Kernel : The Virtual Filesystem

## 1 The Role of the Virtual Filesystem (VFS)

Filesystems supported by the VFS may be grouped into three main classes:
1. Disk-based filesystems
2. Network filesystems
3. Special filesystems

#### 1.1 The Common File Model

A useful feature of Linux’s VFS allows it to handle virtual block devices such as `/dev/loop0`, which may be used to mount filesystems stored in regular files. 
**As a possible application, a user may protect her own private filesystem by storing an encrypted version of it in a regular file.**
> @todo Man(4), related source file : drivers/block/loop.c

The common file model consists of the following object types:
- The superblock object

  Stores information concerning a mounted filesystem. For disk-based filesystems,
  this object usually corresponds to a filesystem control block stored on disk.

- The inode object

  Stores general information about a specific file. For disk-based filesystems, this
  object usually corresponds to a file control block stored on disk. Each inode
  object is associated with an inode number, which uniquely identifies the file
  within the filesystem.

- The file object

  Stores information about the interaction between an open file and a process.
  **This information exists only in kernel memory during the period when a process
  has the file open.**

- The dentry object

  Stores information about the linking of a directory entry (that is, a particular
  name of the file) with the corresponding file. Each disk-based filesystem stores
  this information in its own particular way on disk.

Besides providing a common interface to all filesystem implementations, the VFS has
another important role related to system performance.
The most recently used dentry objects are contained in a disk cache named the dentry cache, which speeds up
the translation from a file pathname to the inode of the last pathname component.

Notice how a disk cache differs from a hardware cache or a memory cache, neither of
which has anything to do with disks or other devices. A hardware cache is a fast
static RAM that speeds up requests directed to the slower dynamic RAM (see the
section “Hardware Cache” in Chapter 2). *A memory cache is a software mechanism
introduced to bypass the Kernel Memory Allocator (see the section “The Slab Allocator” in Chapter 8).*
> @todo understand the Kernel Memory Allocator !


#### 1.2 System Calls Handled by the VFS

We said earlier that the VFS is a layer between application programs and specific filesystems. **However, in some cases, a file operation can be performed by the VFS itself,
without invoking a lower-level procedure.** For instance, when a process closes an
open file, the file on disk doesn’t usually need to be touched, and hence the VFS simply releases the corresponding file object.
Similarly, when the **lseek()** system call
modifies a file pointer, which is an attribute related to the interaction between an
opened file and a process, the VFS needs to modify only the corresponding file object
without accessing the file on disk, and therefore it does not have to invoke a specific
filesystem procedure. In some sense, the VFS could be considered a “generic” filesystem that relies, when necessary, on specific ones.

## 2 VFS Data Structures

#### 2.1 Superblock Objects

#### 2.2 Inode Objects
#### 2.3 File Objects

#### 2.4 dentry Objects


## 4 Filesystem Handling

Like every traditional Unix system, Linux makes use of a **system’s root filesystem**: it is
the filesystem that is directly mounted by the kernel during the booting phase and
that holds the system initialization scripts and the most essential system programs.
> @todo 所以，root mount 的代码在哪里 ?

#### 4.1 Namespaces
In a traditional Unix system, there is only one tree of mounted filesystems: starting
from the system’s root filesystem, each process can potentially access every file in a
mounted filesystem by specifying the proper pathname. In this respect, Linux 2.6 is
more refined: **every process might have its own tree of mounted filesystems—the socalled namespace of the process.**

However, a process gets a new namespace if it is created by the clone() system
call with the `CLONE_NEWNS` flag set (see the section “The clone(), fork(), and vfork()
System Calls” in Chapter 3). The new namespace is then inherited by children processes if the parent creates them without the `CLONE_NEWNS` flag.
> `CLONE_NEWNS` 不是 clone namespace 而是就是 clone mount namespace 的含义
> /home/shen/Core/linux/fs/namespace.c:copy_mnt_ns 值得分析一下

When a process mounts—or unmounts—a filesystem, it only modifies its
namespace. Therefore, the change is visible to all processes that share the same
namespace, and only to them. A process can even change the root filesystem of its
namespace by using the Linux-specific `pivot_root()` system call.
> @todo pivot_root 了解一下

As we will see in the next section, mounted filesystems are represented by `vfsmount` structures.

The root directory of a filesystem can be different from the root directory of a process: as we have seen in the
earlier section “Files Associated with a Process,” the process’s root directory is the directory corresponding
to the “/ ” pathname. By default, the process’ root directory coincides with the root directory of the system’s
root filesystem (or more precisely, with the root directory of the root filesystem in the namespace of the process,
described in the following section), but it can be changed by invoking the `chroot()` system call.

#### 4.2 Filesystem Mounting

However, Linux is different: it is possible to mount the same filesystem several times.
Of course, if a filesystem is mounted n times, its root directory can be accessed
through n mount points, one per mount operation. Although the same filesystem can
be accessed by using different mount points, it is really unique. Thus, there is only
**one superblock object** for all of them, no matter of how many times it has been
mounted.
> /dev/fd0 被多次 mount 之后，那么可以从多个位置进入该文件系统。
> Suppose that an Ext2 filesystem stored in the `/dev/fd0` floppy disk is mounted on `/flp` by issuing the command:
> ```c
> mount -t ext2 /dev/fd0 /flp
> ```

Mounted filesystems form a hierarchy: the mount point of a filesystem might be a
directory of a second filesystem, which in turn is already mounted over a third filesystem, and so on.

It is also possible to **stack multiple mounts** on a single mount point. Each new
mount on the same mount point hides the previously mounted filesystem, although
processes already using the files and directories under the old mount can continue to
do so. When the topmost mounting is removed, then the next lower mount is once
more made visible.

As you can imagine, keeping track of mounted filesystems can quickly become a
nightmare. For each mount operation, the kernel must save in memory the mount
point and the mount flags, as well as the relationships between the filesystem to be
mounted and the other mounted filesystems. Such information is stored in a
mounted filesystem descriptor of type `vfsmount`. 

The vfsmount data structures are kept in several doubly linked circular lists:
- A hash table indexed by the address of the vfsmount descriptor of the parent filesystem and the address of the dentry object of the mount point directory. The
hash table is stored in the `mount_hashtable` array, whose size depends on the
amount of RAM in the system. Each item of the table is the head of a circular
doubly linked list storing all descriptors that have the same hash value.
The `mnt_hash` field of the descriptor contains the pointers to adjacent elements in this list.
> @todo 这次一定
- For each namespace, a circular doubly linked list including all mounted filesystem descriptors belonging to the namespace.
The `list` field of the namespace
structure stores the head of the list, while the `mnt_list` field of the `vfsmount`
descriptor contains the pointers to adjacent elements in the list.
- For each mounted filesystem, a circular doubly linked list including all child
mounted filesystems. The head of each list is stored in the `mnt_mounts` field of the
mounted filesystem descriptor; moreover, the `mnt_child` field of the descriptor
stores the pointers to the adjacent elements in the list.


The `mnt_flags` field of the descriptor stores the value of several flags that specify how
some kinds of files in the mounted filesystem are handled. These flags, which can be
set through options of the mount command, are listed in Table 12-13.
Here are some functions that handle the mounted filesystem descriptors:
|Name       |Description|
|MNT_NOSUID |Forbid setuid and setgid flags in the mounted filesystem
|MNT_NODEV  |Forbid access to device files in the mounted filesystem
|MNT_NOEXEC |Disallow program execution in the mounted filesystem

- alloc_vfsmnt(name)

Allocates and initializes a mounted filesystem descriptor

- free_vfsmnt(mnt)

Frees a mounted filesystem descriptor pointed to by mnt

- lookup_mnt(mnt, dentry)

Looks up a descriptor in the hash table and returns its address

#### 4.3 Mounting a Generic Filesystem
The mount() system call is used to mount a generic filesystem; its `sys_mount()` service routine acts on the following parameters:
- The pathname of a **device file** containing the filesystem, or NULL if it is not
required (for instance, when the filesystem to be mounted is network-based)
- The pathname of the directory on which the filesystem will be mounted (the mount point)
- The filesystem type, which must be the name of a registered filesystem
- The mount flags (permitted values are listed in Table 12-14)
- A pointer to a filesystem-dependent data structure (which may be NULL)

> 注意 : 第一个参数是含有文件系统的 

Table 12-14. Flags used by the mount() system call
Macro     Description
MS_RDONLY Files can only be read
MS_NOSUID Forbid setuid and setgid flags
MS_NODEV Forbid access to device files
MS_NOEXEC Disallow program execution
MS_SYNCHRONOUS Write operations on files and directories are immediate
MS_REMOUNT Remount the filesystem changing the mount flags
MS_MANDLOCK Mandatory locking allowed
MS_DIRSYNC Write operations on directories are immediate
MS_NOATIME Do not update file access time
MS_NODIRATIME Do not update directory access time
MS_BIND Create a “bind mount,” which allows making a file or directory visible at another point of the system
directory tree (option --bind of the mount command)
MS_MOVE Atomically move a mounted filesystem to another mount point (option --move of the mount command)
MS_REC Recursively create “bind mounts” for a directory subtree
MS_VERBOSE Generate kernel messages on mount errors

The `do_mount()` function takes care of the actual mount operation by performing the
following operations:
> @todo 下面的内容描述很详细哦

Otherwise, it invokes `do_new_mount()`. This is the most common case. It is
triggered when the user asks to mount either a special filesystem or a regular filesystem stored in a disk partition.
`do_new_mount()` invokes the `do_kern_mount()` function passing to it the filesystem type, the mount flags, and the
block device name. This function, which takes care of the actual mount
operation and returns the address of a new mounted filesystem descriptor, is
described below. Next, `do_new_mount()` invokes `do_add_mount()`, which
essentially performs the following actions:

* ***The do_kern_mount() function***

The core of the mount operation is the `do_kern_mount()` function, which checks the
filesystem type flags to determine how the mount operation is to be done. 

The function takes care of the actual mount operation by performing essentially the
following operations:
1. Invokes `get_fs_type()` to search in the list of filesystem types and locate the
name stored in the fstype parameter; `get_fs_type()` returns in the local variable
type the address of the corresponding `file_system_type` descriptor.
2. Invokes `alloc_vfsmnt()` to allocate a new mounted filesystem descriptor and
stores its address in the mnt local variable.
3. Invokes the `type->get_sb()` filesystem-dependent function to allocate a new
superblock and to initialize it (see below).
4. Initializes the `mnt->mnt_sb` field with the address of the new superblock object.
5. Initializes the `mnt->mnt_root` field with the address of the dentry object corresponding
to the root directory of the filesystem, and increases the usage counter
of the dentry object.
6. Initializes the `mnt->mnt_parent` field with the value in mnt (for generic filesystems, the proper value of mnt_parent will be set when the mounted filesystem
descriptor is inserted in the proper lists by `graft_tree()`; 
7. Initializes the `mnt->mnt_namespace` field with the value in `current->namespace`.
8. Releases the `s_umount` read/write semaphore of the superblock object (it was
acquired when the object was allocated in step 3).
9. Returns the address mnt of the mounted filesystem object.

#### 4.4 Mounting the Root Filesystem
To keep the description simple, let’s assume that the root filesystem is stored in a partition of a hard disk (the most common case, after all). While the system boots, the
kernel finds the major number of the disk that contains the root filesystem in the `ROOT_DEV` variable (see Appendix A). The root filesystem can be specified as a device file in
the `/dev` directory either when compiling the kernel or by passing a suitable “root”
option to the initial bootstrap loader. Similarly, the mount flags of the root filesystem
are stored in the root_mountflags variable. The user specifies these flags either by using
the `rdev` external program on a compiled kernel image or by passing a suitable rootflags option to the initial bootstrap loader (see Appendix A).
> 1. 利用 [rdev 程序](https://linux.die.net/man/8/rdev)
> 2. bootstrap loader

Mounting the root filesystem is a two-stage procedure, shown in the following list:
1. The kernel mounts the special rootfs filesystem, which simply provides an empty directory that serves as initial mount point.
2. The kernel mounts the real root filesystem over the empty directory.

* ***Phase 1: Mounting the rootfs filesystem***
> 我曹，这里变化也太大了吧 !
> 是从 vfs_kern_mount 的位置开始利用 rootfs_fs_type 实现的

* ***Phase 2: Mounting the real root filesystem***

#### 4.5 Unmounting a Filesystem
> skip

## 5 Pathname Lookup
If the first character of the pathname is / , the pathname is absolute, and the search
starts from the directory identified by `current->fs->root` (the process root directory). Otherwise, the pathname is relative, and the search starts from the directory
identified by `current->fs->pwd` (the process-current directory).

Having in hand the dentry, and thus the inode, of the initial directory, the code
examines the entry matching the first name to derive the corresponding inode.
Then the **directory file** that has that inode is read from disk and the entry
matching the second name is examined to derive the corresponding inode. This procedure is repeated
for each name included in the path.

However, things are not as simple as they look, because the following Unix and VFS
filesystem features must be taken into consideration:
- The access rights of each directory must be checked to verify whether the process is allowed to read the directory’s content.
- **A filename can be a symbolic link that corresponds to an arbitrary pathname; in
this case, the analysis must be extended to all components of that pathname.**
- Symbolic links may induce circular references; the kernel must take this possibility into account and break endless loops when they occur.
- A filename can be the mount point of a mounted filesystem. This situation must
be detected, and the lookup operation must continue into the new filesystem.
- Pathname lookup has to be done inside the namespace of the process that issued
the system call. The same pathname used by two processes with different
namespaces may specify different files.
> @todo every item of them should taken into consideration !

To make things a bit easier, we first describe what `link_path_walk()` does when `LOOKUP_PARENT` is not set and the pathname does not contain symbolic links (standard pathname lookup).

Next, we discuss the case in which LOOKUP_PARENT is set: this type of lookup is required when creating, deleting, or renaming a directory entry, that is, during a parent pathname lookup.

Finally, we explain how the function resolves symbolic links.

#### 5.1 Standard Pathname Lookup

#### 5.2 Parent Pathname Lookup
In many cases, the real target of a lookup operation is not the last component of the
pathname, but the next-to-last one. For example, when a file is created, the last component denotes the filename of the not yet existing file, and the rest of the pathname
specifies the directory in which the new link must be inserted. Therefore, the lookup
operation should fetch the dentry object of the next-to-last component. For another
example, unlinking a file identified by the pathname /foo/bar consists of removing
bar from the directory foo. Thus, the kernel is really interested in accessing the directory foo rather than bar.

The `LOOKUP_PARENT` flag is used whenever the lookup operation must resolve the
directory containing the last component of the pathname, rather than the last component itself.

When the `LOOKUP_PARENT` flag is set, the link_path_walk() function also sets up the
`last` and `last_type` fields of the nameidata data structure. The last field stores the
name of the last component in the pathname. The last_type field identifies the type
of the last component; 

#### 5.3 Lookup of Symbolic Links
Recall that a symbolic link is a regular file that stores a pathname of another file. A
pathname may include symbolic links, and they must be resolved by the kernel.

The second pathname operation starts from the directory reached by the first operation and
continues until the last component of the symbolic link pathname has been resolved.
Next, the original lookup operation resumes from the dentry reached in the second
one and with the component following the symbolic link in the original pathname.
> find the dentry, **the dentry is a symbolic**, find again


To further complicate the scenario, the pathname included in a symbolic link may
include other symbolic links. You might think that the kernel code that resolves the
symbolic links is hard to understand, but this is not true; the code is actually quite
simple because it is recursive.

However, untamed recursion is intrinsically dangerous. For instance, suppose that a
symbolic link points to itself. Of course, resolving a pathname including such a symbolic link may induce an endless stream of recursive invocations, which in turn
quickly leads to a kernel stack overflow. The link_count field in the descriptor of the
current process is used to avoid the problem: the field is increased before each recursive execution and decreased right after. If a sixth nested lookup operation is
attempted, the whole lookup operation terminates with an error code. Therefore, the
level of nesting of symbolic links can be at most 5.

Furthermore, the `total_link_count` field in the descriptor of the current process
keeps track of how many symbolic links (even nonnested) were followed in the original lookup operation. If this counter reaches the value 40, the lookup operation
aborts. Without this counter, a malicious user could create a pathological pathname
including many consecutive symbolic links that freeze the kernel in a very long
lookup operation.
> recursion : nameidata::total_link_count

This is how the code basically works: once the `link_path_walk()` function retrieves
the dentry object associated with a component of the pathname, it checks whether
the corresponding inode object has a custom `follow_link` method (see step 5l and
step 14 in the section “Standard Pathname Lookup”). If so, the inode is a symbolic
link that must be interpreted before proceeding with the lookup operation of the
original pathname.


## 6 Implementations of VFS System Calls

#### 6.1 The open() System Call
> it describe something, but VFS has already became a monster

## 7 File Locking
The POSIX standard requires a file-locking mechanism based on the `fcntl()` system
call. It is possible to lock an arbitrary region of a file (even a single byte) or to lock
the whole file (including data appended in the future). Because a process can choose
to lock only a part of a file, it can also hold multiple locks on different parts of the
file.

Traditional BSD variants implement advisory locking through the `flock()` system
call. This call does not allow a process to lock a file region, only the whole file.
Traditional System V variants provide the `lockf()` library function, which is simply an
interface to `fcntl()`.

More importantly, System V Release 3 introduced mandatory locking: the kernel
checks that every invocation of the `open()`, `read()`, and `write()` system calls does
not violate a mandatory lock on the file being accessed. Therefore, **mandatory locks**
are enforced even between noncooperative processes.

#### 7.1 Linux File Locking
A process can get or release an advisory file lock on a file in two possible ways:
- By issuing the flock( ) system call. The two parameters of the system call are the
fd file descriptor, and a command to specify the lock operation. The lock applies
to the whole file.
- By using the `fcntl()` system call. The three parameters of the system call are the
fd file descriptor, a command to specify the lock operation, and a pointer to a
flock structure (see Table 12-20). A couple of fields in this structure allow the
process to specify the portion of the file to be locked. Processes can thus hold
several locks on different portions of the same file.

> @todo 所以 flock 和 fnctl 有什么区别吗 ? 是不是只是一个设置范围，一个不可以
> 2. 两者多事可以实现 advisory 和 mandatory 的吗 ?
> 似乎 7.3 和 7.4 分析了其中的内容

Handling leases is much simpler than handling mandatory locks: it is sufficient to
invoke a fcntl() system call with a `F_SETLEASE` or F_GETLEASE command. Another
`fcntl()` invocation with the F_SETSIG command may be used to change the type of
signal to be sent to the lease process holder.

#### 7.2 File-Locking Data Structures

```c
/*
 * struct file_lock represents a generic "file lock". It's used to represent
 * POSIX byte range locks, BSD (flock) locks, and leases. It's important to
 * note that the same struct is used to represent both a request for a lock and
 * the lock itself, but the same object is never used for both.
 *
 * FIXME: should we create a separate "struct lock_request" to help distinguish
 * these two uses?
 *
 * The varous i_flctx lists are ordered by:
 *
 * 1) lock owner
 * 2) lock range start
 * 3) lock range end
 *
 * Obviously, the last two criteria only matter for POSIX locks.
 */
struct file_lock {
	struct file_lock *fl_blocker;	/* The lock, that is blocking us */
	struct list_head fl_list;	/* link into file_lock_context */
	struct hlist_node fl_link;	/* node in global lists */
	struct list_head fl_blocked_requests;	/* list of requests with
						 * ->fl_blocker pointing here
						 */
	struct list_head fl_blocked_member;	/* node in
						 * ->fl_blocker->fl_blocked_requests
						 */
	fl_owner_t fl_owner;
	unsigned int fl_flags;
	unsigned char fl_type;
	unsigned int fl_pid;
	int fl_link_cpu;		/* what cpu's list is this on? */
	wait_queue_head_t fl_wait;
	struct file *fl_file;
	loff_t fl_start;
	loff_t fl_end;

	struct fasync_struct *	fl_fasync; /* for lease break notifications */
	/* for lease breaks: */
	unsigned long fl_break_time;
	unsigned long fl_downgrade_time;

	const struct file_lock_operations *fl_ops;	/* Callbacks for filesystems */
	const struct lock_manager_operations *fl_lmops;	/* Callbacks for lockmanagers */
	union {
		struct nfs_lock_info	nfs_fl;
		struct nfs4_lock_info	nfs4_fl;
		struct {
			struct list_head link;	/* link in AFS vnode's pending_locks list */
			int state;		/* state of grant or error if -ve */
			unsigned int	debug_id;
		} afs;
	} fl_u;
} __randomize_layout;
```

#### 7.3 FL_FLOCK Locks

#### 7.4 FL_POSIX Locks
