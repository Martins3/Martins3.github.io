# init/do_mount.c

1. init_mount_tree

```c
struct file_system_type rootfs_fs_type = {
	.name		= "rootfs",
	.init_fs_context = rootfs_init_fs_context,
	.kill_sb	= kill_litter_super,
};
```
