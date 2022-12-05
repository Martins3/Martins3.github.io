# fs/open.c 
1. contains definition of dozens of syscall


## file_open_name : swapon 遇到的，发现可以打开普通文件和设备

```c
/**
 * file_open_name - open file and return file pointer
 *
 * @name:	struct filename containing path to open
 * @flags:	open flags as per the open(2) second argument
 * @mode:	mode for the new file if O_CREAT is set, else ignored
 *
 * This is the helper to open a file from kernelspace if you really
 * have to.  But in generally you should not do this, so please move
 * along, nothing to see here..
 */
struct file *file_open_name(struct filename *name, int flags, umode_t mode)
{
	struct open_flags op;
	struct open_how how = build_open_how(flags, mode);
	int err = build_open_flags(&how, &op);
	if (err)
		return ERR_PTR(err);
	return do_filp_open(AT_FDCWD, name, &op);
}
```


## open
1. do_filp_open 
    1. create the `struct file`
    2. 

```c
static long do_sys_openat2(int dfd, const char __user *filename,
			   struct open_how *how)
{
	struct open_flags op;
	int fd = build_open_flags(how, &op); 
	struct filename *tmp;

	if (fd)
		return fd;

	tmp = getname(filename); //  read the file pathname from the process address space.
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);

	fd = get_unused_fd_flags(how->flags); //  find an empty slot in current->files->fd. The corresponding index (the new file descriptor) is stored in the fd local variable.
	if (fd >= 0) {
		struct file *f = do_filp_open(dfd, tmp, &op);
		if (IS_ERR(f)) {
			put_unused_fd(fd);
			fd = PTR_ERR(f);
		} else {
			fsnotify_open(f);
			fd_install(fd, f); // insert the file to slot 
		}
	}
	putname(tmp);
	return fd;
}
```

