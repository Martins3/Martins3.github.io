# fs/readdir.c
1. this file is aim at Man getdents(2)
    1. the example code in Man behave counter intuition : `linux_dirent::d_off`
2. https://stackoverflow.com/questions/36144807/why-does-linux-use-getdents-on-directories-instead-of-read @todo
3. https://lwn.net/Articles/606995/ @todo


## TODO
1. getdents and getdents64


## core struct
```c
struct getdents_callback {
	struct dir_context ctx;
	struct linux_dirent __user * current_dir;
	int prev_reclen;
	int count;
	int error;
};
```

## getdents
1. iterate_dir : 
```c
file->f_op->iterate_shared(file, ctx);
file->f_op->iterate(file, ctx);
```
2. filldir : copy data with `unsafe_put_user`



```c
SYSCALL_DEFINE3(getdents, unsigned int, fd,
		struct linux_dirent __user *, dirent, unsigned int, count)
{
	struct fd f;
	struct getdents_callback buf = {
		.ctx.actor = filldir,
		.count = count,
		.current_dir = dirent
	};
	int error;

	if (!access_ok(dirent, count))
		return -EFAULT;

	f = fdget_pos(fd); // dir has fd too !
	if (!f.file)
		return -EBADF;

	error = iterate_dir(f.file, &buf.ctx); // xxx critical
	if (error >= 0)
		error = buf.error;
	if (buf.prev_reclen) {
		struct linux_dirent __user * lastdirent;
		lastdirent = (void __user *)buf.current_dir - buf.prev_reclen;

		if (put_user(buf.ctx.pos, &lastdirent->d_off))
			error = -EFAULT;
		else
			error = count - buf.count;
	}
	fdput_pos(f);
	return error;
}
```
