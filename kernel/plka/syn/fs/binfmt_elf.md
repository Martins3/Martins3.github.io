# binfmt_elf.c 


## TODO
1. lwn 的文章认真读一下，至少 load_elf_binary 其实没有什么特别复杂的东西在，另外两个需要好好分析

## Doc
- [建议全文背诵](https://lwn.net/Articles/631631/)

## KeyNote
1. 原来程序执行的一切秘密都在这里。
```c
static struct linux_binfmt elf_format = {
	.module		= THIS_MODULE,
	.load_binary	= load_elf_binary,
	.load_shlib	= load_elf_library,
	.core_dump	= elf_core_dump,
	.min_coredump	= ELF_EXEC_PAGESIZE,
};
```

## core struct
1. 不是没有 section 了吗 ? 还是说，section的作用是数据上，segment 是程序上的

```c
typedef struct elf64_hdr {
  unsigned char	e_ident[EI_NIDENT];	/* ELF "magic number" */
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;		/* Entry point virtual address */
  Elf64_Off e_phoff;		/* Program header table file offset */
  Elf64_Off e_shoff;		/* Section header table file offset */
  Elf64_Word e_flags;    // flags 没有怎么使用
  Elf64_Half e_ehsize;   // elf header size
  Elf64_Half e_phentsize;// program header entry size 吧 
  Elf64_Half e_phnum;    // program header number
  Elf64_Half e_shentsize;// section header entry size
  Elf64_Half e_shnum;    // section header number
  Elf64_Half e_shstrndx; // string 
} Elf64_Ehdr;

typedef struct elf64_phdr {
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;		/* Segment file offset */
  Elf64_Addr p_vaddr;		/* Segment virtual address */
  Elf64_Addr p_paddr;		/* Segment physical address */ // 一般不会使用上
  Elf64_Xword p_filesz;		/* Segment size in file */
  Elf64_Xword p_memsz;		/* Segment size in memory */
  Elf64_Xword p_align;		/* Segment alignment, file & memory */
} Elf64_Phdr;
```

## load_elf_phdrs

```c
/**
 * load_elf_phdrs() - load ELF program headers
 * @elf_ex:   ELF header of the binary whose program headers should be loaded
 * @elf_file: the opened ELF binary file
 *
 * Loads ELF program headers from the binary file elf_file, which has the ELF
 * header pointed to by elf_ex, into a newly allocated array. The caller is
 * responsible for freeing the allocated data. Returns an ERR_PTR upon failure.
 */
static struct elf_phdr *load_elf_phdrs(const struct elfhdr *elf_ex,
				       struct file *elf_file)
{
	struct elf_phdr *elf_phdata = NULL;
	int retval, err = -1;
	unsigned int size;

	/*
	 * If the size of this structure has changed, then punt, since
	 * we will be doing the wrong thing.
	 */
	if (elf_ex->e_phentsize != sizeof(struct elf_phdr))
		goto out;

	/* Sanity check the number of program headers... */
	/* ...and their total size. */
	size = sizeof(struct elf_phdr) * elf_ex->e_phnum;
	if (size == 0 || size > 65536 || size > ELF_MIN_ALIGN)
		goto out;

	elf_phdata = kmalloc(size, GFP_KERNEL);
	if (!elf_phdata)
		goto out;

	/* Read in the program headers */
	retval = elf_read(elf_file, elf_phdata, size, elf_ex->e_phoff); // 还是对于 vfs_read 的简单封装而已
	if (retval < 0) {
		err = retval;
		goto out;
	}

	/* Success! */
	err = 0;
out:
	if (err) {
		kfree(elf_phdata);
		elf_phdata = NULL;
	}
	return elf_phdata;
}
```



## load_elf_binary : core (1)
```c
/*
 * These are the functions used to load ELF style executables and shared
 * libraries.  There is no binary dependent code anywhere else.
 */

static int load_elf_binary(struct linux_binprm *bprm)
{
	struct file *interpreter = NULL; /* to shut gcc up */
 	unsigned long load_addr = 0, load_bias = 0;
	int load_addr_set = 0;
	unsigned long error;
	struct elf_phdr *elf_ppnt, *elf_phdata, *interp_elf_phdata = NULL;
	unsigned long elf_bss, elf_brk;
	int bss_prot = 0;
	int retval, i;
	unsigned long elf_entry;
	unsigned long e_entry;
	unsigned long interp_load_addr = 0;
	unsigned long start_code, end_code, start_data, end_data;
	unsigned long reloc_func_desc __maybe_unused = 0;
	int executable_stack = EXSTACK_DEFAULT;
	struct elfhdr *elf_ex = (struct elfhdr *)bprm->buf;
	struct {
		struct elfhdr interp_elf_ex;
	} *loc;
	struct arch_elf_state arch_state = INIT_ARCH_ELF_STATE;
	struct mm_struct *mm;
	struct pt_regs *regs;

	loc = kmalloc(sizeof(*loc), GFP_KERNEL);
	if (!loc) {
		retval = -ENOMEM;
		goto out_ret;
	}

	retval = -ENOEXEC;
	/* First of all, some simple consistency checks */
	if (memcmp(elf_ex->e_ident, ELFMAG, SELFMAG) != 0)
		goto out;

	if (elf_ex->e_type != ET_EXEC && elf_ex->e_type != ET_DYN)
		goto out;
	if (!elf_check_arch(elf_ex))
		goto out;
	if (elf_check_fdpic(elf_ex))
		goto out;
	if (!bprm->file->f_op->mmap)
		goto out;

	elf_phdata = load_elf_phdrs(elf_ex, bprm->file); // 将 program header 读入
	if (!elf_phdata)
		goto out;

	elf_ppnt = elf_phdata;
	for (i = 0; i < elf_ex->e_phnum; i++, elf_ppnt++) {
		char *elf_interpreter;

		if (elf_ppnt->p_type != PT_INTERP) // 本循环仅仅处理其中的 PT_INTERP 类型
			continue;

		/*
		 * This is the program interpreter used for shared libraries -
		 * for now assume that this is an a.out format binary.
		 */
		retval = -ENOEXEC;
		if (elf_ppnt->p_filesz > PATH_MAX || elf_ppnt->p_filesz < 2)
			goto out_free_ph;

		retval = -ENOMEM;
		elf_interpreter = kmalloc(elf_ppnt->p_filesz, GFP_KERNEL);
		if (!elf_interpreter)
			goto out_free_ph;

		retval = elf_read(bprm->file, elf_interpreter, elf_ppnt->p_filesz, // 将 PT_INTERP 类型的 Segment 读入
				  elf_ppnt->p_offset);
		if (retval < 0)
			goto out_free_interp;
		/* make sure path is NULL terminated */
		retval = -ENOEXEC;
		if (elf_interpreter[elf_ppnt->p_filesz - 1] != '\0')
			goto out_free_interp;

		interpreter = open_exec(elf_interpreter); // 获取到了该文件
		kfree(elf_interpreter); // 这个 section 已经没有什么作用了
		retval = PTR_ERR(interpreter);
		if (IS_ERR(interpreter))
			goto out_free_ph;

		/*
		 * If the binary is not readable then enforce mm->dumpable = 0
		 * regardless of the interpreter's permissions.
		 */
		would_dump(bprm, interpreter); // todo 暂时不懂

		/* Get the exec headers */
		retval = elf_read(interpreter, &loc->interp_elf_ex, // 获取 interp_elf_ex
				  sizeof(loc->interp_elf_ex), 0);
		if (retval < 0)
			goto out_free_dentry;

		break;

out_free_interp:
		kfree(elf_interpreter);
		goto out_free_ph;
	}

  // 实际上仅仅处理 stack 的可执行问题
	elf_ppnt = elf_phdata;
	for (i = 0; i < elf_ex->e_phnum; i++, elf_ppnt++)
		switch (elf_ppnt->p_type) {
		case PT_GNU_STACK:
			if (elf_ppnt->p_flags & PF_X)
				executable_stack = EXSTACK_ENABLE_X;
			else
				executable_stack = EXSTACK_DISABLE_X;
			break;

		case PT_LOPROC ... PT_HIPROC:
			retval = arch_elf_pt_proc(elf_ex, elf_ppnt, // nop
						  bprm->file, false,
						  &arch_state);
			if (retval)
				goto out_free_dentry;
			break;
		}

	/* Some simple consistency checks for the interpreter */
	if (interpreter) {
		retval = -ELIBBAD;
		/* Not an ELF interpreter */
		if (memcmp(loc->interp_elf_ex.e_ident, ELFMAG, SELFMAG) != 0)
			goto out_free_dentry;
		/* Verify the interpreter has a valid arch */
		if (!elf_check_arch(&loc->interp_elf_ex) || // 检查一下架构之类的
		    elf_check_fdpic(&loc->interp_elf_ex))
			goto out_free_dentry;

		/* Load the interpreter program headers */
		interp_elf_phdata = load_elf_phdrs(&loc->interp_elf_ex,
						   interpreter);
		if (!interp_elf_phdata)
			goto out_free_dentry;

		/* Pass PT_LOPROC..PT_HIPROC headers to arch code */
		elf_ppnt = interp_elf_phdata;
		for (i = 0; i < loc->interp_elf_ex.e_phnum; i++, elf_ppnt++) // todo 其实这里什么都没有做
			switch (elf_ppnt->p_type) {
			case PT_LOPROC ... PT_HIPROC:
				retval = arch_elf_pt_proc(&loc->interp_elf_ex,
							  elf_ppnt, interpreter,
							  true, &arch_state);
				if (retval)
					goto out_free_dentry;
				break;
			}
	}

	/*
	 * Allow arch code to reject the ELF at this point, whilst it's
	 * still possible to return an error to the code that invoked
	 * the exec syscall.
	 */
	retval = arch_check_elf(elf_ex,
				!!interpreter, &loc->interp_elf_ex,
				&arch_state); // nop
	if (retval)
		goto out_free_dentry;

	/* Flush all traces of the currently running executable */
	retval = flush_old_exec(bprm); // todo 
	if (retval)
		goto out_free_dentry;

	/* Do this immediately, since STACK_TOP as used in setup_arg_pages
	   may depend on the personality.  */
	SET_PERSONALITY2(*elf_ex, &arch_state); // todo personality 的作用是什么 ?
	if (elf_read_implies_exec(*elf_ex, executable_stack))
		current->personality |= READ_IMPLIES_EXEC;

	if (!(current->personality & ADDR_NO_RANDOMIZE) && randomize_va_space)
		current->flags |= PF_RANDOMIZE;

	setup_new_exec(bprm); // todo 处理的事情很杂乱
	install_exec_creds(bprm); // todo 处理 cred

	/* Do this so that we can load the interpreter, if need be.  We will
	   change some of these later */
	retval = setup_arg_pages(bprm, randomize_stack_top(STACK_TOP), // todo 似乎开始的时候处理过 arg pages 相关的事情 XXX 这里也是 stack 初始化的位置
				 executable_stack);
	if (retval < 0)
		goto out_free_dentry;
	
	elf_bss = 0;
	elf_brk = 0;

	start_code = ~0UL;
	end_code = 0;
	start_data = 0;
	end_data = 0;

	/* Now we do a little grungy work by mmapping the ELF image into
	   the correct location in memory. */
	for(i = 0, elf_ppnt = elf_phdata;
	    i < elf_ex->e_phnum; i++, elf_ppnt++) {
		int elf_prot, elf_flags;
		unsigned long k, vaddr;
		unsigned long total_size = 0;

		if (elf_ppnt->p_type != PT_LOAD)
			continue;

		if (unlikely (elf_brk > elf_bss)) {
			unsigned long nbyte;
	            
			/* There was a PT_LOAD segment with p_memsz > p_filesz
			   before this one. Map anonymous pages, if needed,
			   and clear the area.  */
			retval = set_brk(elf_bss + load_bias, // XXX set_brk 
					 elf_brk + load_bias,
					 bss_prot);
			if (retval)
				goto out_free_dentry;
			nbyte = ELF_PAGEOFFSET(elf_bss);
			if (nbyte) {
				nbyte = ELF_MIN_ALIGN - nbyte;
				if (nbyte > elf_brk - elf_bss)
					nbyte = elf_brk - elf_bss;
				if (clear_user((void __user *)elf_bss +
							load_bias, nbyte)) { // todo arch 相关的内容
					/*
					 * This bss-zeroing can fail if the ELF
					 * file specifies odd protections. So
					 * we don't check the return value
					 */
				}
			}
		}

		elf_prot = make_prot(elf_ppnt->p_flags);

		elf_flags = MAP_PRIVATE | MAP_DENYWRITE | MAP_EXECUTABLE;

		vaddr = elf_ppnt->p_vaddr;
		/*
		 * If we are loading ET_EXEC or we have already performed
		 * the ET_DYN load_addr calculations, proceed normally.
		 */
		if (elf_ex->e_type == ET_EXEC || load_addr_set) {
			elf_flags |= MAP_FIXED;
		} else if (elf_ex->e_type == ET_DYN) { // todo 已经有点看不懂了
			/*
			 * This logic is run once for the first LOAD Program
			 * Header for ET_DYN binaries to calculate the
			 * randomization (load_bias) for all the LOAD
			 * Program Headers, and to calculate the entire
			 * size of the ELF mapping (total_size). (Note that
			 * load_addr_set is set to true later once the
			 * initial mapping is performed.)
			 *
			 * There are effectively two types of ET_DYN
			 * binaries: programs (i.e. PIE: ET_DYN with INTERP)
			 * and loaders (ET_DYN without INTERP, since they
			 * _are_ the ELF interpreter). The loaders must
			 * be loaded away from programs since the program
			 * may otherwise collide with the loader (especially
			 * for ET_EXEC which does not have a randomized
			 * position). For example to handle invocations of
			 * "./ld.so someprog" to test out a new version of
			 * the loader, the subsequent program that the
			 * loader loads must avoid the loader itself, so
			 * they cannot share the same load range. Sufficient
			 * room for the brk must be allocated with the
			 * loader as well, since brk must be available with
			 * the loader.
			 *
			 * Therefore, programs are loaded offset from
			 * ELF_ET_DYN_BASE and loaders are loaded into the
			 * independently randomized mmap region (0 load_bias
			 * without MAP_FIXED).
			 */
			if (interpreter) {
				load_bias = ELF_ET_DYN_BASE;
				if (current->flags & PF_RANDOMIZE)
					load_bias += arch_mmap_rnd();
				elf_flags |= MAP_FIXED;
			} else
				load_bias = 0;

			/*
			 * Since load_bias is used for all subsequent loading
			 * calculations, we must lower it by the first vaddr
			 * so that the remaining calculations based on the
			 * ELF vaddrs will be correctly offset. The result
			 * is then page aligned.
			 */
			load_bias = ELF_PAGESTART(load_bias - vaddr);

			total_size = total_mapping_size(elf_phdata,
							elf_ex->e_phnum);
			if (!total_size) {
				retval = -EINVAL;
				goto out_free_dentry;
			}
		}

		error = elf_map(bprm->file, load_bias + vaddr, elf_ppnt, // XXX 千呼万唤始出来
				elf_prot, elf_flags, total_size);
		if (BAD_ADDR(error)) {
			retval = IS_ERR((void *)error) ?
				PTR_ERR((void*)error) : -EINVAL;
			goto out_free_dentry;
		}

		if (!load_addr_set) {
			load_addr_set = 1;
			load_addr = (elf_ppnt->p_vaddr - elf_ppnt->p_offset);
			if (elf_ex->e_type == ET_DYN) {
				load_bias += error -
				             ELF_PAGESTART(load_bias + vaddr);
				load_addr += load_bias;
				reloc_func_desc = load_bias;
			}
		}
		k = elf_ppnt->p_vaddr;
		if ((elf_ppnt->p_flags & PF_X) && k < start_code)
			start_code = k;
		if (start_data < k)
			start_data = k;

		/*
		 * Check to see if the section's size will overflow the
		 * allowed task size. Note that p_filesz must always be
		 * <= p_memsz so it is only necessary to check p_memsz.
		 */
		if (BAD_ADDR(k) || elf_ppnt->p_filesz > elf_ppnt->p_memsz ||
		    elf_ppnt->p_memsz > TASK_SIZE ||
		    TASK_SIZE - elf_ppnt->p_memsz < k) {
			/* set_brk can never work. Avoid overflows. */
			retval = -EINVAL;
			goto out_free_dentry;
		}

		k = elf_ppnt->p_vaddr + elf_ppnt->p_filesz;

		if (k > elf_bss)
			elf_bss = k;
		if ((elf_ppnt->p_flags & PF_X) && end_code < k)
			end_code = k;
		if (end_data < k)
			end_data = k;
		k = elf_ppnt->p_vaddr + elf_ppnt->p_memsz;
		if (k > elf_brk) {
			bss_prot = elf_prot;
			elf_brk = k;
		}
	}

	e_entry = elf_ex->e_entry + load_bias;
	elf_bss += load_bias;
	elf_brk += load_bias;
	start_code += load_bias;
	end_code += load_bias;
	start_data += load_bias;
	end_data += load_bias;

	/* Calling set_brk effectively mmaps the pages that we need
	 * for the bss and break sections.  We must do this before
	 * mapping in the interpreter, to make sure it doesn't wind
	 * up getting placed where the bss needs to go.
	 */
	retval = set_brk(elf_bss, elf_brk, bss_prot); // todo 和前面的 set_brk 有什么关系 ?
	if (retval)
		goto out_free_dentry;
	if (likely(elf_bss != elf_brk) && unlikely(padzero(elf_bss))) {
		retval = -EFAULT; /* Nobody gets to see this, but.. */
		goto out_free_dentry;
	}

	if (interpreter) { // todo 所以在 load elf 的过程中间， interpreter 的作用到底是什么 ?
		elf_entry = load_elf_interp(&loc->interp_elf_ex,
					    interpreter,
					    load_bias, interp_elf_phdata);
		if (!IS_ERR((void *)elf_entry)) {
			/*
			 * load_elf_interp() returns relocation
			 * adjustment
			 */
			interp_load_addr = elf_entry;
			elf_entry += loc->interp_elf_ex.e_entry;
		}
		if (BAD_ADDR(elf_entry)) {
			retval = IS_ERR((void *)elf_entry) ?
					(int)elf_entry : -EINVAL;
			goto out_free_dentry;
		}
		reloc_func_desc = interp_load_addr;

		allow_write_access(interpreter);
		fput(interpreter);
	} else {
		elf_entry = e_entry;
		if (BAD_ADDR(elf_entry)) {
			retval = -EINVAL;
			goto out_free_dentry;
		}
	}

	kfree(interp_elf_phdata);
	kfree(elf_phdata);

	set_binfmt(&elf_format);

#ifdef ARCH_HAS_SETUP_ADDITIONAL_PAGES
	retval = arch_setup_additional_pages(bprm, !!interpreter);
	if (retval < 0)
		goto out;
#endif /* ARCH_HAS_SETUP_ADDITIONAL_PAGES */

	retval = create_elf_tables(bprm, elf_ex,
			  load_addr, interp_load_addr, e_entry);
	if (retval < 0)
		goto out;

	mm = current->mm;
	mm->end_code = end_code;
	mm->start_code = start_code;
	mm->start_data = start_data;
	mm->end_data = end_data;
	mm->start_stack = bprm->p;

	if ((current->flags & PF_RANDOMIZE) && (randomize_va_space > 1)) {
		/*
		 * For architectures with ELF randomization, when executing
		 * a loader directly (i.e. no interpreter listed in ELF
		 * headers), move the brk area out of the mmap region
		 * (since it grows up, and may collide early with the stack
		 * growing down), and into the unused ELF_ET_DYN_BASE region.
		 */
		if (IS_ENABLED(CONFIG_ARCH_HAS_ELF_RANDOMIZE) &&
		    elf_ex->e_type == ET_DYN && !interpreter) {
			mm->brk = mm->start_brk = ELF_ET_DYN_BASE;
		}

		mm->brk = mm->start_brk = arch_randomize_brk(mm);
#ifdef compat_brk_randomized
		current->brk_randomized = 1;
#endif
	}

	if (current->personality & MMAP_PAGE_ZERO) {
		/* Why this, you ask???  Well SVr4 maps page 0 as read-only,
		   and some applications "depend" upon this behavior.
		   Since we do not have the power to recompile these, we
		   emulate the SVr4 behavior. Sigh. */
		error = vm_mmap(NULL, 0, PAGE_SIZE, PROT_READ | PROT_EXEC,
				MAP_FIXED | MAP_PRIVATE, 0);
	}

	regs = current_pt_regs();
#ifdef ELF_PLAT_INIT
	/*
	 * The ABI may specify that certain registers be set up in special
	 * ways (on i386 %edx is the address of a DT_FINI function, for
	 * example.  In addition, it may also specify (eg, PowerPC64 ELF)
	 * that the e_entry field is the address of the function descriptor
	 * for the startup routine, rather than the address of the startup
	 * routine itself.  This macro performs whatever initialization to
	 * the regs structure is required as well as any relocations to the
	 * function descriptor entries when executing dynamically links apps.
	 */
	ELF_PLAT_INIT(regs, reloc_func_desc);
#endif

	finalize_exec(bprm); // todo 最终
	start_thread(regs, elf_entry, bprm->p); // todo 开始
	retval = 0;
out:
	kfree(loc);
out_ret:
	return retval;

	/* error cleanup */
out_free_dentry:
	kfree(interp_elf_phdata);
	allow_write_access(interpreter);
	if (interpreter)
		fput(interpreter);
out_free_ph:
	kfree(elf_phdata);
	goto out;
}
```


## load_elf_library : core (2)

## elf_core_dump : core (3)
