## kallsyms_lookup_name
- [Unexporting kallsyms_lookup_name()](https://lwn.net/Articles/813350/)

通过 kallsyms_lookup_name() and kallsyms_on_each_symbol() 几乎是可以实现为所欲为效果。

现在 kallsyms_lookup_name 只是被 trace 系统使用的。

## KALLSYMS
```txt
config KALLSYMS
	bool "Load all symbols for debugging/ksymoops" if EXPERT
	default y
	help
	  Say Y here to let the kernel print out symbolic crash information and
	  symbolic stack backtraces. This increases the size of the kernel
	  somewhat, as all symbols have to be loaded into the kernel image.

config KALLSYMS_SELFTEST
	bool "Test the basic functions and performance of kallsyms"
	depends on KALLSYMS
	default n
	help
	  Test the basic functions and performance of some interfaces, such as
	  kallsyms_lookup_name. It also calculates the compression rate of the
	  kallsyms compression algorithm for the current symbol set.

	  Start self-test automatically after system startup. Suggest executing
	  "dmesg | grep kallsyms_selftest" to collect test results. "finish" is
	  displayed in the last line, indicating that the test is complete.

config KALLSYMS_ALL
	bool "Include all symbols in kallsyms"
	depends on DEBUG_KERNEL && KALLSYMS
	help
	  Normally kallsyms only contains the symbols of functions for nicer
	  OOPS messages and backtraces (i.e., symbols from the text and inittext
	  sections). This is sufficient for most cases. And only if you want to
	  enable kernel live patching, or other less common use cases (e.g.,
	  when a debugger is used) all symbols are required (i.e., names of
	  variables from the data sections, etc).

	  This option makes sure that all symbols are loaded into the kernel
	  image (i.e., symbols from all sections) in cost of increased kernel
	  size (depending on the kernel configuration, it may be 300KiB or
	  something like this).

	  Say N unless you really need all symbols, or kernel live patching.

config KALLSYMS_ABSOLUTE_PERCPU
	bool
	depends on KALLSYMS
	default X86_64 && SMP
```

## kallsyms.c
源码很少，应该是直接遍历 elf 的符号表

其中的 CONFIG_BPF_SYSCALL 选项也包含了一些内容

### [ ] 所以 /proc/kallsyms 里面的内容是如何获取的
暂时没兴趣了

## 问题

### 解析其中的内容
```txt
ffffffffc004553c t bpf_prog_81dfab3bd7eb416a    [bpf]
ffffffffc00456a8 t bpf_prog_ab4bc4523b7fe6b4    [bpf]
ffffffffc000d118 t bpf_prog_3918c82a5f4c0360    [bpf]
```

`T` 是表示什么类型 ?
为什么`System.map`　开始的部分有大量的地址似乎是实地址`0000000000000000 D __per_cpu_start`

### 对比一下关闭和打开 kallsyms 的时候，系统中的 System.map 和 /proc/kallsyms 内容还是一样的吗?

## System.map 和 CONFIG_KALLSYMS 的关系是
<!-- e9c3686c-16ad-42b8-8903-cbc7924e6bcf -->

- https://kernelnewbies.org/FAQ/System.map
- https://linux.die.net/man/8/klogd

- https://rlworkman.net/system.map/
  - 你相信吗? klogd 来做地址转换 ，还需要读取用户态的中的文件
  - https://linux.die.net/man/8/sysklogd

- https://stackoverflow.com/questions/70576329/how-to-understand-system-map-output-from-linux-kernel-build-how-i-calculate-phy : 回答这个问题
- https://stackoverflow.com/questions/28936630/what-is-the-need-of-having-both-system-map-file-and-proc-kallsyms
  - 我更加认同 firo 的回答

System.map 中特有的
```txt
0000000000000000 : _kernel_flags_le_hi32
0000000000000000 : _kernel_size_le_hi32
0000000000000200 : PECOFF_FILE_ALIGNMENT
00000000002e2200 : __pecoff_data_rawsize
000000000000000a : _kernel_flags_le_lo32
00000000003b0000 : __pecoff_data_size
00000000013e0000 : _kernel_size_le_lo32
```

/proc/kallsyms 多出来的都是当时系统加载的模块的符号地址:
```txt
[__builtin__kprobes] # 但是这个是不懂
[vhost_iotlb]
[vhost_net]
[x_tables]
[xfs]
[xt_MASQUERADE]
[xt_addrtype]
[xt_conntrack]
```

当去掉这些模块内容之后，/proc/kallsyms 没有其他的内容了。

当 CONFIG_RANDOMIZE_BASE=y 的时候，比较结果:
```txt
🧀  sudo cat /proc/kallsyms | grep br_ip6_fragment
ffffffff906570f0 T __pfx_br_ip6_fragment
ffffffff90657100 T br_ip6_fragment
ffffffff90b7edbc r __ksymtab_br_ip6_fragment

🧀  sudo cat /boot/System.map | grep br_ip6_fragment
ffffffff81a570f0 T __pfx_br_ip6_fragment
ffffffff81a57100 T br_ip6_fragment
ffffffff81f7edbc r __ksymtab_br_ip6_fragment
```

给内核添加上 nokaslr ，然后重启，结果为:
```txt
🧀  sudo cat /proc/kallsyms | grep br_ip6_fragment
ffffffff81a570f0 T __pfx_br_ip6_fragment
ffffffff81a57100 T br_ip6_fragment
ffffffff81f7edbc r __ksymtab_br_ip6_fragment
```


## 为什么我的内核启动有警告

```txt
[ 0.001000] Unknown kernel command line parameters "nokaslr", will be passed to user space.
```

而且显示的确把这个转递给 systemd 了：
```txt
[    0.705640] Run /init as init process
[    0.705735]   with arguments:
[    0.705812]     /init
[    0.705860]     nokaslr
[    0.705911]   with environment:
[    0.705989]     HOME=/
[    0.706037]     TERM=linux
[    0.706158]     selinux=0
```
有趣，原来是这样的:
https://lore.kernel.org/all/IA2PR20MB72069B2764125FF8A1927940FD1D2@IA2PR20MB7206.namprd20.prod.outlook.com/

> Reason: On x86, this parameter is parsed by the early loader, and so the
> main kernel itself doesn't do anything with it.

## [ ] 那么 debuginfo 中到底带什么东西 ?

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
