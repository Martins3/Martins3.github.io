# Professional Linux Kernel Architecture : Modules
Modules are an efficient way of adding device drivers, filesystems and other components dynamically into the Linux kernel without having to build a new kernel or reboot the system
## 7.1 Overview
Modules have many advantages, of which the following are worthy of particular mention
1. By using modules, distributors are able to pre-compile a comprehensive collection of
drivers without bloating the size of the kernel image beyond bounds. After automatic
hardware detection or user prompting, the installation routine selects the appropriate
modules and adds them into the kernel.
2. Kernel developers can pack experimental code into modules that can be unloaded and
reloaded after each modification. This allows new features to be tested quickly without
having to reboot the system each time.
> 用户和开发者都有好处

License issues can also be resolved with the help of modules.
## 7.2 Using Modules

#### 7.2.1 Adding and Removing
> modprobe 是什么东西

The actions needed when loading a module show strong similarities with the linking of application
programs by means of ld and with the use of dynamic libraries with ld.so. 
Externally, **modules are just normal relocatable object files**, as a `file` call will quickly confirm
They are, of course, neither executable files nor program libraries as normally found in system programming; however, the basic structure of the binary module file is based on the same scheme also used for
the above purposes.


Addresses
are again given in relative and not absolute units. However, it is the kernel itself and not the dynamic
loader that performs relocation.


When the system call(init_module) is processed, the module code is first copied from the kernel into kernel memory;
this is followed by relocation and the resolution of as yet undefined references in the module

**Handling Unresolved References**
The `nm` tool can be used to generate a list of all external functions in a module

Note that if your kernel was not built with
KALLSYMS_ALL enabled, generic_ro_fops will not be visible. Only symbols of functions but no other
symbols like constant structures as generic_ro_fops are included in this case
> KALLSYMS_ALL 选项是什么作用


this list shows the memory addresses together with the
corresponding function names and can be accessed via the proc filesystem, this being the purpose of
the file /proc/kallsyms:

#### 7.2.2 Dependencies
The `depmod` tool in the modutils standard tool collection is used to calculate the dependencies between
the modules of a system

```sh
cat /lib/modules/5.1.18-1-MANJARO/modules.dep
```
This information is processed by modprobe, which is used to insert modules into the kernel if existing
dependencies are to be resolved automatically. 

Most symbols to which the modules refer are not defined in other modules but in the kernel itself.
For this reason, the file `/lib/modules/version/System.map` is generated (likewise using depmod) when
modules are installed. 
> 这一个选项消失了

#### 7.2.3 Querying Module Information
Additional sources of information are text descriptions that specify the purpose and usage of modules
and are stored directly in the binary files. They can be queried using the modinfo tool in the modutils
distribution. Various items of data are stored:

#### 7.2.4 Automatic Loading
How is this possible? When the kernel processes the mount system call, it discovers that no information
on the desired filesystem — vfat — is present in its data structures. It therefore attempts to load the corresponding module using the request_module function whose exact structure is discussed in Section 7.4.1.
This function uses the kmod mechanism to start the modprobe tool, which then inserts the vfat module in
the usual way

The solution to the problem is a small ‘‘database‘‘ that is attached to every module.
The contents describe which devices are supported by the module.

The database information is provided via module aliases. These are generic identifiers for modules that
encode the described pieces of information. The macro `MODULE_ALIAS` is used to generate module
aliases.


Providing module aliases forms the basis to solve the automatic module loading problem, but is not
yet completely sufficient. The kernel needs some support from userspace. After the kernel has noticed
that it needs a module for a device with specific properties, it needs to pass an appropriate request to
a userspace daemon. This daemon then seeks the apt module and inserts it into the kernel. Section 7.4
describes how this is implemented.

```c
/* For userspace: you can also call me... */
#define MODULE_ALIAS(_alias) MODULE_INFO(alias, _alias)

/* Generic info of form tag = "info" */
#define MODULE_INFO(tag, info) __MODULE_INFO(tag, tag, info)
```



## 7.3 Inserting and Deleting Modules
Two system calls form the interface between the userspace tools and the module implementation of the kernel:
1. `init_module` — Inserts a new module into the kernel. All the userspace tool needs do is provide
the *binary data*. All further steps (particularly relocation and symbol resolution) are performed in the kernel itself.
2. `delete_module` — Removes a module from the kernel. A prerequisite is, of course, that the
code is no longer in use and that no other modules are employing functions exported from the module.

There is also a function named `request_module` (not a system call) that is used to load modules from the
kernel side. It is required not only to load modules but also to implement hotplug capabilities.




##### 7.3.1 Module Representation


`module.h` `struct module`

struct module的编译选项:
1. `KALLSYMS` is a configuration option (but only for embedded systems — it is always enabled on
regular machines) that holds in memory a list of all symbols defined in the kernel itself and in the
loaded modules (otherwise only the exported functions are stored). 
This is useful if oops messages (which are used if the kernel detects a deviation from the normal behavior, for example, if
a NULL pointer is de-referenced) are to output not only hexadecimal numbers but also the names
of the functions involved.
2. In contrast to kernel versions prior to 2.5, the ability to unload modules must now be configured explicitly. The required additional information is not included in the module data structure
unless the configuration option `MODULE_UNLOAD` is selected.

struct module相关的配置:
1. `MODVERSIONS` enables version control; this prevents an obsolete module whose interface definitions no longer match those of the current version from loading into the kernel. Section 7.5 deals
with this in more detail.
2. `MODULE_FORCE_UNLOAD` enables modules to be removed from the kernel by force, even if there
are still references to the module or the code is being used by other modules. This brute force
method is never needed in normal operation but can be useful during development.
3. `KMOD` enables the kernel to automatically load modules once they are needed. This requires some
interaction with the userspace, which is described below in the chapter.

The elements of `struct module` have the following meaning:
1. `module_state` indicates the current state of the module and can assume one of the values of `module_state`
```c
enum module_state {
	MODULE_STATE_LIVE,	/* Normal state. */
	MODULE_STATE_COMING,	/* Full formed, running module_init. */
	MODULE_STATE_GOING,	/* Going away. */
	MODULE_STATE_UNFORMED,	/* Still setting it up. */
};
```
2. `list` is a standard list element used by the kernel to keep all loaded modules in a doubly linked
list
3. `name` specifies the name of the module. 
4. `syms`, `num_syms`, and `crc` are used to manage the symbols exported by the module. syms is an
array of num_syms entries of the kernel_symbol type and is responsible for assigning identifiers
(name) to memory addresses (value).
`crcs` is also an array with `num_syms` entries that store checksums for the exported symbols
needed to implement version control.
5. When symbols are exported, the kernel considers not only symbols that may be used by all
modules regardless of their license, but also symbols that may be used only by modules with
GPL and GPL-compatible licenses.
The
gpl_syms, num_gpl_syms and gpl_crcs elements are provided for GPL-only symbols, while
gpl_future_syms, num_gpl_future_syms and gpl_future_crcs serve for future GPL-only symbols.

6. If a module defines new exceptions (see Chapter 4), their description is held in the extable array.
num_exentries specifies the number of entries in the array.

7. `init` is a pointer to a function called when the module is initialized.
8. The binary data of a module are divided into two parts: the initialization part and the core part.
The former contains everything that can be discarded after loading has terminated (e.g., the initialization functions). The latter contains all data needed during the current operation. The start
address of the initialization part is held in `module_init` and comprises `init_size` bytes, whereas
the core area is described by `module_core` and `core_size`.
9. `arch` is a processor-specific hook that, depending on the particular system, can be filled with
various additional data needed to run modules. Most architectures do not require any additional information and therefore define struct mod_arch_specific as an empty structure that is
removed by the compiler during optimization.
10. `taints` is set if a module taints the kernel. Tainting means that the kernel suspects the module
of doing something harmful that could prevent correct kernel operation. Should a kernel panic
occur, then the error diagnosis will also contain information about why the kernel is tainted.
This helps developers to distinguish bug reports coming from properly running systems and
those where something was already suspicious.
The function `add_taint_module` is provided to taint a given instance of struct module. A module can taint the kernel for two reasons:
11. `license_gplok` is a Boolean variable that specifies whether the module license is GPLcompatible; in other words, whether GPL-exported functions may be used or not. The flag is set
when the module is inserted into the kernel. How the kernel judges a license to be compatible
with the GPL or not is discussed below
12. `module_ref` is used for reference counting
13. `modules_which_use_me` is used as a list element in the data structures that describe the intermodule dependencies in the kernel
14. waiter is a pointer to the task structure of the process that caused the module to be unloaded
and is now waiting for the action to terminate.
15. exit is the counterpart to init. It is a pointer to a function called to perform module-specific
clean-up work (e.g., releasing reserved memory areas) when a module is removed.
16. `symtab`, `num_symtab` and `strtab` are used to record information on all symbols of the module
(not only on the explicitly exported symbols).
17. `percpu` points to per-CPU data that belong to the module. It is initialized when the module is
loaded.
18. `args` is a pointer to the command-line arguments passed to the module during loading

#### 7.3.2 Dependencies and References

```c
/* modules using other modules: kdb wants to see this. */
struct module_use {
	struct list_head source_list;
	struct list_head target_list;
	struct module *source, *target;
};

/* Does a already use b? */
static int already_uses(struct module *a, struct module *b)


/*
 * Module a uses b
 *  - we add 'a' as a "source", 'b' as a "target" of module use
 *  - the module_use is added to the list of 'b' sources (so
 *    'b' can walk the list to see who sourced them), and of 'a'
 *    targets (so 'a' can see what modules it targets).
 */
static int add_module_usage(struct module *a, struct module *b)


/* Clear the unload stuff of the module. */
static void module_unload_free(struct module *mod)

static inline void print_unload_info(struct seq_file *m, struct module *mod)
```
> 上面罗列一些使用 module_use 的函数，都是简单的函数，但是使用的规则没有深究，应该不麻烦

**Manipulating Data Structures**
> 介绍了上面罗列的函数

#### 7.3.3 Binary Structure of Modules
Modules use the **ELF** binary format, which features several additional sections not present in normal programs or libraries

In addition to a few compiler-generated sections that are not relevant for our
purposes (mainly relocation sections), modules consist of the following ELF sections:
1. The `__ksymtab`, `__ksymtab_gpl`, and `__ksymtab_gpl_future` sections contain a symbol table
with all symbols exported by the module. Whereas the symbols in the first-named section can be
used by all kernel parts regardless of the license, symbols in `__kysmtab_gpl` may be used only
by GPL-compatible parts, and those in `__ksymtab_gpl_future` only by GPL-compatible parts in
the future.

2. `__kcrctab`, `__kcrctab_gpl`, and `__kcrctab_gpl_future` contain checksums for all (GPL, or
future-GPL) exported functions of the module. `__versions` includes the checksums for all 
references used by the module from external sources.
3. `__param` stores information on the parameters accepted by a module.
4. `__ex_table` is used to define new entries for the exception table of the kernel in case the module
code needs this mechanism.
5. `.modinfo` stores the names of all other modules that must reside in the kernel before a module
can be loaded — in other words, the names of all modules that the particular module
depends on.
In addition, each module can hold specific information that can be queried using the modinfo
userspace tool, particularly the name of the author, a description of the module, license information, and a list of parameters.
6. `.exit.text` contains code (and possibly data) required when the module is removed from the
kernel. This information is not kept in the normal text segment so that the kernel need not load it
into memory if the option for removing modules was not enabled in the kernel configuration.
7. The initialization functions (and data) are stored in ``.init.text.`` They are held in a separate
section because they are no longer needed after completion of initialization and can therefore
be removed from memory.
8. `.gnu.linkonce.this_module` provides an instance of struct module, which stores the name of
the module (name) and pointers to the initialization and clean-up functions (init and cleanup)
in the binary file. By referring to this section, the kernel recognizes whether a specific binary file
is a module or not. If it is missing, file loading is rejected.


```c
/**
 * module_init() - driver initialization entry point
 * @x: function to be run at kernel boot time or module insertion
 *
 * module_init() will either be called during do_initcalls() (if
 * builtin) or at module insertion time (if a module).  There can only
 * be one per module.
 */
#define module_init(x)	__initcall(x);

/**
 * module_exit() - driver exit entry point
 * @x: function to be run when driver is removed
 *
 * module_exit() will wrap the driver clean-up code
 * with cleanup_module() when used with rmmod when
 * the driver is a module.  If the driver is statically
 * compiled into the kernel, module_exit() has no effect.
 * There can only be one per module.
 */
#define module_exit(x)	__exitcall(x);
```

```c
#define __init		__section(.init.text) __cold notrace __noretpoline
#define __initdata	__section(.init.data)
#define __initconst	__constsection(.init.rodata)
#define __exitdata	__section(.exit.data)
#define __exit_call	__used __section(.exitcall.exit)
```

The kernel provides two macros for exporting symbols — EXPORT_SYMBOL and EXPORT_SYMBOL_GPL. As
their names suggest, a distinction is made between exporting general symbols and exporting symbols
that may be used only by GPL-compatible code. Again, their purpose is to place the symbols in the
appropriate section of the module binary image:

**Exporting Symbols**
The module initialization and clean-up functions are stored in the module instance in the
`.gnu.linkonce.module` section.


**Exporting Symbols**
The kernel provides two macros for exporting symbols — `EXPORT_SYMBOL` and `EXPORT_SYMBOL_GPL`. As
their names suggest, a distinction is made between exporting general symbols and exporting symbols
that may be used only by GPL-compatible code

```c
#define EXPORT_SYMBOL(sym)					\
	__EXPORT_SYMBOL(sym, "")
```
> 还有若干级别的展开

Two code sections are generated for each exported symbol. They serve the following purpose:
1. `__kstrtab_function` is stored in the `__ksymtab_strings` section as a statically defined variable.
Its value is a string that corresponds to the name of the (function) function.
2. A `kernel_symbol` instance is stored in the `__ksymtab` (or `__kstrtab_gpl`) section. It consists of a
pointer to the exported function and a pointer to the entry just created in the string table.
This allows the kernel to find the matching code address by reference to the function name in the
string; this is needed when resolving references.

`__CRC_SYMBOL` is used when kernel version control is enabled for exported functions (refer to Section 7.5
for further details); otherwise, it is defined as an empty string as I have assumed here for simplicity’s
sake.


**General Module Information**
```c
/* generic info of form tag = "info" */
#define module_info(tag, info) __module_info(tag, tag, info)

#define __MODULE_INFO(tag, name, info)					  \
static const char __UNIQUE_ID(name)[]					  \
  __used __attribute__((section(".modinfo"), unused, aligned(1)))	  \
  = __stringify(tag) "=" info
```

In addition to this general macro that generates tag = info entries, there are a range of macros that create
entries with pre-defined meanings. These are discussed below.
1. Module license
2. Author and Description
3. Alternative Name
4. Elementary Version Control
> 其他的简洁调用 MODULE_INFO 的macro


#### 7.3.4 Inserting Modules
**System Call Implementation**
The `init_module` system call is the interface between userspace and kernel and is used to load new modules.

`syscall.h`
```c
  asmlinkage long sys_init_module(void __user *umod, unsigned long len,
          const char __user *uargs);
```
```c
  SYSCALL_DEFINE3(init_module, void __user *, umod,
      unsigned long, len, const char __user *, uargs) {
    int err;
    struct load_info info = { };
    may_init_module();
    copy_module_from_user(umod, len, &info);
    return load_module(&info, uargs, 0);
  }
```
```c
  /* Allocate and load the module: note that size of section 0 is always
     zero, and we rely on this for optional sections. */
  static int load_module(struct load_info *info, const char __user *uargs,
             int flags)
  {
    struct module *mod;
    long err;
    char *after_dashes;

    err = module_sig_check(info, flags);

    err = elf_header_check(info);

    /* Figure out module layout, and allocate all the memory. */
    mod = layout_and_allocate(info, flags);

    /* Reserve our place in the list. */
    err = add_unformed_module(mod);

#ifdef CONFIG_MODULE_SIG
    mod->sig_ok = info->sig_ok;
    if (!mod->sig_ok) {
      pr_notice_once("%s: module verification failed: signature "
               "and/or required key missing - tainting "
               "kernel\n", mod->name);
      add_taint_module(mod, TAINT_UNSIGNED_MODULE, LOCKDEP_STILL_OK);
    }
#endif

    /* To avoid stressing percpu allocator, do this once we're unique. */
    err = percpu_modalloc(mod, info);

    /* Now module is in final location, initialize linked lists, etc. */
    err = module_unload_init(mod);

    init_param_lock(mod);

    /* Now we've got everything in the final locations, we can
     * find optional sections. */
    err = find_module_sections(mod, info);

    err = check_module_license_and_versions(mod);

    /* Set up MODINFO_ATTR fields */
    setup_modinfo(mod, info);

    /* Fix up syms, so that st_value is a pointer to location. */
    err = simplify_symbols(mod, info);

    err = apply_relocations(mod, info);

    err = post_relocation(mod, info);

    flush_module_icache(mod);

    /* Now copy in args */
    mod->args = strndup_user(uargs, ~0UL >> 1);

    dynamic_debug_setup(info->debug, info->num_debug);

    /* Ftrace init must be called in the MODULE_STATE_UNFORMED state */
    ftrace_module_init(mod);

    /* Finally it's fully formed, ready to start executing. */
    err = complete_formation(mod, info);

    /* Module is ready to execute: parsing args may do that. */
    after_dashes = parse_args(mod->name, mod->args, mod->kp, mod->num_kp,
            -32768, 32767, mod,
            unknown_module_param_cb);

    /* Link in to syfs. */
    err = mod_sysfs_setup(mod, info, mod->kp, mod->num_kp);

    /* Get rid of temporary copy. */
    free_copy(info);

    /* Done! */
    trace_module_load(mod);

    return do_init_module(mod);
  }
```
The binary data are transferred into the kernel address space using `load_module`. All required relocations
are performed, and all references are resolved. The arguments are converted into a form that is easy to
analyze (a table of `kernel_param` instances), and an instance of the module data structure is created with
all the necessary information on the module.

Once the `module` instance created in the `load_module` function has been added to the global modules list,
all the kernel need do is to call the module initialization function and free the memory occupied by the
initialization data.

**Loading Modules**
`load_module` assumes the following tasks:
1. Copying module data (and arguments) from userspace into a temporary memory location in kernel address space; the relative addresses of the ELF sections are replaced with absolute addresses of the temporary image.
2. Finding the positions of the (optional) sections
3. Ensuring that the version control string and the definition of struct module match in the kernel and module
4. Distributing the existing sections to their final positions in memory
5. **Relocating symbols and resolving references**. Any version control information linked with the module symbols is noted.
6. Processing the arguments of the module
> 首先，我们需要搞清楚, 一般的二进制文件的加载的过程是什么样子的? 并且是如何启动的?
> 1. 为什么需要将module data 复制到 temporary memory 中间，这是模块所特有的东西吗?
> 2. 为什么需要做符号处理(又是特有的东西吗 ?)

> 接下来分析load_modules的细节

1. Rewriting Section Addresses
```c
/*
 * Set up our basic convenience variables (pointers to section headers,
 * search for module section index etc), and do some basic section
 * verification.
 *
 * Set info->mod to the temporary copy of the module in info->hdr. The final one
 * will be allocated in move_module().
 */
static int setup_load_info(struct load_info *info, int flags){
...
	/* Find internal symbols and strings. */
	for (i = 1; i < info->hdr->e_shnum; i++) {
		if (info->sechdrs[i].sh_type == SHT_SYMTAB) {
			info->index.sym = i;
			info->index.str = info->sechdrs[i].sh_link;
			info->strtab = (char *)info->hdr
				+ info->sechdrs[info->index.str].sh_offset;
			break;
		}
...
}
```

Iteration through all sections is used to find the position of the symbol table (the only section whose type
is `SHT_SYMTAB`) and of the associated symbol string table whose section is linked with the symbol table
using the ELF link feature.

2. Finding Section Addresses

In section `.gnu.linkonce.this_module`, there is an instance of struct module (find_sec is an auxiliary
function that finds the index of an ELF section by reference to its name)
```c
// 依旧在setup_load_info 中间
	info->index.mod = find_sec(info, ".gnu.linkonce.this_module");
	if (!info->index.mod) {
		pr_warn("%s: No module found in object\n",
			info->name ?: "(missing .modinfo name field)");
		return -ENOEXEC;
	}
	/* This is temporary: point mod into copy of data. */
	info->mod = (void *)info->hdr + info->sechdrs[info->index.mod].sh_offset;
```

3. Organizing Data in Memory

`layout_sections` is used to decide which sections of the module are to be loaded at which positions in
memory or which modules must be copied from their temporary address. The sections are split into two
parts: `core` and `init`. While the first contains all code sections required during the entire run time of the
module, the kernel places all initialization data and functions in a separate part that is removed when
loading is completed.

Module sections are not transferred to their final memory position unless the SHF_ALLOC flag is set in
their header

```c
	/* Figure out module layout, and allocate all the memory. */
static struct module *layout_and_allocate(struct load_info *info, int flags) // 在layout_and_allocate 中间分别调用 layout_sections 和 move_module 


/* Lay out the SHF_ALLOC sections in a way not dissimilar to how ld
   might -- code, read-only data, read-write data, small data.  Tally
   sizes, and place the offsets into sh_entsize fields: high bit means it
   belongs in init. */
static void layout_sections(struct module *mod, struct load_info *info)
{
```

4. Transferring Data

```c
static int move_module(struct module *mod, struct load_info *info)
```

5. Querying the Module License
Technically insignificant but important from a legal point of view — the module license can now be read
from the .modinfo section and placed in the module data structure:
```c
	err = check_module_license_and_versions(mod);
```

6. Resolving References and Relocation
> 其实关键内容在下面，但是同时介绍了
```c
static int find_module_sections(struct module *mod, struct load_info *info)
```
The next step in module loading is to place the table of (GPL-) exported symbols in the kernel by setting
the `num_syms`, `syms` and `crcindex` elements (or their GPL equivalents) to the corresponding memory
locations of the binary data.
> 其实是在初始化module 中间的成员


**Resolving References**
```c
	err = simplify_symbols(mod, info);


/* Change all symbols so that st_value encodes the pointer directly. */
static int simplify_symbols(struct module *mod, const struct load_info *info){
...

		case SHN_UNDEF:
			ksym = resolve_symbol_wait(mod, info, name);
			/* Ok if resolved.  */
			if (ksym && !IS_ERR(ksym)) {
				sym[i].st_value = kernel_symbol_value(ksym);
				break;
			}

			/* Ok if weak.  */
			if (!ksym && ELF_ST_BIND(sym[i].st_info) == STB_WEAK)
				break;

			ret = PTR_ERR(ksym) ?: -ENOENT;
			pr_warn("%s: Unknown symbol %s (err %d)\n",
				mod->name, name, ret);
			break;
...


/* Resolve a symbol for this module.  I.e. if we find one, record usage. */
static const struct kernel_symbol *resolve_symbol(struct module *mod,
						  const struct load_info *info,
						  const char *name,
						  char ownername[])
{
...
	sym = find_symbol(name, &owner, &crc,
			  !(mod->taints & (1 << TAINT_PROPRIETARY_MODULE)), true);
	if (!sym)
		goto unlock;

	if (!check_version(info, name, mod, crc)) {
		sym = ERR_PTR(-EINVAL);
		goto getname;
	}

	err = ref_module(mod, owner);
...


/* Find an exported symbol and return it, along with, (optional) crc and
 * (optional) module which owns it.  Needs preempt disabled or module_mutex. */
const struct kernel_symbol *find_symbol(const char *name,
```
> 1. 实际上的调用包含 wait 的机制
> 2. 搜索机制比想象的简单，大概就是对于指定的区间进行二分查找就可以了，但是指定区间是什么，如何维护，并不清楚
> 3. @question 所以到底如何实现解析符号的


#### 7.3.5 Removing Modules
```c
SYSCALL_DEFINE2(delete_module, const char __user *, name_user,
		unsigned int, flags)
{
	struct module *mod;
	char name[MODULE_NAME_LEN];
	int ret, forced = 0;

	if (!capable(CAP_SYS_MODULE) || modules_disabled)
		return -EPERM;

	if (strncpy_from_user(name, name_user, MODULE_NAME_LEN-1) < 0)
		return -EFAULT;
	name[MODULE_NAME_LEN-1] = '\0';

	audit_log_kern_module(name);

	if (mutex_lock_interruptible(&module_mutex) != 0)
		return -EINTR;

	mod = find_module(name);
	if (!mod) {
		ret = -ENOENT;
		goto out;
	}

	if (!list_empty(&mod->source_list)) {
		/* Other modules depend on us: get rid of them first. */
		ret = -EWOULDBLOCK;
		goto out;
	}

	/* Doing init or already dying? */
	if (mod->state != MODULE_STATE_LIVE) {
		/* FIXME: if (force), slam module count damn the torpedoes */
		pr_debug("%s already dying\n", mod->name);
		ret = -EBUSY;
		goto out;
	}

	/* If it has an init func, it must have an exit func to unload */
	if (mod->init && !mod->exit) {
		forced = try_force_unload(flags);
		if (!forced) {
			/* This module can't be removed */
			ret = -EBUSY;
			goto out;
		}
	}

	/* Stop the machine so refcounts can't move and disable module. */
	ret = try_stop_module(mod, flags, &forced);
	if (ret != 0)
		goto out;

	mutex_unlock(&module_mutex);
	/* Final destruction now no one is using it. */
	if (mod->exit != NULL)
		mod->exit();
	blocking_notifier_call_chain(&module_notify_list,
				     MODULE_STATE_GOING, mod);
	klp_module_going(mod);
	ftrace_release_mod(mod);

	async_synchronize_full();

	/* Store the name of the last unloaded module for diagnostic purposes */
	strlcpy(last_unloaded_module, mod->name, sizeof(last_unloaded_module));

	free_module(mod);
	return 0;
out:
	mutex_unlock(&module_mutex);
	return ret;
```
> 这一个函数结构清晰明了，大概就是　找到，检查依赖关系，调用`mod->exit()`，释放内存


## 7.4 Automation and Hotplugging
Modules can be loaded not only on the initiative of the user or by means of an automated script, but can
also be requested by the kernel itself. There are two situations where this kind of loading is useful:
1. The kernel establishes that a required function is not available.
For example, a filesystem needs to be mounted but is not supported by the kernel.
The kernel can attempt to load the required module and then retry file mounting.

2. A new device is connected to a hotpluggable bus (USB, FireWire, PCI, etc.). The kernel
detects the new device and automatically loads the module with the appropriate driver

@todo 实在是过于细节了
#### 7.4.1 Automatic Loading with kmod


#### 7.4.2 Hotplugging


## 7.5 Version Control
@todo 实在是过于细节了

#### 7.5.1 Checksum Methods
**Generating a Checksum**



#### 7.5.2 Version Control Functions



**TODO**  问题:
1. 模块和设备的关系是什么? 
https://unix.stackexchange.com/questions/47208/what-is-the-difference-between-kernel-drivers-and-kernel-modules
没有包含的关系，只是含有很大的交集而已。


2. initrd 
https://en.wikipedia.org/wiki/Initial_ramdisk
