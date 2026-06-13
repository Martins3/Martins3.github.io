# crash 内核实现
## 仔细看看这个文档吧
- Documentation/admin-guide/kdump/

- https://crash-utility.github.io/crash.changelog.html
- https://linux.die.net/man/5/kdump.conf
- https://www.dedoimedo.com/computers/crash-analyze.html#mozTocId186712

- https://mp.weixin.qq.com/s/mCnBkhw97vCgx45ku4V54w
	- What’s Inside a Linux Kernel Core Dump

## 内核实现
- https://mp.weixin.qq.com/s/yHHYjxZjF7nc7urbilGzSA
- [知乎专栏: 5 分钟搞懂 kexec 工作原理](https://zhuanlan.zhihu.com/p/105284305)

- 将 kernel 和 initrd 放到内核中*连续的*内存中，然后让内核直接从该位置启动。
- initrd 的位置在 `boot_params`

kernel 镜像有指定的入口地址，kernel 镜像要加载到入口地址位置才能正常启动，而这块内存正在被当前内核使用，所以 kernel 镜像需要临时存放的内存，在跳转前要将内容搬移到入口地址。
> - [ ] 似乎 kernel 的入口地址是物理地址，而且是固定的

> 之前分配的所有的物理内存直接踩掉就可以了。

```sh
kexec -s uImage --ramdisk=./ramdisk.bin
```
为此，kexec 引入了 entry 的概念，其实就是一个 unsigned long 对象，记录申请到的 page 的物理地址，并利用低位的 4bit 来对 entry 进行分类，共计有如下 4 类 entry。
- `IND_DESTINATION`，用于记录 segment 要搬移到的目标地址
- `IND_SOURCE` ：用于记录 segment 内容所在的物理地址
- `IND_INDIRECTION`
- `IND_DONE`

当 kexec -l 的时候
![](https://pic1.zhimg.com/80/v2-8329bc493b0018c9a720181bc13d51e4_1440w.jpg)

而当 kexec -e 的时候，
![](https://pic1.zhimg.com/80/v2-a5199ce8d58d112ea047c65bb8e762d0_1440w.jpg)

[知乎专栏：linux 系统内存 dump 机制介绍（一）--kdump](https://zhuanlan.zhihu.com/p/25008148)
- 所以，是不是需要两个内核来实现？

[知乎专栏：很详细，也很混乱](https://zhuanlan.zhihu.com/p/522898356)


- [ ] https://dumphex.github.io/2020/02/15/kdump/
- [ ] https://wiki.archlinux.org/title/Kdump

## kdump
when linux crashed, kexec and kdump will dump kernel memory to vmcore
- https://github.com/crash-utility/crash
- drgn : 具有明显的速度优势

`PG_reserved` is set for special pages. The "struct page" of such a page
should in general not be touched (e.g. set dirty) except by its owner.
Pages marked as `PG_reserved` include:
- Pages allocated in the context of kexec/kdump (loaded kernel image, control pages, vmcoreinfo)

## kdump/kdump.txt
- [ ] 是可以重新执行出现错误的内核吗?

On x86 machines, the first 640 KB of physical memory is needed to boot,
regardless of where the kernel loads. Therefore, kexec backs up this
region just before rebooting into the dump-capture kernel.

All of the necessary information about the system kernel's core image is
encoded in the ELF format, and stored in a reserved area of memory
before a crash. The physical address of the start of the ELF header is
passed to the dump-capture kernel through the elfcorehdr= boot
parameter. Optionally the size of the ELF header can also be passed
when using the elfcorehdr=[size[KMG]@]offset[KMG] syntax.

With the dump-capture kernel, you can access the memory image through
/proc/vmcore. This exports the dump as an ELF-format file that you can
write out using file copy commands such as cp or scp. Further, you can
use analysis tools such as the GNU Debugger (GDB) and the Crash tool to
debug the dump file. This method ensures that the dump pages are correctly
ordered.

There are two possible methods of using Kdump.

1) Build a separate custom dump-capture kernel for capturing the
   kernel core dump.

2) Or use the system kernel binary itself as dump-capture kernel and there is
   no need to build a separate dump-capture kernel. This is possible
   only with the architectures which support a relocatable kernel. As
   of today, i386, x86_64, ppc64, ia64, arm and arm64 architectures support
   relocatable kernel.

- [ ] 什么叫做 relocatable kernel

## man kexec(2)

> It's also possible to invoke kexec without an option parameter. In that case, kexec loads the specified kernel and then invokes shutdown(8).  If the shutdown scripts of your Linux distribu‐
tion support kexec-based rebooting, they then call kexec -e just before actually rebooting the machine. That way, the machine does a clean shutdown including all shutdown scripts.

## kexec.c
- `kexec_load`
    - `kexec_load_check`
    - `do_kexec_load`
        - `kimage_alloc_init`
            - `do_kimage_alloc_init`
            - `copy_user_segment_list`
        - `kimage_load_segment`
        - `kimage_terminate`

## `kexec_core.c`

- 各种 kimage 管理

以及一些 crash 开头的函数。

- `kernel_kexec`
    - `kernel_restart_prepare` ：应该是将各种设备全部关闭的。
    - `migrate_to_reboot_cpu`
    - `cpu_hotplug_enable`
    - `machine_shutdown` ：将 secondary CPU 全部关闭。
    - `machine_kexec`

- `crash_save_cpu`
- `crash_kexec`

- `kernel_kexec` ：居然唯一调用者是 reboot 系统调用。

## `kexec_file.c`
https://man7.org/linux/man-pages/man2/kexec_load.2.html

这是 x86 特有的系统调用:
```txt
config KEXEC_FILE
    bool "kexec file based system call"
    select KEXEC_CORE
    select BUILD_BIN2C
    depends on X86_64
    depends on CRYPTO=y
    depends on CRYPTO_SHA256=y
    ---help---
      This is new version of kexec system call. This system call is
      file based and takes file descriptors as system call argument
      for kernel and initramfs as opposed to list of segments as
      accepted by previous system call.
```

## 分析触发 core dump 之后的结果

- `write_sysrq_trigger`
    - `__handle_sysrq`
        - `sysrq_handle_crash` ：使用访问不存在的地址来实现

- `die_kernel_fault`
    - `die`
        - `kexec_should_crash`

## 分析 x86 kdump 的流程
- `__crash_kexec`

### [ ] `kexec_crash_image` 是如何组装的

### 似乎 is panic 是一个很关键的参数

## acpi 是如何探测的

```txt
[    0.000000] BIOS-e820: [mem 0x0000000000000000-0x0000000000000fff] reserved
[    0.000000] BIOS-e820: [mem 0x0000000000001000-0x000000000009fbff] usable
[    0.000000] BIOS-e820: [mem 0x000000000009fc00-0x000000000009ffff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000000f0000-0x00000000000fffff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000b3000000-0x00000000bef5cfff] usable
[    0.000000] BIOS-e820: [mem 0x00000000befffc00-0x00000000beffffff] usable
[    0.000000] BIOS-e820: [mem 0x00000000bffe0000-0x00000000bfffffff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000feffc000-0x00000000feffffff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000fffc0000-0x00000000ffffffff] reserved
```

0x0000000000001000-0x000000000009fbff + 0x00000000b3000000-0x00000000bef5cfff + 0x00000000befffc00-0x00000000beffffff

计算了一下，正好是:
```plain
Out[5]: 191.98437213897705
```

- `e820__memory_setup_default` ：这这里解析参数，是内核参数提供的
- `e820__memory_setup` : 在这里打印出来

----> 这个 zhihu 参考有问题啊
- `kexec_file_load`
  - `kimage_file_alloc_init`
    - `kimage_file_prepare_segments`
      - `arch_kexec_kernel_image_load`
        - `image->fops->load`
          - `kexec_bzImage64_ops.load`
            - `bzImage64_load`
              - `setup_cmdline`



- `do_kexec_load`
  - `kimage_load_segment`
    - `kimage_load_crash_segment`

- `__crash_kexec` ：是在这里进行切换的
  - `machine_kexec`
    - `relocate_kernel` ：参数

- `do_kexec_load` ：系统调用的入口的
  - `kimage_alloc_init` :

## kexec 的常规路径是什么

kexec-tools 中间:
```c
reboot(LINUX_REBOOT_CMD_KEXEC);
```

进入到内核中间：
- `kernel_kexec`
  - `machine_shutdown`
  - `machine_kexec`

## crash 源码分析
主要是解析各种格式:
- lkcd_v8.c
- diskdump.n



## 和 kdump 关联的几个文件
<!-- ab8e968f-3f68-4231-8676-89a59f935ad9 -->
- /proc/kcore
	- https://man7.org/linux/man-pages/man5/proc_kcore.5.html : 将内核用 elf 格式呈现
	- 用于 crash 实时分析
- /dev/kmem
	- 已经被移除掉 commit bbcd53c96071 ("drivers/char: remove /dev/kmem for good")
- /dev/mem
	- https://man7.org/linux/man-pages/man4/mem.4.html
	- 直接访问物理内存
- /proc/vmcore
	- 在 crash 的时候使用
	- https://unix.stackexchange.com/questions/41455/how-can-i-generate-proc-vmcore
	- https://stackoverflow.com/questions/34626466/what-are-the-contents-of-proc-vmcore

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
