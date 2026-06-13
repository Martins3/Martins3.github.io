# 分析 mount 技术

# fs/fs_context.md

## Documentation/filesystems/mount_api.txt

> The mount context is created by calling vfs_new_fs_context() or
> vfs_dup_fs_context() and is destroyed with put_fs_context().  Note that the
> structure is not refcounted.
>
> VFS, security and filesystem mount options are set individually with
> vfs_parse_mount_option().  Options provided by the old mount(2) system call as
> a page of data can be parsed with generic_parse_monolithic().

什么 option

> When mounting, the filesystem is allowed to take data from any of the pointers
> and attach it to the superblock (or whatever), provided it clears the pointer
> in the mount context.
>
> The filesystem is also allowed to allocate resources and pin them with the
> mount context.  For instance, NFS might pin the appropriate protocol version
> module.
