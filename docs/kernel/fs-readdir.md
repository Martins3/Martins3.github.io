# 分析 fs/readdir.c
1. this file is aim at Man getdents(2)
    1. the example code in Man behave counter intuition : `linux_dirent::d_off`
## [Why does Linux use getdents() on directories instead of read()?](https://stackoverflow.com/questions/36144807/why-does-linux-use-getdents-on-directories-instead-of-read)
- [ ] 等待总结
## [Two paths to a better readdir()](https://lwn.net/Articles/606995/)
- [ ] 等待总结


ccls 索引的时候，但是没人用的 `__x64_sys_getdents`
```txt
@[
    __x64_sys_getdents64+5
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 3390
```

## 简单分析下具体的代码，只有 500 行
注册这个给具体的文件系统用
```c
	struct getdents_callback64 buf = {
		.ctx.actor = filldir64,
		.count = count,
		.current_dir = dirent
	};
```

- `__x64_sys_getdents64`
  - iterate_dir : 携带参数 getdents_callback64
    - file->f_op->iterate_shared(file, ctx);
      - xfs_dir_file_operations
    - file->f_op->iterate(file, ctx); # 目前的配置中，从来没有被调用过
