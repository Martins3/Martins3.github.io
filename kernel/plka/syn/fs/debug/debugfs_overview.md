# fs/debug

## KeyNote
1. inode.c : create file and dir
2. file.c : io for different data type
    1. debugfs_create_u64
    2. debugfs_create_u64

## Doc
http://tinylab.org/show-the-usage-of-procfs-sysfs-debugfs/
https://lwn.net/Articles/115405/


## Todo
1. make the stupid `my_debugfs.c` work
2. what's relation with libfs(simple_read_from_buffer, ...)

## debugfs_create_dir

## debugfs_create_file



##  debugfs_remove
1. start_creating debugfs_remove
2. @todo I guess, debugfs_init register the file system, and start_creating, debugfs_remove handle the mount problem.

```c
static int __init debugfs_init(void)
{
	int retval;

	retval = sysfs_create_mount_point(kernel_kobj, "debug");
	if (retval)
		return retval;

	retval = register_filesystem(&debug_fs_type);
	if (retval)
		sysfs_remove_mount_point(kernel_kobj, "debug");
	else
		debugfs_registered = true;

	return retval;
}
```

