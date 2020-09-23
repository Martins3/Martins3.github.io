# fs/exec.c 的分析

## KeyNote
1. 本文处理 : 各种 exec 的辅助函数 以及 syscall的封装

## TODO
1. sched_exec
2. script 格式将会是我们的好朋友的 !
3. force_sigsegv 移植都想知道的 sigsegv 

2. setup_new_exec
3. copy_strings 

## core struct
1. linux_binprm : 在读取程序之前就拥有的各种基本属性
2. linux_binfmt : 标准接口

```c
/*
 * This structure is used to hold the arguments that are used when loading binaries.
 */
struct linux_binprm {
#ifdef CONFIG_MMU
	struct vm_area_struct *vma;
	unsigned long vma_pages;
#else
# define MAX_ARG_PAGES	32
	struct page *page[MAX_ARG_PAGES];
#endif
	struct mm_struct *mm;
	unsigned long p; /* current top of mem */
	unsigned long argmin; /* rlimit marker for copy_strings() */
	unsigned int
		/*
		 * True after the bprm_set_creds hook has been called once
		 * (multiple calls can be made via prepare_binprm() for
		 * binfmt_script/misc).
		 */
		called_set_creds:1,
		/*
		 * True if most recent call to the commoncaps bprm_set_creds
		 * hook (due to multiple prepare_binprm() calls from the
		 * binfmt_script/misc handlers) resulted in elevated
		 * privileges.
		 */
		cap_elevated:1,
		/*
		 * Set by bprm_set_creds hook to indicate a privilege-gaining
		 * exec has happened. Used to sanitize execution environment
		 * and to set AT_SECURE auxv for glibc.
		 */
		secureexec:1;
#ifdef __alpha__
	unsigned int taso:1;
#endif
	unsigned int recursion_depth; /* only for search_binary_handler() */
	struct file * file;
	struct cred *cred;	/* new credentials */
	int unsafe;		/* how unsafe this exec is (mask of LSM_UNSAFE_*) */
	unsigned int per_clear;	/* bits to clear in current->personality */
	int argc, envc;
	const char * filename;	/* Name of binary as seen by procps */
	const char * interp;	/* Name of the binary really executed. Most
				   of the time same as filename, but could be
				   different for binfmt_{misc,script} */
	unsigned interp_flags;
	unsigned interp_data;
	unsigned long loader, exec;

	struct rlimit rlim_stack; /* Saved RLIMIT_STACK used during exec. */

	char buf[BINPRM_BUF_SIZE];
} __randomize_layout;
```

```c
/*
 * This structure defines the functions that are used to load the binary formats that
 * linux accepts.
 */
struct linux_binfmt {
	struct list_head lh;
	struct module *module;
	int (*load_binary)(struct linux_binprm *);
	int (*load_shlib)(struct file *);
	int (*core_dump)(struct coredump_params *cprm);
	unsigned long min_coredump;	/* minimal dump size */
} __randomize_layout;
```




## `__do_execve_file` : syscall API 最终指向的位置
1. execve
2. execveat
3. uselib : 其替代品是什么 ?

```c
/*
 * sys_execve() executes a new program.
 */
static int __do_execve_file(int fd, struct filename *filename,
			    struct user_arg_ptr argv,
			    struct user_arg_ptr envp,
			    int flags, struct file *file)
{
	char *pathbuf = NULL;
	struct linux_binprm *bprm;
	struct files_struct *displaced;
	int retval;

	if (IS_ERR(filename))
		return PTR_ERR(filename);

	/*
	 * We move the actual failure in case of RLIMIT_NPROC excess from
	 * set*uid() to execve() because too many poorly written programs
	 * don't check setuid() return code.  Here we additionally recheck
	 * whether NPROC limit is still exceeded.
	 */
	if ((current->flags & PF_NPROC_EXCEEDED) &&
	    atomic_read(&current_user()->processes) > rlimit(RLIMIT_NPROC)) {
		retval = -EAGAIN;
		goto out_ret;
	}

	/* We're below the limit (still or again), so we don't want to make
	 * further execve() calls fail. */
	current->flags &= ~PF_NPROC_EXCEEDED;

	retval = unshare_files(&displaced); // todo 可以深入分析
	if (retval)
		goto out_ret;

	retval = -ENOMEM;
	bprm = kzalloc(sizeof(*bprm), GFP_KERNEL);
	if (!bprm)
		goto out_files;

	retval = prepare_bprm_creds(bprm); // todo cred 相关初始化
	if (retval)
		goto out_free;

	check_unsafe_exec(bprm); // todo cred 检查相关
	current->in_execve = 1;

	if (!file)
		file = do_open_execat(fd, filename, flags); // 打开文件，只是增加非常多的封装而已
	retval = PTR_ERR(file);
	if (IS_ERR(file))
		goto out_unmark;

	sched_exec(); // todo 

	bprm->file = file;
	if (!filename) {
		bprm->filename = "none";
	} else if (fd == AT_FDCWD || filename->name[0] == '/') {
		bprm->filename = filename->name;
	} else {
		if (filename->name[0] == '\0')
			pathbuf = kasprintf(GFP_KERNEL, "/dev/fd/%d", fd);
		else
			pathbuf = kasprintf(GFP_KERNEL, "/dev/fd/%d/%s",
					    fd, filename->name);
		if (!pathbuf) {
			retval = -ENOMEM;
			goto out_unmark;
		}
		/*
		 * Record that a name derived from an O_CLOEXEC fd will be
		 * inaccessible after exec. Relies on having exclusive access to
		 * current->files (due to unshare_files above).
		 */
		if (close_on_exec(fd, rcu_dereference_raw(current->files->fdt)))
			bprm->interp_flags |= BINPRM_FLAGS_PATH_INACCESSIBLE;
		bprm->filename = pathbuf;
	}
	bprm->interp = bprm->filename;

  // todo 初始化一个几乎默认的设置
  // 1. cr3 寄存器被修改了吗 ? 
  // 2. cache 和 tlb 被刷新了 ?
	retval = bprm_mm_init(bprm);
	if (retval)
		goto out_unmark;

	retval = prepare_arg_pages(bprm, argv, envp); // 初始化参数需要的空间
	if (retval < 0)
		goto out;

	retval = prepare_binprm(bprm); // 调用 lms 的安全模块，然后从文件中间到发 bprm 的 BUF 中间
	if (retval < 0)
		goto out;

  // Like copy_strings, but get argv and its values from kernel memory
  // todo copy_strings 可以分析一下
	retval = copy_strings_kernel(1, &bprm->filename, bprm); 
	if (retval < 0)
		goto out;

	bprm->exec = bprm->p;
	retval = copy_strings(bprm->envc, envp, bprm);
	if (retval < 0)
		goto out;

	retval = copy_strings(bprm->argc, argv, bprm);
	if (retval < 0)
		goto out;

	would_dump(bprm, bprm->file); // todo 这个真的不理解啊 !

	retval = exec_binprm(bprm); // XXX 核心业务，
	if (retval < 0)
		goto out;

  // 当代码到达此处的时候，事情已经做完了。
	/* execve succeeded */
	current->fs->in_exec = 0;
	current->in_execve = 0;
	rseq_execve(current); // resq syscall 
	acct_update_integrals(current); // 统计的数据
	task_numa_free(current, false); // nop
	free_bprm(bprm);
	kfree(pathbuf);
	if (filename)
		putname(filename);
	if (displaced)
		put_files_struct(displaced);
	return retval;

out:
	if (bprm->mm) {
		acct_arg_size(bprm, 0);
		mmput(bprm->mm);
	}

out_unmark:
	current->fs->in_exec = 0;
	current->in_execve = 0;

out_free:
	free_bprm(bprm);
	kfree(pathbuf);

out_files:
	if (displaced)
		reset_files_struct(displaced);
out_ret:
	if (filename)
		putname(filename);
	return retval;
}
```

## exec_binprm : 真正的入口

```c
static int exec_binprm(struct linux_binprm *bprm)
{
	pid_t old_pid, old_vpid;
	int ret;

	/* Need to fetch pid before load_binary changes it */
	old_pid = current->pid;
	rcu_read_lock();
	old_vpid = task_pid_nr_ns(current, task_active_pid_ns(current->parent)); // todo 终于遇到你
	rcu_read_unlock();

	ret = search_binary_handler(bprm);
	if (ret >= 0) {
		audit_bprm(bprm);
		trace_sched_process_exec(current, old_pid, bprm);
		ptrace_event(PTRACE_EVENT_EXEC, old_vpid);
		proc_exec_connector(current);
	}

	return ret;
}
```

```c
#define printable(c) (((c)=='\t') || ((c)=='\n') || (0x20<=(c) && (c)<=0x7e))
/*
 * cycle the list of binary formats handler, until one recognizes the image
 */
int search_binary_handler(struct linux_binprm *bprm)
{
	bool need_retry = IS_ENABLED(CONFIG_MODULES);
	struct linux_binfmt *fmt;
	int retval;

	/* This allows 4 levels of binfmt rewrites before failing hard. */
	if (bprm->recursion_depth > 5)
		return -ELOOP;

	retval = security_bprm_check(bprm);
	if (retval)
		return retval;

	retval = -ENOENT;
 retry:
	read_lock(&binfmt_lock);
	list_for_each_entry(fmt, &formats, lh) {
		if (!try_module_get(fmt->module)) // todo 既然都加入到列表中间了，为什么会出现 module 没有使用
			continue;
		read_unlock(&binfmt_lock);

		bprm->recursion_depth++; // todo 为什么会递归
		retval = fmt->load_binary(bprm); // XXX 进入到具体的信息中间
		bprm->recursion_depth--;

		read_lock(&binfmt_lock);
		put_binfmt(fmt);
		if (retval < 0 && !bprm->mm) {
			/* we got to flush_old_exec() and failed after it */
			read_unlock(&binfmt_lock);
			force_sigsegv(SIGSEGV); // todo
			return retval;
		}
		if (retval != -ENOEXEC || !bprm->file) {
			read_unlock(&binfmt_lock);
			return retval;
		}
	}
	read_unlock(&binfmt_lock);

	if (need_retry) {
		if (printable(bprm->buf[0]) && printable(bprm->buf[1]) &&
		    printable(bprm->buf[2]) && printable(bprm->buf[3]))
			return retval;
		if (request_module("binfmt-%04x", *(ushort *)(bprm->buf + 2)) < 0)
			return retval;
		need_retry = false;
		goto retry;
	}

	return retval;
}
EXPORT_SYMBOL(search_binary_handler);
```


## `__register_binfmt`
1. 除非各种格式都是 module 才需要动态注册: 是
2. 默认注释只有 elf 和 script 

```c
void __register_binfmt(struct linux_binfmt * fmt, int insert)
{
	BUG_ON(!fmt);
	if (WARN_ON(!fmt->load_binary))
		return;
	write_lock(&binfmt_lock);
	insert ? list_add(&fmt->lh, &formats) :
		 list_add_tail(&fmt->lh, &formats);
	write_unlock(&binfmt_lock);
}
```

