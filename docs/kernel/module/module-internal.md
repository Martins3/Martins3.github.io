## 有趣的

```c
SYSCALL_DEFINE2(delete_module, const char __user *, name_user,
		unsigned int, flags)
```
这一个函数结构清晰明了，大概就是　找到，检查依赖关系，调用`mod->exit()`，释放内存

## 如下几个问题到时候看看

1. cat /proc/kallsyms 的实现
2. 因为 ftrace 是知道 module 信息的，很自然，ftrace 需要在任何模块加载的时候加以分析
- delete_module
  - ftrace_release_mod(mod);
- load_module
    ftrace_module_init(mod);




## 这下面的内容都整理下 : Professional Linux Kernel Architecture : Modules

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

struct module 的编译选项:
1. `KALLSYMS` is a configuration option (but only for embedded systems — it is always enabled on
regular machines) that holds in memory a list of all symbols defined in the kernel itself and in the
loaded modules (otherwise only the exported functions are stored).
This is useful if oops messages (which are used if the kernel detects a deviation from the normal behavior, for example, if
a NULL pointer is de-referenced) are to output not only hexadecimal numbers but also the names
of the functions involved.
2. In contrast to kernel versions prior to 2.5, the ability to unload modules must now be configured explicitly. The required additional information is not included in the module data structure
unless the configuration option `MODULE_UNLOAD` is selected.

struct module 相关的配置:
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
> 其他的简洁调用 MODULE_INFO 的 macro


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
> 1. 为什么需要将 module data 复制到 temporary memory 中间，这是模块所特有的东西吗?
> 2. 为什么需要做符号处理(又是特有的东西吗 ?)

> 接下来分析 load_modules 的细节

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
> 其实是在初始化 module 中间的成员


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

https://unix.stackexchange.com/questions/47208/what-is-the-difference-between-kernel-drivers-and-kernel-modules
https://en.wikipedia.org/wiki/Initial_ramdisk

显然这个 module 也是可以的

## upstream 的问题

### 1 内核没有处理好 module 之间的关系
但是话又说回来，make allmodules 的时候不会又这个问题吗?

在自己构建的内核中，如果删掉 sudo rmmod virtio_pci ，会触发这个错误:
```txt
[   76.220082] BUG: kernel NULL pointer dereference, address: 0000000000000008
[   76.220797] #PF: supervisor read access in kernel mode
[   76.220970] #PF: error_code(0x0000) - not-present page
[   76.220970] PGD 0 P4D 0
[   76.220970] Oops: Oops: 0000 [#1] PREEMPT SMP NOPTI
[   76.220970] CPU: 1 UID: 0 PID: 2564 Comm: rmmod Kdump: loaded Not tainted 6.12.1 #63
[   76.220970] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2012-3-4
[   76.220970] RIP: 0010:vp_del_vq.isra.0+0x3e/0x70 [virtio_pci]
[   76.220970] Code: 48 8b 85 28 04 00 00 48 89 df ff d0 0f 1f 00 48 89 df 5b 5d 41 5c e9 71 da f6 c0 4c 8d a7 a0 03 00 00 4c 89 e7 e8 92 33 d0 c1 <48> 8b 53 08 4c 89 e7 48 89 c6 48 8b 43 10 48 89 42 08 48 89 10 48
[   76.220970] RSP: 0018:ffffc9000110fc68 EFLAGS: 00010046
[   76.220970] RAX: 0000000000000246 RBX: 0000000000000000 RCX: 000000000080007d
[   76.220970] RDX: 0000000000000001 RSI: 0000000000000000 RDI: ffffffff82119cf3
[   76.220970] RBP: ffff888007851000 R08: 0000000000000001 R09: ffffffffc0416e62
[   76.220970] R10: 0000000000000960 R11: 0000000000000000 R12: ffff8880078513a0
[   76.220970] R13: ffff8880078512f8 R14: 0000000000000000 R15: ffff888040d4a0c0
[   76.220970] FS:  00007f30a3bb1040(0000) GS:ffff88803e840000(0000) knlGS:0000000000000000
[   76.220970] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[   76.220970] CR2: 0000000000000008 CR3: 00000000098e8000 CR4: 0000000000752ef0
[   76.220970] PKRU: 55555554
[   76.220970] Call Trace:
[   76.220970]  <TASK>
[   76.220970]  ? __die+0x23/0x70
[   76.220970]  ? page_fault_oops+0x17d/0x550
[   76.220970]  ? schedule_timeout+0x15d/0x170
[   76.220970]  ? exc_page_fault+0x79/0x180
[   76.220970]  ? asm_exc_page_fault+0x26/0x30
[   76.220970]  ? vp_del_vqs+0x72/0x1c0 [virtio_pci]
[   76.220970]  ? _raw_spin_lock_irqsave+0x23/0x60
[   76.220970]  ? vp_del_vq.isra.0+0x3e/0x70 [virtio_pci]
[   76.220970]  ? vp_del_vq.isra.0+0x3e/0x70 [virtio_pci]
[   76.220970]  vp_del_vqs+0x72/0x1c0 [virtio_pci]
[   76.220970]  virtballoon_remove+0xbe/0x120 [virtio_balloon]
[   76.220970]  virtio_dev_remove+0x3e/0xa0
[   76.220970]  device_release_driver_internal+0x19f/0x200
[   76.220970]  bus_remove_device+0xc6/0x130
[   76.220970]  device_del+0x163/0x3e0
[   76.220970]  device_unregister+0x17/0x60
[   76.220970]  unregister_virtio_device+0x15/0x30
[   76.220970]  virtio_pci_remove+0x3f/0x80 [virtio_pci]
[   76.220970]  pci_device_remove+0x3f/0xb0
[   76.220970]  device_release_driver_internal+0x19f/0x200
[   76.220970]  driver_detach+0x48/0x90
[   76.220970]  bus_remove_driver+0x6d/0xf0
[   76.220970]  pci_unregister_driver+0x3f/0x90
[   76.220970]  __do_sys_delete_module+0x1ae/0x290
[   76.220970]  do_syscall_64+0xbc/0x210
[   76.220970]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
[   76.220970] RIP: 0033:0x7f30a352c06b
[   76.220970] Code: 73 01 c3 48 8b 0d ad 4d 0c 00 f7 d8 64 89 01 48 83 c8 ff c3 66 2e 0f 1f 84 00 00 00 00 00 90 f3 0f 1e fa b8 b0 00 00 00 0f 05 <48> 3d 01 f0 ff ff 73 01 c3 48 8b 0d 7d 4d 0c 00 f7 d8 64 89 01 48
[   76.220970] RSP: 002b:00007fffb9bc5a78 EFLAGS: 00000206 ORIG_RAX: 00000000000000b0
[   76.220970] RAX: ffffffffffffffda RBX: 0000559e321737d0 RCX: 00007f30a352c06b
[   76.220970] RDX: 0000000000000000 RSI: 0000000000000800 RDI: 0000559e32173838
[   76.220970] RBP: 0000000000000000 R08: 1999999999999999 R09: 0000000000000000
[   76.220970] R10: 00007f30a359e560 R11: 0000000000000206 R12: 0000559e321737d0
[   76.220970] R13: 00007fffb9bc5cd0 R14: 0000559e321732a0 R15: 0000000000000000
[   76.220970]  </TASK>
[   76.220970] Modules linked in: nf_tables iptable_nat configfs dm_mod dax kvm_intel 9pnet_virtio kvm crc32c_intel 9pnet nvme virtio_balloon virtio_console netfs nvme_core virtio_net nvme_auth virtio_scsi sch_fq_codel fuse virtio_pci(-) virtio_pci_modern_dev virtio_pci_legacy_dev
[   76.220970] CR2: 0000000000000008
[   76.220970] ---[ end trace 0000000000000000 ]---
[   76.220970] RIP: 0010:vp_del_vq.isra.0+0x3e/0x70 [virtio_pci]
[   76.220970] Code: 48 8b 85 28 04 00 00 48 89 df ff d0 0f 1f 00 48 89 df 5b 5d 41 5c e9 71 da f6 c0 4c 8d a7 a0 03 00 00 4c 89 e7 e8 92 33 d0 c1 <48> 8b 53 08 4c 89 e7 48 89 c6 48 8b 43 10 48 89 42 08 48 89 10 48
[   76.220970] RSP: 0018:ffffc9000110fc68 EFLAGS: 00010046
[   76.220970] RAX: 0000000000000246 RBX: 0000000000000000 RCX: 000000000080007d
[   76.220970] RDX: 0000000000000001 RSI: 0000000000000000 RDI: ffffffff82119cf3
[   76.220970] RBP: ffff888007851000 R08: 0000000000000001 R09: ffffffffc0416e62
[   76.220970] R10: 0000000000000960 R11: 0000000000000000 R12: ffff8880078513a0
[   76.220970] R13: ffff8880078512f8 R14: 0000000000000000 R15: ffff888040d4a0c0
[   76.220970] FS:  00007f30a3bb1040(0000) GS:ffff88803e840000(0000) knlGS:0000000000000000
[   76.220970] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[   76.220970] CR2: 0000000000000008 CR3: 00000000098e8000 CR4: 0000000000752ef0
[   76.220970] PKRU: 55555554
[   76.220970] note: rmmod[2564] exited with irqs disabled
[   76.267483] note: rmmod[2564] exited with preempt_count 1
```

## 有趣
rmmod 判断一个模块是不是 built-in 的方法是通过 module.builtin 而不是
通过 sysfs 的，在调试虚拟机的时候，/lib/modules/6.13.1/ 下没有被同步，结果发现:
```txt
🧀  lsmod | grep rfkill
rfkill                 40960  0
~ 🌳
🧀  sudo rmmod rfkill
rmmod: ERROR: Module rfkill is builtin.
```

## 似乎 version 也是 module 和 kernel 的关系
如果我们

```txt
[ 7053.402735] kvm_amd: version magic '6.8.1-g5b484703b959 SMP preempt mod_unload ' should be '6.8.1 SMP preempt mod_unload '
```

决定的路径是:
scripts/setlocalversion =>
include/config/kernel.release =>
linux-6.2.12/include/generated/utsrelease.h

```txt
CONFIG_LOCALVERSION="v100"
# CONFIG_LOCALVERSION_AUTO is not set
```
这个配置得到的 version 为 `6.13.2v100+`


## 有一次被构建残留坑了
arm 自己构建 kernel 启动之后，会有这个错误:
```txt
[    1.095162] CPU: 11 UID: 0 PID: 157 Comm: modprobe Not tainted 6.13.1 #10
[    1.095887] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2012-3-4
[    1.096535] pstate: 80400005 (Nzcv daif +PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[    1.097232] pc : nfnetlink_net_init+0x38/0xd4 [nfnetlink]
[    1.097762] lr : nfnetlink_net_init+0x34/0xd4 [nfnetlink]
[    1.098343] sp : ffff80008527b950
[    1.098669] x29: ffff80008527b980 x28: ffff80008527bca8 x27: 000000000000000
1
[    1.099462] x26: ffff80007a6a9064 x25: 000000000000000c x24: ffff80007a6a906
c
[    1.100330] x23: ffff80008210b928 x22: 000000000000000c x21: ffff80008261098
0
[    1.101078] x20: 000000000000000c x19: ffff800082610980 x18: 000000000000000
0
[    1.101768] x17: 0000000000000000 x16: 0000000000000000 x15: 0000aaaada60f42
8
[    1.102572] x14: ffffffffffffffff x13: 64692d646c697562 x12: 2e756e672e65746
f
[    1.103329] x11: 000000000000ffff x10: 0000000000000000 x9 : 000000000000000
0
[    1.104085] x8 : ffff0000c131b0b0 x7 : 0000000000000000 x6 : ffff80008251118
0
[    1.104847] x5 : ffff0000c1994800 x4 : ffff0000ffff2360 x3 : 000000000000480
b
[    1.105538] x2 : ffff80007a6a6000 x1 : ffff0000c1994800 x0 : 000000000000000
0
[    1.106254] Call trace:
[    1.106503]  nfnetlink_net_init+0x38/0xd4 [nfnetlink] (P)
[    1.107055]  ops_init.constprop.0+0x88/0x170
[    1.107513]  register_pernet_operations+0xc4/0x120
[    1.108028]  register_pernet_subsys+0x38/0x5c
[    1.108457]  nfnetlink_init+0x84/0x1000 [nfnetlink]
[    1.108931]  do_one_initcall+0x80/0x1c8
[    1.109354]  do_init_module+0x60/0x218
[    1.109744]  load_module+0x19bc/0x1b5c
[    1.110120]  init_module_from_file+0x88/0xcc
[    1.110537]  __arm64_sys_finit_module+0x144/0x330
[    1.110995]  invoke_syscall+0x48/0x10c
[    1.111364]  el0_svc_common.constprop.0+0x40/0xe0
[    1.111822]  do_el0_svc+0x1c/0x28
[    1.112147]  el0_svc+0x38/0x148
[    1.112456]  el0t_64_sync_handler+0x10c/0x138
[    1.112883]  el0t_64_sync+0x198/0x19c
[    1.113244] Code: f90017e0 d2800000 956a4634 f9455e60 (f8745814)
[    1.113833] ---[ end trace 0000000000000000 ]---
[   10.850206] pci 0000:00:01.0: deferred probe pending: (reason unknown)
[   10.958849] pci 0000:00:02.0: deferred probe pending: (reason unknown)
[   10.974864] pci 0000:00:03.0: deferred probe pending: (reason unknown)
[   10.975515] pci 0000:00:04.0: deferred probe pending: (reason unknown)
[   10.976961] pci 0000:00:06.0: deferred probe pending: (reason unknown)
[   10.977584] pci 0000:00:07.0: deferred probe pending: (reason unknown)
[   10.978360] pci 0000:00:08.0: deferred probe pending: (reason unknown)
[   10.978963] pci 0000:00:0a.0: deferred probe pending: (reason unknown)
[   11.010728] pci 0000:00:0b.0: deferred probe pending: (reason unknown)
[   11.011377] pci 0000:00:0c.0: deferred probe pending: (reason unknown)
[   11.011972] pci 0000:00:0d.0: deferred probe pending: (reason unknown)
[   11.012559] pci 0000:00:09.0: deferred probe pending: (reason unknown)
[   22.133769] rcu: INFO: rcu_preempt detected stalls on CPUs/tasks:
[   22.135164] rcu:     11-...0: (11 ticks this GP) idle=f1d4/1/0x4000000000000
000 softirq=43/43 fqs=688
[   22.225111] rcu:     (detected by 6, t=21053 jiffies, g=-763, q=216 ncpus=12
)
[   22.225788] Sending NMI from CPU 6 to CPUs 11:
[   24.146792] watchdog: Watchdog detected hard LOCKUP on cpu 11
[   24.147895] Modules linked in: nfnetlink(+) ipv6
[   24.148398] Kernel panic - not syncing: Hard LOCKUP
[   24.246519] SMP: stopping secondary CPUs
[   29.997743] SMP: failed to stop secondary CPUs 0,6,11
[   30.022346] Kernel Offset: disabled
[   30.022683] CPU features: 0x000,00000000,18801240,82006203
[   30.023209] Memory Limit: none
[   30.023509] pstore: dump skipped in Panic path because of concurrent dump
[   30.024180] ---[ end Kernel panic - not syncing: Hard LOCKUP ]---
```
采用的配置为 :

如果去
```txt
gdb net/netfilter/nfnetlink.ko
```

```txt
$ l *nfnetlink_net_init+0x38
0x634 is in nfnetlink_net_init (./include/net/netns/generic.h:47).
```

```c
static inline void *net_generic(const struct net *net, unsigned int id)
{
	struct net_generic *ng;
	void *ptr;

	rcu_read_lock();
	ng = rcu_dereference(net->gen);
	ptr = ng->ptr[id];
	rcu_read_unlock();

	return ptr;
}
```

## 必须搞清楚到底为什么 kernel module 的规则

- 如果在gup.c 中添加了一个 print ，那么所有的 kernel module 需要重新构建吗?
- gup.c 中修改了一个函数 ?
- 现在似乎很多时候，kernel 没有启动的原因是这个吧，看看效果是什么？
CONFIG_MODULE_ALLOW_BTF_MISMATCH

## 似乎这里的 config 就是我们需要理解的

kernel/module/Kconfig 中的东西:
```txt
config MODULE_UNLOAD_TAINT_TRACKING
	bool "Tainted module unload tracking"
	depends on MODULE_UNLOAD
	select MODULE_DEBUGFS
	help
	  This option allows you to maintain a record of each unloaded
	  module that tainted the kernel. In addition to displaying a
	  list of linked (or loaded) modules e.g. on detection of a bad
	  page (see bad_page()), the aforementioned details are also
	  shown. If unsure, say N.

config MODVERSIONS
	bool "Module versioning support"
	depends on !COMPILE_TEST
	help
	  Usually, you have to use modules compiled with your kernel.
	  Saying Y here makes it sometimes possible to use modules
	  compiled for different kernels, by adding enough information
	  to the modules to (hopefully) spot any changes which would
	  make them incompatible with the kernel you are running.  If
	  unsure, say N.

config ASM_MODVERSIONS
	bool
	default HAVE_ASM_MODVERSIONS && MODVERSIONS
	help
	  This enables module versioning for exported symbols also from
	  assembly. This can be enabled only when the target architecture
	  supports it.

config MODULE_SRCVERSION_ALL
	bool "Source checksum for all modules"
	help
	  Modules which contain a MODULE_VERSION get an extra "srcversion"
	  field inserted into their modinfo section, which contains a
	  sum of the source files which made it.  This helps maintainers
	  see exactly which source was used to build a module (since
	  others sometimes change the module source without updating
	  the version).  With this option, such a "srcversion" field
	  will be created for all modules.  If unsure, say N.

config MODULE_SIG
	bool "Module signature verification"
	select MODULE_SIG_FORMAT
	help
	  Check modules for valid signatures upon load: the signature
	  is simply appended to the module. For more information see
	  <file:Documentation/admin-guide/module-signing.rst>.

	  Note that this option adds the OpenSSL development packages as a
	  kernel build dependency so that the signing tool can use its crypto
	  library.

	  You should enable this option if you wish to use either
	  CONFIG_SECURITY_LOCKDOWN_LSM or lockdown functionality imposed via
	  another LSM - otherwise unsigned modules will be loadable regardless
	  of the lockdown policy.

	  !!!WARNING!!!  If you enable this option, you MUST make sure that the
	  module DOES NOT get stripped after being signed.  This includes the
	  debuginfo strip done by some packagers (such as rpmbuild) and
	  inclusion into an initramfs that wants the module size reduced.
```

### 这个也是看看的
https://docs.kernel.org/kbuild/modules.html#module-versioning

## module.c 中基本的内容
1. mod_tree_insert 各种 mod_tree 的操作
2. mod_sysfs_init sysfs 相关操作
3. lookup_module_symbol_attrs symbol 相关的操作
   > 总体来说更像是对于二进制文件的操作

> 本来以为 module 和 device 放在一起，其实整个关注的内容完全的不同，放在文件夹的位置也是完全不同的。


## 这个选项 CONFIG_MODVERSIONS 需要看看

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
