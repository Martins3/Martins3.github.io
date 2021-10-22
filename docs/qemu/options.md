# QEMU 的参数解析
大家第一次使用 QEMU 的时候必然被 QEMU 的参数搞的很难受，这是我常用的一个[脚本](https://github.com/Martins3/Martins3.github.io/blob/0aaf533f7a048a621d6a45657fbd42aa34cda45a/hack/qemu/x64-e1000/alpine.sh#L126)，
随随便便几十个参数。

分析细节之前非常推荐读一下 [LWN 的一篇文章](https://lwn.net/Articles/872321/)，大致讲解 QEMU 核心 maintainer Bonzini 在 KVM forum 2021 上做的一篇报告。
有[中文翻译](https://mp.weixin.qq.com/s/xLIXBifypRUJDmSnL7AOEA)可以快速浏览。这篇文章使用 QEMU 的参数解析作为例子，分析了那些不必要的复杂性来源，以及避免的策略。

下面只是粗浅的分析。

## core code flow

在
```c
    for(;;) {
        if (optind >= argc)
            break;
        if (argv[optind][0] != '-') {
            loc_set_cmdline(argv, optind, 1);
            drive_add(IF_DEFAULT, 0, argv[optind++], HD_OPTS);
        } else {
            const QEMUOption *popt;

            popt = lookup_opt(argc, argv, &optarg, &optind);
            if (!(popt->arch_mask & arch_type)) {
                error_report("Option not supported for this target");
                exit(1);
            }
            switch(popt->index) {
            case QEMU_OPTION_cpu:
```


- qemu_opts_parse_noisily : 从 vl.c 开始调用，遇到一个 -foo bar 之类就匹配一个
  - opts_parse
    - opts_parse_id : 当参数为类似 -device nvme,drive=nvme1,serial=foo -drive file=${ext4_img1},format=raw,if=none,id=nvme1 的时候，获取到 id=nvme1
    - qemu_opts_create : 创建 opts 并且将其插入到 QemuOptsList 上
    - opts_do_parse : 用于解析出来一个一个的 QemuOpt 插入到 QemuOptsList 上
      - get_opt_name_value : 将一个参数拆分开来, 比如 2,maxcpus=3 就可以拆分为两个
      - opt_create : 将解析出来的参数划分为使用 QemuOpt 包装，并且插入到 QemuOptsList 上
  - qemu_opts_print_help : 解析出现错误，那么就报错


1. 三层结构: QemuOptsList QemuOpts 和 QemuOpt 的关系可以从下面两个参数理解
```plain
-device nvme,drive=nvme1,serial=foo -drive file=${ext4_img1},format=raw,if=none,id=nvme1
-device nvme,drive=nvme2,serial=foo -drive file=${ext4_img2},format=raw,if=none,id=nvme2
```
创建的 device 都在 QEMU_OPTION_device 中，每一个 `-device` 创建出来一个 QemuOpts，而 `format=raw` 对应一个 QemuOpt

2. 所有的 QEMUOption 的定义在一个全局变量 qemu_options 中间，在 qemu_init 的一个巨大的 for(;;) 中 因为参数都是 -cpu 之类的，可以将这些参数一行行的分离开
  - [ ] 具体是在哪一个函数上的

3. 注意，QemuOptsList 一个 group 只有一个，通过 qemu_add_opts 添加，通过 qemu_find_opts 查询.
  - 而 qemu_opts_create 是在 QemuOptsList 创建 QemuOpts 的

4. QemuOptsList::merge_lists 的作用是
  - 其实 `-smp 2,maxcpus=3` 也可以写为 `-smp 2 -smp maxcpus=3`，所以

## qemu-options.def
这是一个经典的例子:
```c
DEF("drive", HAS_ARG, QEMU_OPTION_drive,
"-drive [file=file][,if=type][,bus=n][,unit=m][,media=d][,index=i]\n"
"       [,cache=writethrough|writeback|none|directsync|unsafe][,format=f]\n"
"       [,snapshot=on|off][,rerror=ignore|stop|report]\n"
"       [,werror=ignore|stop|report|enospc][,id=name]\n"
"       [,aio=threads|native|io_uring]\n"
"       [,readonly=on|off][,copy-on-read=on|off]\n"
"       [,discard=ignore|unmap][,detect-zeroes=on|off|unmap]\n"
"       [[,bps=b]|[[,bps_rd=r][,bps_wr=w]]]\n"
"       [[,iops=i]|[[,iops_rd=r][,iops_wr=w]]]\n"
"       [[,bps_max=bm]|[[,bps_rd_max=rm][,bps_wr_max=wm]]]\n"
"       [[,iops_max=im]|[[,iops_rd_max=irm][,iops_wr_max=iwm]]]\n"
"       [[,iops_size=is]]\n"
"       [[,group=g]]\n"
"                use 'file' as a drive image\n", QEMU_ARCH_ALL)
```

qemu-options.def 会在三个位置 include，因为每次 include 前面 macro 的定义不同，而解析出来不同的内容

### opt_enum
1. 将每一个 drive 中的 opt_enum 找出来，从而
```c
enum {

#define DEF(option, opt_arg, opt_enum, opt_help, arch_mask)     \
    opt_enum,
#define DEFHEADING(text)
#define ARCHHEADING(text, arch_mask)

#include "qemu-options.def"
};
```

将会生成如下的 enum
```c
enum {
  // ....
  QEMU_OPTION_drive,
};
```
其使用位置为

2. 将其中的 help 信息打印出来
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

3. 构建 qemu_options 出来, 在 lookup_opt 中被唯一 reference
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

## qemu_init : 分解为一个个的 QemuOpts
在 vl.c:qemu_init 中的巨大循环中
```c
for(;;) {
  const QEMUOption *popt;

  popt = lookup_opt(argc, argv, &optarg, &optind);
  if (!(popt->arch_mask & arch_type)) {
      error_report("Option not supported for this target");
      exit(1);
  }
  switch(popt->index) {
    // 在这里从巨大的范围中选择
    case QEMU_OPTION_drive:
        if (drive_def(optarg) == NULL) {
            exit(1);
        }
        break;
```

其中通过 lookup_opt 可以将
```plain
-chardev file,path=/tmp/seabios.log,id=seabios 15
```
其两个参数返回值为
poptarg = file,path=/tmp/seabios.log,id=seabios
15 = poptind
同时函数返回值为命中的 QEMUOption

## 移植的方案

原先在 vl.c 中间调用的
```c
            case QEMU_OPTION_xtm:
                opts = qemu_opts_parse_noisily(qemu_find_opts("xtm"),
                                               optarg, true);
                x86_to_mips_parse_options(opts);
                break;
```


- lookup_opt 返回的就是
```c
typedef struct QEMUOption {
    const char *name;
    int flags;
    int index;
    uint32_t arch_mask;
} QEMUOption;
```
