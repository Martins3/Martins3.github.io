# QEMU 的参数解析

大家第一次使用 QEMU 的时候必然被 QEMU 的参数搞的很难受，这是我常用的一个[脚本](https://github.com/Martins3/Martins3.github.io/blob/master/docs/qemu/sh/alpine.sh) 的参数
```sh
qemu-system-x86_64 \
-drive file=/home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2,format=qcow2 \
-m 6G \
-smp 1,maxcpus=3 \
-kernel /home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage \
-append root=/dev/sda3 nokaslr \
-chardev file,path=/tmp/seabios.log,id=seabios \
-device isa-debugcon,iobase=0x402,chardev=seabios \
-bios /home/maritns3/core/seabios/out/bios.bin \
-device nvme,drive=nvme1,serial=foo \
-drive file=/home/maritns3/core/vn/hack/qemu/x64-e1000/img1.ext4,format=raw,if=none,id=nvme1 \
-device virtio-blk-pci,drive=nvme2,iothread=io0 \
-drive file=/home/maritns3/core/vn/hack/qemu/x64-e1000/img2.ext4,format=raw,if=none,id=nvme2 \
-object iothread,id=io0 \
-virtfs local,path=/home/maritns3/core/vn/hack/qemu/x64-e1000/share,mount_tag=host0,security_model=mapped,id=host0 \
-accel tcg,thread=single \
-monitor stdio \
-qmp unix:/home/maritns3/core/vn/hack/qemu/x64-e1000/test.socket,server,nowait \
```
随随便便几十个参数 :( :(

分析细节之前非常推荐读一下 [LWN 的一篇文章](https://lwn.net/Articles/872321/)，大致讲解 QEMU 核心 maintainer Bonzini 在 KVM forum 2021 上做的一篇报告。
有[中文翻译](https://mp.weixin.qq.com/s/xLIXBifypRUJDmSnL7AOEA)可以快速浏览。这篇文章使用 QEMU 的参数解析作为例子，分析了那些不必要的复杂性来源，以及避免的策略。

下面只是粗浅的分析。

## core code flow
这是 main 函数中的巨大的 for 循环，使用 lookup_opt 从左向右对于
参数扫描，每次匹配到一个完整的参数之后，就会返回 QEMUOption 和 optarg
```c
    for(;;) {
        if (optind >= argc)
            break;
        if (argv[optind][0] != '-') {
            loc_set_cmdline(argv, optind, 1);
            drive_add(IF_DEFAULT, 0, argv[optind++], HD_OPTS);
        } else {
            const QEMUOption *popt;

            // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
            printf("[%s] : [%s]\n", popt->name,  optarg);
            // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
            popt = lookup_opt(argc, argv, &optarg, &optind);
            if (!(popt->arch_mask & arch_type)) {
                error_report("Option not supported for this target");
                exit(1);
            }
            switch(popt->index) {
            case QEMU_OPTION_cpu:
```

使用上面的调试语句可以获取下面的输出
```txt
[drive] : [file=/home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2,format=qcow2]
[m] : [6G]
[smp] : [1,maxcpus=3]
[kernel] : [/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage]
[append] : [root=/dev/sda3 nokaslr ]
[chardev] : [file,path=/tmp/seabios.log,id=seabios]
[device] : [isa-debugcon,iobase=0x402,chardev=seabios]
[bios] : [/home/maritns3/core/seabios/out/bios.bin]
[device] : [nvme,drive=nvme1,serial=foo]
[drive] : [file=/home/maritns3/core/vn/hack/qemu/x64-e1000/img1.ext4,format=raw,if=none,id=nvme1]
[device] : [virtio-blk-pci,drive=nvme2,iothread=io0]
[drive] : [file=/home/maritns3/core/vn/hack/qemu/x64-e1000/img2.ext4,format=raw,if=none,id=nvme2]
[object] : [iothread,id=io0]
[virtfs] : [local,path=/home/maritns3/core/vn/hack/qemu/x64-e1000/share,mount_tag=host0,security_model=mapped,id=host0]
[accel] : [tcg,thread=single]
[monitor] : [stdio]
[qmp] : [unix:/home/maritns3/core/vn/hack/qemu/x64-e1000/test.socket,server,nowait]
```

### QEMUOption 组织结构
现在根据 `popt->index` 可以跳转到一个具体的处理操作上.

例如 -machine 会跳转到:
```c
            case QEMU_OPTION_kernel:
                qemu_opts_set(qemu_find_opts("machine"), 0, "kernel", optarg,
                              &error_abort);
                break;
```
解析了这些参数之后，需要将参数的结果保存起来，等到需要使用在查询。

QEMU 使用 QemuOptsList QemuOpts 和 QemuOpt 三级结构来保存
- QemuOptsList 可以持有多个 QemuOpts
- QemuOpts 可以持有多个 QemuOpt


为什么需要三层结构可以从下面两个参数理解:
```plain
[drive] : [file=/home/maritns3/core/vn/hack/qemu/x64-e1000/img1.ext4,format=raw,if=none,id=nvme1]
[drive] : [file=/home/maritns3/core/vn/hack/qemu/x64-e1000/img2.ext4,format=raw,if=none,id=nvme2]
```

1. 一个 `-drive` 的参数会创建出来一个 QemuOpts
2. `-drive` 后面跟着的 file=... format=... if=... 和 id=... 都会创建出来一个 QemuOpt 挂到 QemuOpts 上
3. `-drive` 对应的 QemuOpts 会挂载到一个 QemuOptsList 上，也就是 `qemu_drive_opts` 上的。
```c
QemuOptsList qemu_drive_opts = {
    .name = "drive",
    .head = QTAILQ_HEAD_INITIALIZER(qemu_drive_opts.head),
    .desc = {
        /*
         * no elements => accept any params
         * validation will happen later
         */
        { /* end of list */ }
    },
};
```
4. 这些 QemuOptsList 通过调用 qemu_add_opts  保存到数组 vm_config_groups 中间，通过 qemu_find_opts 使用字符串查询到 QemuOptsList

最后就可以解析的大致流程了:
- qemu_opts_parse_noisily : 在 [core code flow](#core-code-flow) 中遇到一个 -foo bar 之类就匹配一个
  - opts_parse
    - opts_parse_id : 当参数为类似 -device nvme,drive=nvme1,serial=foo -drive file=${ext4_img1},format=raw,if=none,id=nvme1 的时候，获取到 id=nvme1
    - qemu_opts_create : 创建 QemuOpts 并且将其插入到 QemuOptsList 上
    - opts_do_parse : 用于解析出来一个一个的 QemuOpt 插入到 QemuOptsList 上
      - get_opt_name_value : 将一个参数拆分开来, 比如 2,maxcpus=3 就可以拆分为两个
      - opt_create : 将解析出来的参数划分为使用 QemuOpt 包装，并且插入到 QemuOptsList 上
  - qemu_opts_print_help : 解析出现错误，那么就报错


## qemu-options.def
在 qemu-options.def 中会定义每一个选项的基本信息
```c
DEF("cpu", HAS_ARG, QEMU_OPTION_cpu,
    "-cpu cpu        select CPU ('-cpu help' for list)\n", QEMU_ARCH_ALL)
```

qemu-options.def 会在三个位置 include，因为每次 include 前面 macro 的定义不同，而解析出来不同的内容

### qemu_options
```c
static const QEMUOption qemu_options[] = {
    { "h", 0, QEMU_OPTION_h, QEMU_ARCH_ALL },

#define DEF(option, opt_arg, opt_enum, opt_help, arch_mask)     \
    { option, opt_arg, opt_enum, arch_mask },
#define DEFHEADING(text)
#define ARCHHEADING(text, arch_mask)

#include "qemu-options.def"
    { NULL },
};
```
在 lookup_opt 中，查询 qemu_options 来将参数做拆分并返回 QEMUOption

```c
typedef struct QEMUOption {
    const char *name;
    int flags;
    int index;
    uint32_t arch_mask;
} QEMUOption;
```

### opt_enum
将每一个 drive 中的 opt_enum 找出来，从而
```c
enum {

#define DEF(option, opt_arg, opt_enum, opt_help, arch_mask)     \
    opt_enum,
#define DEFHEADING(text)
#define ARCHHEADING(text, arch_mask)

#include "qemu-options.def"
};
```
这是 [core code flow](#core-code-flow) 中根据 QEMUOption::index 来做 switch case.

将会生成如下的 enum
```c
enum {
  // ....
  QEMU_OPTION_drive,
};
```
其使用位置为

### help info
```c
static void help(int exitcode)
{
    version();
    printf("usage: %s [options] [disk_image]\n\n"
           "'disk_image' is a raw hard disk image for IDE hard disk 0\n\n",
            error_get_progname());

#define DEF(option, opt_arg, opt_enum, opt_help, arch_mask)    \
    if ((arch_mask) & arch_type)                               \
        fputs(opt_help, stdout);

#define ARCHHEADING(text, arch_mask) \
    if ((arch_mask) & arch_type)    \
        puts(stringify(text));

#define DEFHEADING(text) ARCHHEADING(text, QEMU_ARCH_ALL)

#include "qemu-options.def"

    printf("\nDuring emulation, the following keys are useful:\n"
           "ctrl-alt-f      toggle full screen\n"
           "ctrl-alt-n      switch to virtual console 'n'\n"
           "ctrl-alt        toggle mouse and keyboard grab\n"
           "\n"
           "When using -nographic, press 'ctrl-a h' to get some help.\n"
           "\n"
           QEMU_HELP_BOTTOM "\n");

    exit(exitcode);
}
```

## extra notes
- QemuOptsList::merge_lists : `-smp 2,maxcpus=3` 也可以写为 `-smp 2 -smp maxcpus=3`
- QEMU 参数复杂度而产生的项目 quickemu / utm
- QEMU 作者的另一个的工具 ffmpeg 的参数也非常的复杂，以至于有这个网站 https://evanhahn.github.io/ffmpeg-buddy/

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
