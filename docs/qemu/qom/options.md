# QEMU 的参数解析

## 解释如何被解析的
这是 main 函数中的巨大的 for 循环，使用 lookup_opt 从左向右对于
参数扫描，每次匹配到一个完整的参数之后，就会返回 QEMUOption 和 optarg

qemu_init 函数中:
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
```txt
[drive] : [file=/home/maritns3/core/vn/hack/qemu/x64-e1000/img1.ext4,format=raw,if=none,id=nvme1]
[drive] : [file=/home/maritns3/core/vn/hack/qemu/x64-e1000/img2.ext4,format=raw,if=none,id=nvme2]
```

1. 一个 `-drive` 的参数会创建出来一个 QemuOpts
2. `-drive` 后面跟着的 file=... format=... if=... 和 id=... 都会创建出来一个 QemuOpt ，然后挂到 QemuOpts 上
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
4. 这些 QemuOptsList 通过调用 qemu_add_opts 保存到数组 vm_config_groups 中间，通过 qemu_find_opts 使用字符串查询到 QemuOptsList

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

将会生成如下的 enum
```c
enum {
  // ....
  QEMU_OPTION_drive,
};
```

这是在 qemu_init 中 中根据 QEMUOption::index 来做 switch case.

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

## 参数如何使用的

没有想象的那么智能，很多的参数解析开始的时候都是需要特殊的处理的

- main
  - qemu_init
    - qemu_create_early_backends
      - object_option_foreach_add
        - user_creatable_add_qapi
          - user_creatable_add_type
            - user_creatable_complete
              - event_loop_base_complete

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_create_cli_devices
        - qemu_opts_foreach
          - device_init_func
            - qdev_device_add
              - qdev_device_add_from_qdict
                - qdev_new

- main
  - qemu_init
    - qemu_create_machine
      - object_new_with_type
        - object_initialize_with_type
          - object_init_with_type
            - object_init_with_type
              - object_init_with_type
                - machine_initfn

### 通过 qom 机制来解析参数

故意添加错误参数，可以看到，所有的参数，都是 property
如果添加的参数不存在或者其他的什么不对，就会解析错误


-device virtio-blk,drive=virtio-blk1,id=virt-blk1,iothread=io0,nono=1

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_create_cli_devices
        - qemu_opts_foreach
          - device_init_func
            - qdev_device_add
              - qdev_device_add_from_qdict
                - object_set_properties_from_keyval
                  - object_set_properties_from_qdict
                    - object_set_properties_from_qdict
                      - object_property_set
                        - object_property_find_err
                          - object_property_find_err

```txt
#1  object_property_find_err (errp=0x7fffffffb040, name=0x555558db99f0 "nono", obj=0x555558dacff0) at ../qom/object.c:1348
#2  object_property_find_err (obj=0x555558dacff0, name=0x555558db99f0 "nono", errp=0x7fffffffb040) at ../qom/object.c:1343
#3  0x0000555555d02a89 in object_property_set (obj=obj@entry=0x555558dacff0, name=0x555558db99f0 "nono", v=v@entry=0x555558db9ab0, errp=errp@entry=0x7fffffffb040) at ../qom/object.c:1444
#4  0x0000555555d05a74 in object_set_properties_from_qdict (obj=0x555558dacff0, qdict=0x555558db8920, v=0x555558db9ab0, errp=0x7fffffffb040) at ../qom/object_interfaces.c:57
#5  0x0000555555d05c69 in object_set_properties_from_qdict (errp=0x7fffffffb040, v=0x555558db9ab0, qdict=0x555558db8920, obj=0x555558dacff0) at ../qom/object_interfaces.c:53
#6  object_set_properties_from_keyval (obj=0x555558dacff0, qdict=0x555558db8920, from_json=<optimized out>, errp=0x7fffffffb040) at ../qom/object_interfaces.c:75
#7  0x0000555555e35040 in qdev_device_add_from_qdict (opts=opts@entry=0x555557eadcb0, from_json=from_json@entry=false, errp=0x7fffffffb040, errp@entry=0x555556fd44d8 <error_fatal>) at ../system/qdev-monitor.c:707
#8  0x0000555555e3553a in qdev_device_add (opts=0x5555570c6780, errp=errp@entry=0x555556fd44d8 <error_fatal>) at ../system/qdev-monitor.c:733
```

热插的时候也是差不多的:
```txt
- hmp_device_add
  - qdev_device_add
    - qdev_device_add_from_qdict
      - object_set_properties_from_keyval
        - object_set_properties_from_qdict
          - object_set_properties_from_qdict
            - object_property_set
              - object_property_find_err
                - object_property_find_err
```

-accel kvm,nono=1

- main
  - qemu_init
    - configure_accelerators
      - qemu_opts_foreach
        - do_configure_accelerator
          - qemu_opt_foreach
            - accelerator_set_property
              - object_parse_property_opt
                - object_property_parse
                  - object_property_set
                    - object_property_find_err
                      - object_property_find_err
                        - xueshi

```txt
- accelerator_set_property
  - object_parse_property_opt
    - object_property_parse
      - object_property_set
        - object_property_find_err
          - object_property_find_err
```

## TODO
### 一些缺失的东西
kvm_accel_class_init 中的，
```txt

    object_class_property_set_description(oc, "kernel-irqchip",
        "Configure KVM in-kernel irqchip");
```
希望通过 qemu-system-$(uname -m) -accel kvm,help 来展示，但是没有

既然有 qemu-system-$(uname -m) -machine pc,help ，那么为什么没有呢？
为什么不可以通过 type_print_class_properties 来实现呢?


这个也是没有的:
```txt
🧀  qemu-system-$(uname -m) -blockdev help
qemu-system-$(uname -m): -blockdev help: Help is not available for this option
```
### 问题

```txt
	arg_nvme+=" -device nvme,drive=nvme_basic1,max_ioqpairs=14,serial=$(uuidgen),id=nvme_b1 "
	arg_nvme+=" -drive file=${nvme1},format=qcow2,if=none,id=nvme_basic1 "
```
这里的类似 max_ioqpairs=14 有办法全部都查询出来吗?

## 一些烦人的机制

QEMU 可以创建多个 virtio-serial controller，但需要显式指定 controller ID 和 port 所属 bus；
否则 QEMU 会把所有 virtconsole/virtserialport 自动接到默认的第一个 virtio-serial bus。

概念上是：

virtio-serial controller A
    ├── hvc0
    └── vport0

virtio-serial controller B
    └── vport1

QEMU 参数可以写成：

-device virtio-serial,id=serial0 \
-device virtio-serial,id=serial1 \
-device virtconsole,bus=serial0.0,chardev=hvc \
-device virtserialport,bus=serial0.0,nr=1,chardev=vport0,name=org.qemu.vport.0 \
-device virtserialport,bus=serial1.0,nr=1,chardev=vport1,name=org.qemu.vport.1

但通常没有必要为每个 port 创建一个 controller。一个 virtio-serial controller 可以承载多个 virtconsole/
virtserialport，只要为 port 分配不同的 nr 即可：

-device virtio-serial,id=serial0 \
-device virtconsole,bus=serial0.0,chardev=hvc \
-device virtserialport,bus=serial0.0,nr=1,chardev=vport0,name=org.qemu.vport.0 \
-device virtserialport,bus=serial0.0,nr=2,chardev=vport1,name=org.qemu.vport.1

当前参数没有显式 bus/nr，所以由 QEMU 自动选择第一个 controller 和下一个可用 port 编号。


类似还有 virtio-scsi 机制，QEMU 可以实现自动的挂 controller 的

## 扩展内容
- QemuOptsList::merge_lists : `-smp 2,maxcpus=3` 也可以写为 `-smp 2 -smp maxcpus=3`
- 参数之间存在引用，例如 blockdev 和 drive 直接，具体没有看，但是应该容易的
- [ ] libvirt 如何生成 qemu 的参数的
- https://techpiezo.com/linux/enable-audio-in-qemu-virtual-machine/ : 音频配置

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
