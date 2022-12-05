# kdump，kexec 和 crash-utility

## container-of
https://crash-utility.github.io/crash.changelog.html

## 基本使用

- foreach bt : 所有进程的 backtrace
- bt -a : 所有的 CPU 的 backtrace
- bt -FF  264 : CPU
  - [ ] -FF 的数据，好吧，需要重新理解 kmalloc 和 stack 的关系
- search sd_fops : 搜索 sd_fops，我靠，根本不能理解为什么这个东西的实现原理啊
- dev : 展示所有的 device
- kmem
  - `-s` : 展示 k

## 常见问题
### struct: invalid data structure reference: r5conf
https://stackoverflow.com/questions/58810201/how-to-find-a-symbol-file-and-tell-crash-about-it

需要加载对应的模块

## 基本使用
- centos 8
  - https://unixcop.com/install-and-configure-kernel-crash-dump-on-centos-8/
- ubuntu
- Arch

## kexec and kdump
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
- [ ] 分析本质 https://wiki.archlinux.org/title/Kdump

## kdump 并不是开箱即用的
- https://linux.die.net/man/5/kdump.conf
    - https://www.thegeekdiary.com/understanding-kdump-configuration-file-etc-kdump-conf/
- https://www.thegeekstuff.com/2014/05/kdump/

- crash 的使用方法
  - https://www.ebpf.top/post/ubuntu_kdump_crash/

- https://unix.stackexchange.com/questions/671800/unable-to-get-kernel-crash-dump-on-kernel-panic

应该是自动保存到 vmcore 中哇:
- [ ] linux-crashdump
  - 这个工具似乎来源于 debian 的： https://www.cyberciti.biz/faq/how-to-on-enable-kernel-crash-dump-on-debian-linux/

## 支持 kernel dump 的功能
否则找不到 /proc/vmcore

```txt
If enabled, NixOS will set up a kernel that will boot on crash, and leave the user in systemd rescue to be able to save the crashed kernel dump at /proc/vmcore. It also activates the NMI watchdog.
```

参考:
- https://github.com/NixOS/nixpkgs/blob/master/nixos/modules/misc/crashdump.nix
- https://search.nixos.org/options?channel=22.05&show=boot.crashDump.enable&from=0&size=50&sort=relevance&type=packages&query=boot.crashDump.enable

使用这个方法可以检查到 CONFIG 的确修改过:
https://superuser.com/questions/287371/obtain-kernel-config-from-currently-running-linux-system

```txt
➜  .dotfiles git:(backup) zgrep CONFIG_CRASH /proc/config.gz
CONFIG_CRASH_DUMP=y
CONFIG_CRASH_CORE=y
```

crashdump.nix 中，postCommand 是给

- [ ] 无法理解，在 nixos 的启动中，我检查到了这个
```txt
[    2.473781] stage-2-init: running activation script...
[    2.843469] stage-2-init: setting up /etc...
[    3.453159] stage-2-init: loading crashdump kernel...
```

```sh
echo 1 > /proc/sys/kernel/sysrq
echo c > /proc/sysrq-trigger
```


## crash utility 的官方文档可以自习看看
- https://crash-utility.github.io/crash_whitepaper.html

## kdump
when linux crashed, kexec and kdump will dump kernel memory to vmcore
- https://github.com/crash-utility/crash
- drgn : 具有明显的速度优势

`PG_reserved` is set for special pages. The "struct page" of such a page
should in general not be touched (e.g. set dirty) except by its owner.
Pages marked as `PG_reserved` include:
- Pages allocated in the context of kexec/kdump (loaded kernel image, control pages, vmcoreinfo)


## [ ] 既然存在那么多个的 CPU，为什么 syslog 上只有一个 cpu 的 backtrace 的哇

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

## 简单看看代码

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

## 相关资源
- https://github.com/kexecboot/kexecboot
    - Kexecboot is a nice Linux-As-a-Bootloader implementation based on kexec.


## 分析触发 core dump 之后的结果

但是可以强制关机来维持生活:

```c

static const struct file_operations proc_sysrq_trigger_operations = {
    .write      = write_sysrq_trigger,
    .llseek     = noop_llseek,
};

static void sysrq_init_procfs(void)
{
    if (!proc_create("sysrq-trigger", S_IWUSR, NULL,
             &proc_sysrq_trigger_operations))
        pr_err("Failed to register proc interface\n");
}
```

我操，实际上，sysrq 的功能好多哇:
- `write_sysrq_trigger`
    - `__handle_sysrq`
        - `sysrq_handle_crash` ：使用访问不存在的地址来实现

- `die_kernel_fault`
    - `die`
        - `kexec_should_crash`

## 到底什么是 vmcore
- https://stackoverflow.com/questions/34626466/what-are-the-contents-of-proc-vmcore :star: 简洁清晰
- https://unix.stackexchange.com/questions/41455/how-can-i-generate-proc-vmcore
    - 让我知道了，nixos 默认是没有打开的，所以也看不到 /proc/vmcore 的哇

## 按照这个操作一下
- https://www.thegeekdiary.com/understanding-kdump-configuration-file-etc-kdump-conf/

## Ubuntu 上的操作相当简单
https://ubuntu.com/server/docs/kernel-crash-dump

```txt
sudo apt install linux-crashdump
```
安装的过程中，两个 yes 就可以了，然后重启，通过工具可以看到如何重启的。

## [ ] 无法理解内核的运行的用户态工具的关系

kexec -p [/boot/vmlinuz-linux-kdump] --initrd=[/boot/initramfs-linux-kdump.img] --append="root=[root-device] single irqpoll maxcpus=1 reset_devices"
sudo kexec -p /run/current-system/kernel \
        --initrd=/run/current-system/initrd \
        --append="init=$(readlink -f /run/current-system/init) irqpoll maxcpus=1 reset_devices"

参考这里
- https://gist.github.com/Mic92/4fdf9a55131a7452f97003f445294f97

```sh
sudo kexec --load /run/current-system/kernel \
        --initrd=/run/current-system/initrd \
        --append="init=$(readlink -f /run/current-system/init) irqpoll maxcpus=1 reset_devices"
systemctl kexec
```

重启之后:
```sh
➜  ~ cat /proc/cmdline
init=/nix/store/bqlpq2n4c49xykrhpd75b2wr2p8pswnk-nixos-system-nixos-22.05.1437.e8d47977286/init irqpoll maxcpus=1 reset_devices
```
再次重启，回复正常:
```sh
➜  ~ cat /proc/cmdline
BOOT_IMAGE=(hd0,msdos1)/nix/store/9czbjwwky8vrw8f69v5hjb41bdyhzrp4-linux-5.15.50/bzImage init=/nix/store/bqlpq2n4c49xykrhpd75b2wr2p8pswnk-nixos-system-nixos-22.05.1437.e8d47977286/init loglevel=4 crashkernel=128M nmi_watchdog=panic softlockup_panic=1
```

虽然这个的确是展示了一个重启，但是还是测试有问题:

上面 --load 和 -p 没有区别的，都可以，只是 arch 中说的，去掉 -p 直接切换，这个操作目前不行的哇:

```sh
sudo kexec -p /run/current-system/kernel \
        --initrd=/run/current-system/initrd \
        --append="BOOT_IMAGE=(hd0,msdos1)/nix/store/9czbjwwky8vrw8f69v5hjb41bdyhzrp4-linux-5.15.50/bzImage init=/nix/store/bqlpq2n4c49xykrhpd75b2wr2p8pswnk-nixos-system-nixos-22.05.1437.e8d47977286/init loglevel=4 crashkernel=128M nmi_watchdog=panic softlockup_panic=1 maxcpus=1"
```
仔细想想，这并不是解决办法

继续使用:
```sh
sudo kexec -p /run/current-system/kernel \
        --initrd=/run/current-system/initrd \
        --append="init=$(readlink -f /run/current-system/init) irqpoll maxcpus=1 reset_devices"

echo 1 > /proc/sys/kernel/sysrq
echo c > /proc/sysrq-trigger
```

## crashkernel=auto
- https://www.mail-archive.com/kexec@lists.infradead.org/msg25696.html
  - 没有什么规律


## 补充一下

在 centos 8 中，使用 alpine.sh 会导致其 kdump.service 无法正确启动。
```txt
kdump: Failed to detect kdump kernel location
kdump: failed to prepare for kdump bootinfo.
kdump: Starting kdump: [FAILED]
```


# 分析 x86 kdump 的流程
- `__crash_kexec`

## [ ] `kexec_crash_image` 是如何组装的

## 似乎 is panic 是一个很关键的参数

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


- [ ] kexec -p 和 kexec -l 的关系是什么?
  - 是不是，分别执行的参数吗?

## kexec 的常规路径是什么

kexec-tools 中间:
```c
reboot(LINUX_REBOOT_CMD_KEXEC);
```

进入到内核中间：
- `kernel_kexec`
  - `machine_shutdown`
  - `machine_kexec`

## crash 和 exec 似乎使用的 kimage 都是不同的

```c
struct kimage *kexec_image;
struct kimage *kexec_crash_image;
```

https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/kernel_administration_guide/kernel_crash_dump_guide#sect-kdump-memory-requirements

## 基本的使用
- bt -FF : 为什么 stack 上和 kobject 有关

## ubuntu 上的基本使用方法
- apt install crash

- apt-get install linux-image-$(uname -r)-dbgsym

- 安装的位置在 /usr/lib/debug/boot 中:

crash /usr/lib/debug/boot/vmlinux-5.4.0-28-generic 的时候，可以直接在 kernel 中调试:

编辑一下:
vim /boot/grub/grub.cfg


将 [/boot/vmlinuz 转换为 vmlinux](https://superuser.com/questions/298826/how-do-i-uncompress-vmlinuz-to-vmlinux) 是不行的，里面没有调试信息

## 补充的操作
- virsh dump --memory-only --live <vm uuid>  g.dump

然后 crash vmlinux g.dump

```sh
rpm2cpio xxx.rpm | cpio -idm
find . -name "vmlinux"
echo  crash  g.dump  `!!` >> crash.sh
chmod a+x crash.sh
```

- foreach bt : 所有进程的 backtrace
- bt -a : 所有的 CPU 的 backtrace
- bt -FF  264 : CPU
  - [ ] -FF 的数据，好吧，需要重新理解 kmalloc 和 stack 的关系
- search sd_fops : 搜索 sd_fops，我靠，根本不能理解为什么这个东西的实现原理啊
- dev : 展示所有的 device
- kmem
  - `-s` : 展示 k

- bt -FF -c 12

## 如何 ubuntu crash 之后
- 获取 debuginfo
- https://askubuntu.com/questions/197016/how-to-install-a-package-that-contains-ubuntu-kernel-debug-symbols
- https://superuser.com/questions/62575/where-is-vmlinux-on-my-ubuntu-installation

echo "deb http://ddebs.ubuntu.com $(lsb_release -cs)-updates main restricted universe multiverse
deb http://ddebs.ubuntu.com $(lsb_release -cs)-proposed main restricted universe multiverse" | \
sudo tee -a /etc/apt/sources.list.d/ddebs.list
