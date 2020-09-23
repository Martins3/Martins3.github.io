# ramfs , tmpfs , rootfs, initramfs , initrd and kernfs

> https://blog.csdn.net/The_K_is_on_the_way/article/details/80486840
> will be deleted soon !

https://lwn.net/Articles/571590/

> kernfs exclusively deals with sysfs_dirent, which will
> be later renamed to kernfs_node, and kernfs_ops.  sysfs becomes a
> **wrapping layer** over sysfs which interfaces `kobject` and `[bin_]attribute`.
> 
> The goal of these changes is to allow other users to make use of the
> core features of sysfs instead of rolling their own pseudo filesystem
> implementation which usually fails to deal with issues with file
> shutdowns, locking separation from vfs layer and so on.  This patchset
> refactors sysfs and separates out most core functionalities to kernfs;
> however, the mount code hasn't been updated yet and it can't be used
> by other users yet.  The patchset is pretty big already and the two
> steps can be separated relatively well, so I think this is a good
> split point.

