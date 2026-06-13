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
#1  object_property_find_err (errp=0x7fffffffb0a0, name=0x5555570c5fc0 "nono", obj=0x55555737d790) at ../qom/object.c:1348
#2  object_property_find_err (obj=0x55555737d790, name=0x5555570c5fc0 "nono", errp=0x7fffffffb0a0) at ../qom/object.c:1343
#3  0x0000555555d02a89 in object_property_set (obj=obj@entry=0x55555737d790, name=name@entry=0x5555570c5fc0 "nono", v=v@entry=0x55555737da30, errp=0x7fffffffb0a0, errp@entry=0x555556fd44d8 <error_fatal>) at ../qom/object.c:1444
#4  0x0000555555d03a10 in object_property_parse (obj=obj@entry=0x55555737d790, name=name@entry=0x5555570c5fc0 "nono", string=string@entry=0x5555570c5f60 "1", errp=errp@entry=0x555556fd44d8 <error_fatal>) at ../qom/object.c:1703
#5  0x0000555555b149db in object_parse_property_opt (skip=0x555555fb49f6 "accel", errp=0x555556fd44d8 <error_fatal>, value=0x5555570c5f60 "1", name=0x5555570c5fc0 "nono", obj=0x55555737d790) at ../system/vl.c:1709
#6  accelerator_set_property (opaque=0x55555737d790, name=0x5555570c5fc0 "nono", value=0x5555570c5f60 "1",
```

## qemu 的 help
<!-- 06ae1551-a4eb-4ee0-ab83-5a6eab3f13f5 -->

终于知道 qemu 如何查询设备的
- qemu-system-x86_64 -device vhost-user-scsi-pci,help
- qemu-system-x86_64 -machine pc,accel=kvm,help
- qemu-system-x86_64 -machine help
- qemu-system-x86_64 -nic model=help
- qemu-system-x86_64 -device nvme,help
- qemu-system-x86_64 -device help
还是不知道怎么解决的问题: 加入我想要查询 kvm 的各种 property 的时候，应该如何处理?



发现 qemu 的 help 可以输出一些非常辅助理解的东西:

      qemu-system-$(uname -m) --help
      qemu-system-$(uname -m) -device help
      qemu-system-$(uname -m) -object help
      qemu-system-$(uname -m) -device nvme,help
      qemu-system-$(uname -m) -device virtio-blk-pci,help
      qemu-system-$(uname -m) -device qemu-xhci,help
      qemu-system-$(uname -m) -object qtest,help
      qemu-system-$(uname -m) -cpu help
      qemu-system-$(uname -m) -machine help
      qemu-system-$(uname -m) -machine pc,help
      qemu-system-$(uname -m) -smp help
      qemu-system-$(uname -m) -blockdev help
      qemu-system-$(uname -m) -netdev help
      qemu-system-$(uname -m) -nic help
      qemu-system-$(uname -m) -L help

如何才可以获取到 drive 的 help 呢？
是的确没有吗?

### qemu-system-$(uname -m) --help

### qemu-system-$(uname -m) -device help


- main
  - qemu_init
    - qemu_process_help_options
      - qemu_opts_foreach
        - qdev_device_help

### qemu-system-$(uname -m) -object help

- main
  - qemu_init
    - object_option_parse
      - user_creatable_print_help
        - user_creatable_print_types

```c
static void user_creatable_print_types(void)
{
    GSList *l, *list;

    qemu_printf("List of user creatable objects:\n");
    list = object_class_get_list_sorted(TYPE_USER_CREATABLE, false);
    for (l = list; l != NULL; l = l->next) {
        ObjectClass *oc = OBJECT_CLASS(l->data);
        qemu_printf("  %s\n", object_class_get_name(oc));
    }
    g_slist_free(list);
}
```

```txt
🧀  ./qemu-system-$(uname -m) -object help
List of user creatable objects:
  acpi-generic-initiator
  acpi-generic-port
  authz-list
  authz-list-file
  authz-simple
  can-bus
  can-host-socketcan
  colo-compare
  cryptodev-backend
  cryptodev-backend-builtin
  cryptodev-vhost-user
  dbus-display
  dbus-vmstate
  filter-buffer
  filter-dump
  filter-mirror
  filter-redirector
  filter-replay
  filter-rewriter
  input-barrier
  input-linux
  iommufd
  iothread
  main-loop
  memory-backend-file
  memory-backend-memfd
  memory-backend-ms
  memory-backend-ram
  memory-backend-shm
  pr-manager-helper
  qtest
  rng-builtin
  rng-egd
  rng-random
  secret
  secret_keyring
  sev-guest
  sev-snp-guest
  thread-context
  throttle-group
  tls-creds-anon
  tls-creds-psk
  tls-creds-x509
  x-remote-object
```

还可以创建出来 main-loop ，这个东西的作用是什么需要理解下。

### qemu-system-$(uname -m) -device virtio-balloon,help

```txt
🧀  qemu-system-$(uname -m) -device virtio-balloon,help
virtio-balloon-pci options:
  acpi-index=<uint32>    -  (default: 0)
  addr=<str>             - Slot and optional function number, example: 06.0 or 06 (default: -1)
  aer=<bool>             - on/off (default: off)
  any_layout=<bool>      - on/off (default: on)
  ats=<bool>             - on/off (default: off)
  busnr=<busnr>
  disable-legacy=<OnOffAuto> - on/off/auto (default: auto)
  disable-modern=<bool>  - on/off (default: off)
  event_idx=<bool>       - on/off (default: on)
  failover_pair_id=<str>
  deflate-on-oom=<bool>  - on/off (default: off)
  free-page-hint=<bool>  - on/off (default: off)
  free-page-reporting=<bool> - on/off (default: off)
  guest-stats-polling-interval=<int>
  guest-stats=<guest statistics>
  in_order=<bool>        - on/off (default: off)
  indirect_desc=<bool>   - on/off (default: on)
  ioeventfd=<bool>       - on/off (default: on)
  iommu_platform=<bool>  - on/off (default: off)
  iothread=<link<iothread>>
  modern-pio-notify=<bool> - on/off (default: off)
  multifunction=<bool>   - on/off (default: off)
  notify_on_empty=<bool> - on/off (default: on)
  packed=<bool>          - on/off (default: off)
  page-per-vq=<bool>     - on/off (default: off)
  page-poison=<bool>     - on/off (default: on)
  qemu-4-0-config-size=<bool> - on/off (default: off)
  queue_reset=<bool>     - on/off (default: on)
  rombar=<int32>         -  (default: -1)
  romfile=<str>
  romsize=<uint32>       -  (default: 4294967295)
  sriov-pf=<str>
  use-disabled-flag=<bool> - on/off (default: on)
  use-started=<bool>     - on/off (default: on)
  vectors=<uint32>       -  (default: 4294967295)
  virtio-backend=<child<virtio-balloon-device>>
  virtio-pci-bus-master-bug-migration=<bool> - on/off (default: off)
  x-ats-page-aligned=<bool> - on/off (default: on)
  x-disable-legacy-check=<bool> - on/off (default: off)
  x-ignore-backend-features=<bool> - on/off (default: off)
  x-max-bounce-buffer-size=<size> - Maximum buffer size allocated for bounce buffers used for mapped access to indirect DMA memory (default: 4096)
  x-pcie-ari-nextfn-1=<bool> - on/off (default: off)
  x-pcie-deverr-init=<bool> - on/off (default: on)
  x-pcie-err-unc-mask=<bool> - on/off (default: on)
  x-pcie-ext-tag=<bool>  - on/off (default: on)
  x-pcie-extcap-init=<bool> - on/off (default: on)
  x-pcie-flr-init=<bool> - on/off (default: on)
  x-pcie-lnkctl-init=<bool> - on/off (default: on)
  x-pcie-lnksta-dllla=<bool> - on/off (default: on)
  x-pcie-pm-init=<bool>  - on/off (default: on)
  x-pcie-pm-no-soft-reset=<bool> - on/off (default: off)
```

### qemu-system-$(uname -m) -device nvme,help
```
nvme options:
  account-failed=<OnOffAuto> - on/off/auto (default: "auto")
  account-invalid=<OnOffAuto> - on/off/auto (default: "auto")
  acpi-index=<uint32>    -  (default: 0)
  addr=<int32>           - Slot and optional function number, example: 06.0 or 06 (default: -1)
  aer_max_queued=<uint32> -  (default: 64)
  aerl=<uint8>           -  (default: 3)
  atomic.awun=<uint16>   -  (default: 0)
  atomic.awupf=<uint16>  -  (default: 0)
  atomic.dn=<bool>       -  (default: false)
  backend_defaults=<OnOffAuto> - on/off/auto (default: "auto")
  bootindex=<int32>
  busnr=<busnr>
  cmb_size_mb=<uint32>   -  (default: 0)
  ctratt.mem=<bool>      -  (default: false)
  discard_granularity=<size> -  (default: 4294967295)
  drive=<str>            - Node name or ID of a block device to use as a backend
  failover_pair_id=<str>
  ioeventfd=<bool>       -  (default: false)
  logical_block_size=<size> - A power of two between 512 B and 2 MiB (default: 0)
  max_ioqpairs=<uint32>  -  (default: 64)
  mdts=<uint8>           -  (default: 7)
  min_io_size=<size>     -  (default: 0)
  mqes=<uint16>          -  (default: 2047)
  msix-exclusive-bar=<bool> -  (default: false)
  msix_qsize=<uint16>    -  (default: 65)
  multifunction=<bool>   - on/off (default: false)
  num_queues=<uint32>    -  (default: 0)
  opt_io_size=<size>     -  (default: 0)
  physical_block_size=<size> - A power of two between 512 B and 2 MiB (default: 0)
  pmrdev=<link<memory-backend>>
  rombar=<uint32>        -  (default: 1)
  romfile=<str>
  romsize=<uint32>       -  (default: 4294967295)
  serial=<str>
  share-rw=<bool>        -  (default: false)
  smart_critical_warning=<uint8>
  spdm_port=<uint16>     -  (default: 0)
  sriov_max_vfs=<uint16> -  (default: 0)
  sriov_max_vi_per_vf=<uint32> -  (default: 0)
  sriov_max_vq_per_vf=<uint32> -  (default: 0)
  sriov_vi_flexible=<uint16> -  (default: 0)
  sriov_vq_flexible=<uint16> -  (default: 0)
  subsys=<link<nvme-subsys>>
  use-intel-id=<bool>    -  (default: false)
  vsl=<uint8>            -  (default: 7)
  write-cache=<OnOffAuto> - on/off/auto (default: "auto")
  x-max-bounce-buffer-size=<size> - Maximum buffer size allocated for bounce buffers used for mapped access to indirect DMA memory (default: 4096)
  x-pcie-ari-nextfn-1=<bool> - on/off (default: false)
  x-pcie-err-unc-mask=<bool> - on/off (default: true)
  x-pcie-ext-tag=<bool>  - on/off (default: true)
  x-pcie-extcap-init=<bool> - on/off (default: true)
  x-pcie-lnksta-dllla=<bool> - on/off (default: true)
  zoned.auto_transition=<bool> -  (default: true)
  zoned.zasl=<uint8>     -  (default: 0)
```


### qemu-system-$(uname -m) -device virtio-blk-pci,help

qemu-system-$(uname -m) --help 中已经说过
```txt
-device driver[,prop[=value][,...]]
                add device (based on driver)
                prop=value,... sets driver properties
                use '-device help' to print all possible drivers
                use '-device driver,help' to print all possible properties
```


这个就很复杂了:
- main
  - qemu_init
    - qemu_process_help_options
      - qemu_opts_foreach
        - qdev_device_help
          - qmp_device_list_properties
            - object_new_with_type
              - object_initialize_with_type
                - object_init_with_type
                  - object_init_with_type
                    - virtio_blk_pci_instance_init
                      - virtio_instance_init_common
                        - object_initialize_child_with_props
                          - object_initialize_child_with_propsv
                            - object_property_add_child

显然在 hw/block/virtio-blk.c:virtio_blk_properties ，所以通过这个例子，
是一个理解 qemu 的 qom 机制的好方法:

```txt
🍺 qemu-system-$(uname -m) -device virtio-blk,help
virtio-blk-pci options:
  account-failed=<OnOffAuto> - on/off/auto (default: "auto")
  account-invalid=<OnOffAuto> - on/off/auto (default: "auto")
  acpi-index=<uint32>    -  (default: 0)
  addr=<int32>           - Slot and optional function number, example: 06.0 or 06 (default: -1)
  aer=<bool>             - on/off (default: false)
  any_layout=<bool>      - on/off (default: true)
  ats=<bool>             - on/off (default: false)
  backend_defaults=<OnOffAuto> - on/off/auto (default: "auto")
  bootindex=<int32>
  busnr=<busnr>
  class=<uint32>         -  (default: 0)
  config-wce=<bool>      - on/off (default: true)
  cyls=<uint32>          -  (default: 0)
  disable-legacy=<OnOffAuto> - on/off/auto (default: "auto")
  disable-modern=<bool>  -  (default: false)
  discard=<bool>         - on/off (default: true)
  discard_granularity=<size> -  (default: 4294967295)
  drive=<str>            - Node name or ID of a block device to use as a backend
  event_idx=<bool>       - on/off (default: true)
  failover_pair_id=<str>
  heads=<uint32>         -  (default: 0)
  in_order=<bool>        - on/off (default: false)
  indirect_desc=<bool>   - on/off (default: true)
  ioeventfd=<bool>       - on/off (default: true)
  iommu_platform=<bool>  - on/off (default: false)
  iothread-vq-mapping=<IOThreadVirtQueueMappingList> - IOThread virtqueue mapping list [{"iothread":"<id>", "vqs":[1,2,3,...]},...]
  iothread=<link<iothread>>
  lcyls=<uint32>         -  (default: 0)
  lheads=<uint32>        -  (default: 0)
  logical_block_size=<size> - A power of two between 512 B and 2 MiB (default: 0)
  lsecs=<uint32>         -  (default: 0)
  max-discard-sectors=<uint32> -  (default: 4194303)
  max-write-zeroes-sectors=<uint32> -  (default: 4194303)
  migrate-extra=<bool>   - on/off (default: true)
  min_io_size=<size>     -  (default: 0)
  modern-pio-notify=<bool> - on/off (default: false)
  multifunction=<bool>   - on/off (default: false)
  notify_on_empty=<bool> - on/off (default: true)
  num-queues=<uint16>    -  (default: 65535)
  opt_io_size=<size>     -  (default: 0)
  packed=<bool>          - on/off (default: false)
  page-per-vq=<bool>     - on/off (default: false)
  physical_block_size=<size> - A power of two between 512 B and 2 MiB (default: 0)
  queue-size=<uint16>    -  (default: 256)
  queue_reset=<bool>     - on/off (default: true)
  report-discard-granularity=<bool> -  (default: true)
  request-merging=<bool> - on/off (default: true)
  rerror=<BlockdevOnError> - Error handling policy, report/ignore/enospc/stop/auto (default: "auto")
  rombar=<uint32>        -  (default: 1)
  romfile=<str>
  romsize=<uint32>       -  (default: 4294967295)
  secs=<uint32>          -  (default: 0)
  seg-max-adjust=<bool>  -  (default: true)
  serial=<str>
  share-rw=<bool>        -  (default: false)
  use-disabled-flag=<bool> -  (default: true)
  use-started=<bool>     -  (default: true)
  vectors=<uint32>       -  (default: 4294967295)
  virtio-backend=<child<virtio-blk-device>>
  virtio-pci-bus-master-bug-migration=<bool> - on/off (default: false)
  werror=<BlockdevOnError> - Error handling policy, report/ignore/enospc/stop/auto (default: "auto")
  write-cache=<OnOffAuto> - on/off/auto (default: "auto")
  write-zeroes=<bool>    - on/off (default: true)
  x-ats-page-aligned=<bool> - on/off (default: true)
  x-disable-legacy-check=<bool> -  (default: false)
  x-disable-pcie=<bool>  - on/off (default: false)
  x-enable-wce-if-config-wce=<bool> -  (default: true)
  x-ignore-backend-features=<bool> -  (default: false)
  x-max-bounce-buffer-size=<size> - Maximum buffer size allocated for bounce buffers used for mapped access to indirect DMA memory (default: 4096)
  x-pcie-ari-nextfn-1=<bool> - on/off (default: false)
  x-pcie-deverr-init=<bool> - on/off (default: true)
  x-pcie-err-unc-mask=<bool> - on/off (default: true)
  x-pcie-ext-tag=<bool>  - on/off (default: true)
  x-pcie-extcap-init=<bool> - on/off (default: true)
  x-pcie-flr-init=<bool> - on/off (default: true)
  x-pcie-lnkctl-init=<bool> - on/off (default: true)
  x-pcie-lnksta-dllla=<bool> - on/off (default: true)
  x-pcie-pm-init=<bool>  - on/off (default: true)
  x-pcie-pm-no-soft-reset=<bool> - on/off (default: false)
```

### qemu-system-$(uname -m) -device qemu-xhci,help
```txt
qemu-system-$(uname -m) -device qemu-xhci,help
qemu-xhci options:
  acpi-index=<uint32>    -  (default: 0)
  addr=<int32>           - Slot and optional function number, example: 06.0 or 06 (default: -1)
  busnr=<busnr>
  failover_pair_id=<str>
  host=<link<device>>
  multifunction=<bool>   - on/off (default: false)
  p2=<uint32>            -  (default: 4)
  p3=<uint32>            -  (default: 4)
  rombar=<uint32>        -  (default: 1)
  romfile=<str>
  romsize=<uint32>       -  (default: 4294967295)
  streams=<bool>         - on/off (default: true)
  x-max-bounce-buffer-size=<size> - Maximum buffer size allocated for bounce buffers used for mapped access to indirect DMA memory (default: 4096)
  x-pcie-ari-nextfn-1=<bool> - on/off (default: false)
  x-pcie-err-unc-mask=<bool> - on/off (default: true)
  x-pcie-ext-tag=<bool>  - on/off (default: true)
  x-pcie-extcap-init=<bool> - on/off (default: true)
  x-pcie-lnksta-dllla=<bool> - on/off (default: true)
  xhci-core=<child<base-xhci>>
```

### qemu-system-$(uname -m) -object qtest,help

- main
  - qemu_init
    - object_option_parse
      - user_creatable_print_help
        - type_print_class_properties



### qemu-system-$(uname -m) -cpu help

- main
  - qemu_init
    - qemu_process_help_options
      - x86_cpu_list
        - g_slist_foreach
          - x86_cpu_list_entry

### qemu-system-$(uname -m) -machine help

- main
  - qemu_init
    - machine_help_func

既然 qom 设计的那么复杂，考虑的是成千上万的设备才对啊

难道还有一部分复杂性来自于
```txt
qemu-system-x86_64 --machine help

qemu-system-x86_64 --cpu help
```
只有这么点设备的话，如果维持 100w 的代码量的啊?

这里的东西可以仔细看看
```txt
Supported machines are:
microvm              microvm (i386)
pc                   Standard PC (i440FX + PIIX, 1996) (alias of pc-i440fx-9.1)
pc-i440fx-9.1        Standard PC (i440FX + PIIX, 1996) (default)
pc-i440fx-9.0        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-8.2        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-8.1        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-8.0        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-7.2        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-7.1        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-7.0        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-6.2        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-6.1        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-6.0        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-5.2        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-5.1        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-5.0        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-4.2        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-4.1        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-4.0        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-3.1        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-3.0        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-2.9        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-2.8        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-2.7        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-2.6        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-2.5        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-2.4        Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-2.12       Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-2.11       Standard PC (i440FX + PIIX, 1996) (deprecated)
pc-i440fx-2.10       Standard PC (i440FX + PIIX, 1996) (deprecated)
q35                  Standard PC (Q35 + ICH9, 2009) (alias of pc-q35-9.1)
pc-q35-9.1           Standard PC (Q35 + ICH9, 2009)
pc-q35-9.0           Standard PC (Q35 + ICH9, 2009)
pc-q35-8.2           Standard PC (Q35 + ICH9, 2009)
pc-q35-8.1           Standard PC (Q35 + ICH9, 2009)
pc-q35-8.0           Standard PC (Q35 + ICH9, 2009)
pc-q35-7.2           Standard PC (Q35 + ICH9, 2009)
pc-q35-7.1           Standard PC (Q35 + ICH9, 2009)
pc-q35-7.0           Standard PC (Q35 + ICH9, 2009)
pc-q35-6.2           Standard PC (Q35 + ICH9, 2009)
pc-q35-6.1           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-6.0           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-5.2           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-5.1           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-5.0           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-4.2           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-4.1           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-4.0.1         Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-4.0           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-3.1           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-3.0           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-2.9           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-2.8           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-2.7           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-2.6           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-2.5           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-2.4           Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-2.12          Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-2.11          Standard PC (Q35 + ICH9, 2009) (deprecated)
pc-q35-2.10          Standard PC (Q35 + ICH9, 2009) (deprecated)
isapc                ISA-only PC
none                 empty machine
x-remote             Experimental remote machine
```

- 这么多 alias 是做什么的?
- 测试一下 none x-remote microvm
- 有办法定义一个新的设备吗?

想不到 arm 环境中更加复杂:
```txt
[nix-shell:~]$ qemu-system-$(uname -m) --machine help
Supported machines are:
akita                Sharp SL-C1000 (Akita) PDA (PXA270) (deprecated)
ast1030-evb          Aspeed AST1030 MiniBMC (Cortex-M4)
ast2500-evb          Aspeed AST2500 EVB (ARM1176)
ast2600-evb          Aspeed AST2600 EVB (Cortex-A7)
ast2700-evb          Aspeed AST2700 EVB (Cortex-A35)
b-l475e-iot01a       B-L475E-IOT01A Discovery Kit (Cortex-M4)
bletchley-bmc        Facebook Bletchley BMC (Cortex-A7)
borzoi               Sharp SL-C3100 (Borzoi) PDA (PXA270) (deprecated)
bpim2u               Bananapi M2U (Cortex-A7)
canon-a1100          Canon PowerShot A1100 IS (ARM946)
cheetah              Palm Tungsten|E aka. Cheetah PDA (OMAP310) (deprecated)
collie               Sharp SL-5500 (Collie) PDA (SA-1110)
connex               Gumstix Connex (PXA255) (deprecated)
cubieboard           cubietech cubieboard (Cortex-A8)
emcraft-sf2          SmartFusion2 SOM kit from Emcraft (M2S010)
fby35-bmc            Facebook fby35 BMC (Cortex-A7)
fby35                Meta Platforms fby35
fp5280g2-bmc         Inspur FP5280G2 BMC (ARM1176)
fuji-bmc             Facebook Fuji BMC (Cortex-A7)
g220a-bmc            Bytedance G220A BMC (ARM1176)
highbank             Calxeda Highbank (ECX-1000)
imx25-pdk            ARM i.MX25 PDK board (ARM926)
integratorcp         ARM Integrator/CP (ARM926EJ-S)
kudo-bmc             Kudo BMC (Cortex-A9)
kzm                  ARM KZM Emulation Baseboard (ARM1136)
lm3s6965evb          Stellaris LM3S6965EVB (Cortex-M3)
lm3s811evb           Stellaris LM3S811EVB (Cortex-M3)
mainstone            Mainstone II (PXA27x) (deprecated)
mcimx6ul-evk         Freescale i.MX6UL Evaluation Kit (Cortex-A7)
mcimx7d-sabre        Freescale i.MX7 DUAL SABRE (Cortex-A7)
microbit             BBC micro:bit (Cortex-M0)
midway               Calxeda Midway (ECX-2000)
mori-bmc             Mori BMC (Cortex-A9)
mps2-an385           ARM MPS2 with AN385 FPGA image for Cortex-M3
mps2-an386           ARM MPS2 with AN386 FPGA image for Cortex-M4
mps2-an500           ARM MPS2 with AN500 FPGA image for Cortex-M7
mps2-an505           ARM MPS2 with AN505 FPGA image for Cortex-M33
mps2-an511           ARM MPS2 with AN511 DesignStart FPGA image for Cortex-M3
mps2-an521           ARM MPS2 with AN521 FPGA image for dual Cortex-M33
mps3-an524           ARM MPS3 with AN524 FPGA image for dual Cortex-M33
mps3-an536           ARM MPS3 with AN536 FPGA image for Cortex-R52
mps3-an547           ARM MPS3 with AN547 FPGA image for Cortex-M55
musca-a              ARM Musca-A board (dual Cortex-M33)
musca-b1             ARM Musca-B1 board (dual Cortex-M33)
musicpal             Marvell 88w8618 / MusicPal (ARM926EJ-S)
n800                 Nokia N800 tablet aka. RX-34 (OMAP2420) (deprecated)
n810                 Nokia N810 tablet aka. RX-44 (OMAP2420) (deprecated)
netduino2            Netduino 2 Machine (Cortex-M3)
netduinoplus2        Netduino Plus 2 Machine (Cortex-M4)
none                 empty machine
npcm750-evb          Nuvoton NPCM750 Evaluation Board (Cortex-A9)
nuri                 Samsung NURI board (Exynos4210)
olimex-stm32-h405    Olimex STM32-H405 (Cortex-M4)
orangepi-pc          Orange Pi PC (Cortex-A7)
palmetto-bmc         OpenPOWER Palmetto BMC (ARM926EJ-S)
qcom-dc-scm-v1-bmc   Qualcomm DC-SCM V1 BMC (Cortex A7)
qcom-firework-bmc    Qualcomm DC-SCM V1/Firework BMC (Cortex A7)
quanta-gbs-bmc       Quanta GBS (Cortex-A9)
quanta-gsj           Quanta GSJ (Cortex-A9)
quanta-q71l-bmc      Quanta-Q71l BMC (ARM926EJ-S)
rainier-bmc          IBM Rainier BMC (Cortex-A7)
raspi0               Raspberry Pi Zero (revision 1.2)
raspi1ap             Raspberry Pi A+ (revision 1.1)
raspi2b              Raspberry Pi 2B (revision 1.1)
raspi3ap             Raspberry Pi 3A+ (revision 1.0)
raspi3b              Raspberry Pi 3B (revision 1.2)
raspi4b              Raspberry Pi 4B (revision 1.5)
realview-eb          ARM RealView Emulation Baseboard (ARM926EJ-S)
realview-eb-mpcore   ARM RealView Emulation Baseboard (ARM11MPCore)
realview-pb-a8       ARM RealView Platform Baseboard for Cortex-A8
realview-pbx-a9      ARM RealView Platform Baseboard Explore for Cortex-A9
romulus-bmc          OpenPOWER Romulus BMC (ARM1176)
sabrelite            Freescale i.MX6 Quad SABRE Lite Board (Cortex-A9)
sbsa-ref             QEMU 'SBSA Reference' ARM Virtual Machine
smdkc210             Samsung SMDKC210 board (Exynos4210)
sonorapass-bmc       OCP SonoraPass BMC (ARM1176)
spitz                Sharp SL-C3000 (Spitz) PDA (PXA270) (deprecated)
stm32vldiscovery     ST STM32VLDISCOVERY (Cortex-M3)
supermicro-x11spi-bmc Supermicro X11 SPI BMC (ARM1176)
supermicrox11-bmc    Supermicro X11 BMC (ARM926EJ-S)
sx1                  Siemens SX1 (OMAP310) V2
sx1-v1               Siemens SX1 (OMAP310) V1
tacoma-bmc           OpenPOWER Tacoma BMC (Cortex-A7) (deprecated)
terrier              Sharp SL-C3200 (Terrier) PDA (PXA270) (deprecated)
tiogapass-bmc        Facebook Tiogapass BMC (ARM1176)
tosa                 Sharp SL-6000 (Tosa) PDA (PXA255) (deprecated)
verdex               Gumstix Verdex Pro XL6P COMs (PXA270) (deprecated)
versatileab          ARM Versatile/AB (ARM926EJ-S)
versatilepb          ARM Versatile/PB (ARM926EJ-S)
vexpress-a15         ARM Versatile Express for Cortex-A15
vexpress-a9          ARM Versatile Express for Cortex-A9
virt-2.10            QEMU 2.10 ARM Virtual Machine (deprecated)
virt-2.11            QEMU 2.11 ARM Virtual Machine (deprecated)
virt-2.12            QEMU 2.12 ARM Virtual Machine (deprecated)
virt-2.6             QEMU 2.6 ARM Virtual Machine (deprecated)
virt-2.7             QEMU 2.7 ARM Virtual Machine (deprecated)
virt-2.8             QEMU 2.8 ARM Virtual Machine (deprecated)
virt-2.9             QEMU 2.9 ARM Virtual Machine (deprecated)
virt-3.0             QEMU 3.0 ARM Virtual Machine (deprecated)
virt-3.1             QEMU 3.1 ARM Virtual Machine (deprecated)
virt-4.0             QEMU 4.0 ARM Virtual Machine (deprecated)
virt-4.1             QEMU 4.1 ARM Virtual Machine (deprecated)
virt-4.2             QEMU 4.2 ARM Virtual Machine (deprecated)
virt-5.0             QEMU 5.0 ARM Virtual Machine (deprecated)
virt-5.1             QEMU 5.1 ARM Virtual Machine (deprecated)
virt-5.2             QEMU 5.2 ARM Virtual Machine (deprecated)
virt-6.0             QEMU 6.0 ARM Virtual Machine (deprecated)
virt-6.1             QEMU 6.1 ARM Virtual Machine (deprecated)
virt-6.2             QEMU 6.2 ARM Virtual Machine
virt-7.0             QEMU 7.0 ARM Virtual Machine
virt-7.1             QEMU 7.1 ARM Virtual Machine
virt-7.2             QEMU 7.2 ARM Virtual Machine
virt-8.0             QEMU 8.0 ARM Virtual Machine
virt-8.1             QEMU 8.1 ARM Virtual Machine
virt-8.2             QEMU 8.2 ARM Virtual Machine
virt-9.0             QEMU 9.0 ARM Virtual Machine
virt                 QEMU 9.1 ARM Virtual Machine (alias of virt-9.1)
virt-9.1             QEMU 9.1 ARM Virtual Machine
witherspoon-bmc      OpenPOWER Witherspoon BMC (ARM1176)
x-remote             Experimental remote machine
xilinx-zynq-a9       Xilinx Zynq Platform Baseboard for Cortex-A9
xlnx-versal-virt     Xilinx Versal Virtual development board
xlnx-zcu102          Xilinx ZynqMP ZCU102 board with 4xA53s and 2xR5Fs based on the value of smp
yosemitev2-bmc       Facebook YosemiteV2 BMC (ARM1176)
z2                   Zipit Z2 (PXA27x) (deprecated)
```

### qemu-system-$(uname -m) -machine pc,help

- main
  - qemu_init
    - machine_help_func
      - type_print_class_properties

```txt
🧀  qemu-system-$(uname -m) -machine pc,help
pc-i440fx-9.2-machine options:
  acpi=<OnOffAuto>       - Enable ACPI
  append=<string>        - Linux kernel command line
  boot=<BootConfiguration> - Boot configuration
  bus-lock-ratelimit=<uint64_t> - Set the ratelimit for the bus locks acquired in VMs
  confidential-guest-support=<link<confidential-guest-support>> - Set confidential guest scheme to support
  default-bus-bypass-iommu=<bool>
  dt-compatible=<string> - Overrides the "compatible" property of the dt root node
  dtb=<string>           - Linux kernel device tree file
  dump-guest-core=<bool> - Include guest memory in a core dump
  dumpdtb=<string>       - Dump current dtb to a file and quit
  fd-bootchk=<bool>
  firmware=<string>      - Firmware image
  graphics=<bool>        - Set on/off to enable/disable graphics emulation
  hpet=<bool>            - Enable/disable high precision event timer emulation
  i8042=<bool>           - Enable/disable Intel 8042 PS/2 controller emulation
  initrd=<string>        - Linux initial ramdisk file
  kernel=<string>        - Linux kernel image file
  max-fw-size=<size>     - Maximum combined firmware size
  max-ram-below-4g=<size> - Maximum ram below the 4G boundary (32bit boundary)
  mem-merge=<bool>       - Enable/disable memory merge support
  memory-backend=<link<memory-backend>> - Set RAM backendValid value is ID of hostmem based backend
  memory-encryption=<string> - Set memory encryption object to use
  memory=<MemorySizeConfiguration> - Memory size configuration
  phandle-start=<int>    - The first phandle ID we may generate dynamically
  pic=<OnOffAuto>        - Enable i8259 PIC
  pit=<OnOffAuto>        - Enable i8254 PIT
  sata=<bool>            - Enable/disable Serial ATA bus
  sgx-epc=<SgxEPC>       - SGX EPC device
  smbios-entry-point-type=<str> - SMBIOS Entry Point type [32, 64]
  smbus=<bool>           - Enable/disable system management bus
  smm=<OnOffAuto>        - Enable SMM
  smp-cache=<SmpCachePropertiesWrapper> - Cache properties list for SMP machine
  smp=<SMPConfiguration> - CPU topology
  suppress-vmdesc=<bool> - Set on to disable self-describing migration
  usb=<bool>             - Set on/off to enable/disable usb
  vmport=<OnOffAuto>     - Enable vmport (pc & q35)
  x-oem-id=<string>      - Override the default value of field OEMID in ACPI table header.The string may be up to 6 bytes in size
  x-oem-table-id=<string> - Override the default value of field OEM Table ID in ACPI table header.The string may be up to 8 bytes in size
  x-south-bridge=<PCSouthBridgeOption> - Use a different south bridge than PIIX3
```

```txt
🧀  qemu-system-$(uname -m) -machine q35,help
pc-q35-9.2-machine options:
  acpi=<OnOffAuto>       - Enable ACPI
  append=<string>        - Linux kernel command line
  boot=<BootConfiguration> - Boot configuration
  bus-lock-ratelimit=<uint64_t> - Set the ratelimit for the bus locks acquired in VMs
  confidential-guest-support=<link<confidential-guest-support>> - Set confidential guest scheme to support
  default-bus-bypass-iommu=<bool>
  dt-compatible=<string> - Overrides the "compatible" property of the dt root node
  dtb=<string>           - Linux kernel device tree file
  dump-guest-core=<bool> - Include guest memory in a core dump
  dumpdtb=<string>       - Dump current dtb to a file and quit
  fd-bootchk=<bool>
  firmware=<string>      - Firmware image
  graphics=<bool>        - Set on/off to enable/disable graphics emulation
  hpet=<bool>            - Enable/disable high precision event timer emulation
  i8042=<bool>           - Enable/disable Intel 8042 PS/2 controller emulation
  initrd=<string>        - Linux initial ramdisk file
  kernel=<string>        - Linux kernel image file
  max-fw-size=<size>     - Maximum combined firmware size
  max-ram-below-4g=<size> - Maximum ram below the 4G boundary (32bit boundary)
  mem-merge=<bool>       - Enable/disable memory merge support
  memory-backend=<link<memory-backend>> - Set RAM backendValid value is ID of hostmem based backend
  memory-encryption=<string> - Set memory encryption object to use
  memory=<MemorySizeConfiguration> - Memory size configuration
  phandle-start=<int>    - The first phandle ID we may generate dynamically
  pic=<OnOffAuto>        - Enable i8259 PIC
  pit=<OnOffAuto>        - Enable i8254 PIT
  sata=<bool>            - Enable/disable Serial ATA bus
  sgx-epc=<SgxEPC>       - SGX EPC device
  smbios-entry-point-type=<str> - SMBIOS Entry Point type [32, 64]
  smbus=<bool>           - Enable/disable system management bus
  smm=<OnOffAuto>        - Enable SMM
  smp-cache=<SmpCachePropertiesWrapper> - Cache properties list for SMP machine
  smp=<SMPConfiguration> - CPU topology
  suppress-vmdesc=<bool> - Set on to disable self-describing migration
  usb=<bool>             - Set on/off to enable/disable usb
  vmport=<OnOffAuto>     - Enable vmport (pc & q35)
  x-oem-id=<string>      - Override the default value of field OEMID in ACPI table header.The string may be up to 6 bytes in size
  x-oem-table-id=<string> - Override the default value of field OEM Table ID in ACPI table header.The string may be up to 8 bytes in size
```

```txt
🧀  qemu-system-$(uname -m) -machine microvm,help
microvm-machine options:
  acpi=<OnOffAuto>       - Enable ACPI
  append=<string>        - Linux kernel command line
  auto-kernel-cmdline=<bool> - Set off to disable adding virtio-mmio devices to the kernel cmdline
  boot=<BootConfiguration> - Boot configuration
  bus-lock-ratelimit=<uint64_t> - Set the ratelimit for the bus locks acquired in VMs
  confidential-guest-support=<link<confidential-guest-support>> - Set confidential guest scheme to support
  dt-compatible=<string> - Overrides the "compatible" property of the dt root node
  dtb=<string>           - Linux kernel device tree file
  dump-guest-core=<bool> - Include guest memory in a core dump
  dumpdtb=<string>       - Dump current dtb to a file and quit
  firmware=<string>      - Firmware image
  graphics=<bool>        - Set on/off to enable/disable graphics emulation
  initrd=<string>        - Linux initial ramdisk file
  ioapic2=<OnOffAuto>    - Enable second IO-APIC
  isa-serial=<bool>      - Set off to disable the instantiation an ISA serial port
  kernel=<string>        - Linux kernel image file
  mem-merge=<bool>       - Enable/disable memory merge support
  memory-backend=<link<memory-backend>> - Set RAM backendValid value is ID of hostmem based backend
  memory-encryption=<string> - Set memory encryption object to use
  memory=<MemorySizeConfiguration> - Memory size configuration
  pcie=<OnOffAuto>       - Enable PCIe
  phandle-start=<int>    - The first phandle ID we may generate dynamically
  pic=<OnOffAuto>        - Enable i8259 PIC
  pit=<OnOffAuto>        - Enable i8254 PIT
  rtc=<OnOffAuto>        - Enable MC146818 RTC
  sgx-epc=<SgxEPC>       - SGX EPC device
  smm=<OnOffAuto>        - Enable SMM
  smp-cache=<SmpCachePropertiesWrapper> - Cache properties list for SMP machine
  smp=<SMPConfiguration> - CPU topology
  suppress-vmdesc=<bool> - Set on to disable self-describing migration
  usb=<bool>             - Set on/off to enable/disable usb
  x-oem-id=<string>      - Override the default value of field OEMID in ACPI table header.The string may be up to 6 bytes in size
  x-oem-table-id=<string> - Override the default value of field OEM Table ID in ACPI table header.The string may be up to 8 bytes in size
  x-option-roms=<bool>   - Set off to disable loading option ROMs
```
这里的参数很有意思的，

1. 如果 usb=off 那么就看不到板载的 usb 控制器了
```txt
00:01.2 USB controller [0c03]: Intel Corporation 82371SB PIIX3 USB [Natoma/Triton II] [8086:7020] (rev 01)
```

### qemu-system-$(uname -m) -chardev help

```txt
  ringbuf
  serial
  stdio
  spicevmc
  mux
  pipe
  qemu-vdagent
  hub
  null
  pty
  msmouse
  socket
  spiceport
  vc
  parallel
  dbus
  memory
  udp
  file
  wctablet
  testdev
```

### qemu-system-$(uname -m) -smp help

- main
  - qemu_init
    - machine_parse_property_opt
      - machine_parse_property_opt
        - qemu_opts_print_help


### qemu-system-$(uname -m) -netdev help

```txt
qemu-system-$(uname -m) -netdev help
Available netdev backend types:
socket
stream
dgram
hubport
tap
user
l2tpv3
vde
bridge
vhost-user
vhost-vdpa
```

- main
  - qemu_init
    - qemu_create_late_backends
      - net_init_clients
        - qemu_opts_foreach
          - net_init_netdev
            - net_init_netdev
              - show_netdevs



### qemu-system-$(uname -m) -nic help

```txt
🧀  qemu-system-$(uname -m) -nic help
Available netdev backend types:
socket
stream
dgram
hubport
tap
user
l2tpv3
vde
bridge
vhost-user
vhost-vdpa

Available NIC models (use -nic model=help for a filtered list):
e1000
e1000-82544gc
e1000-82545em
e1000e
i82550
i82551
i82557a
i82557b
i82557c
i82558a
i82558b
i82559a
i82559b
i82559c
i82559er
i82562
i82801
igb
ne2k_isa
ne2k_pci
pcnet
rtl8139
tulip
usb-net
virtio-net-device
virtio-net-pci
virtio-net-pci-non-transitional
virtio-net-pci-transitional
vmxnet3
xen-net-device
```
### qemu-system-$(uname -m) -L help

实现方法简单粗暴:
```c
static void qemu_process_help_options(void)
{
    /*
     * Check for -cpu help and -device help before we call select_machine(),
     * which will return an error if the architecture has no default machine
     * type and the user did not specify one, so that the user doesn't need
     * to say '-cpu help -machine something'.
     */
    if (cpu_option && is_help_option(cpu_option)) {
        list_cpus();
        exit(0);
    }

    if (qemu_opts_foreach(qemu_find_opts("device"),
                          device_help_func, NULL, NULL)) {
        exit(0);
    }

    /* -L help lists the data directories and exits. */
    if (list_data_dirs) {
        qemu_list_data_dirs();
        exit(0);
    }
}
```

- system/datadir.c

qemu_find_file

### qemu-system-$(uname -m) -audiodev help

### 不存在的
#### qemu-system-$(uname -m) -blockdev help
#### qemu-system-$(uname -m) -netdev user,help

### hmp : info qdm

和 qemu-system-$(uname -m) -device help 的输出非常类似，但是 -device help 不会输出
gpex-pcihost 等，具体差别有待分析

```txt
(qemu) info qdm
Controller/Bridge/Hub devices:
name "cxl-downstream", bus PCI, desc "CXL Switch Downstream Port"
name "cxl-rp", bus PCI, desc "CXL Root Port"
name "cxl-upstream", bus PCI, desc "CXL Switch Upstream Port"
name "gpex-pcihost", bus System, no-user
name "gpex-root", bus PCI, desc "QEMU generic PCIe host bridge", no-user
name "gpio_i2c", bus System, desc "Virtual GPIO to I2C bridge", no-user
name "i82801b11-bridge", bus PCI
name "ICH9-LPC", bus PCI, desc "ICH9 LPC bridge", no-user
name "ioh3420", bus PCI, desc "Intel IOH device id 3420 PCIE Root Port"
name "isabus-bridge", bus System, no-user
name "mch", bus PCI, desc "Host bridge", no-user
name "pci-bridge", bus PCI, desc "Standard PCI Bridge"
name "pci-bridge-seat", bus PCI, desc "Standard PCI Bridge (multiseat)"
name "pcie-pci-bridge", bus PCI
name "pcie-root-port", bus PCI, desc "PCI Express Root Port"
name "pxb", bus PCI, desc "PCI Expander Bridge"
name "pxb-cxl", bus PCI, desc "CXL Host Bridge"
name "pxb-pcie", bus PCI, desc "PCI Express Expander Bridge"
name "q35-pcihost", bus System, no-user
name "remote-pcihost", bus System, no-user
name "usb-host", bus usb-bus
name "usb-hub", bus usb-bus
name "vfio-pci-igd-lpc-bridge", bus PCI, desc "VFIO dummy ISA/LPC bridge for IGD assignment"
name "vmbus-bridge", bus System
name "x3130-upstream", bus PCI, desc "TI X3130 Upstream Port of PCI Express Switch"
name "xio3130-downstream", bus PCI, desc "TI X3130 Downstream Port of PCI Express Switch"

USB devices:
name "ich9-usb-ehci1", bus PCI
name "ich9-usb-ehci2", bus PCI
name "ich9-usb-uhci1", bus PCI
name "ich9-usb-uhci2", bus PCI
name "ich9-usb-uhci3", bus PCI
name "ich9-usb-uhci4", bus PCI
name "ich9-usb-uhci5", bus PCI
name "ich9-usb-uhci6", bus PCI
name "nec-usb-xhci", bus PCI
name "pci-ohci", bus PCI, desc "Apple USB Controller"
name "piix3-usb-uhci", bus PCI
name "piix4-usb-uhci", bus PCI
name "qemu-xhci", bus PCI
name "usb-ehci", bus PCI

Storage devices:
name "am53c974", bus PCI, desc "AMD Am53c974 PCscsi-PCI SCSI adapter"
name "cfi.pflash01", bus System, no-user
name "cxl-type3", bus PCI, desc "CXL Memory Device (Type 3)"
name "dc390", bus PCI, desc "Tekram DC-390 SCSI adapter"
name "emmc", bus sd-bus, desc "eMMC", no-user
name "esp", no-user
name "floppy", bus floppy-bus, desc "virtual floppy drive"
name "generic-sdhci", bus System, no-user
name "ich9-ahci", bus PCI, alias "ahci"
name "ide-cd", bus IDE, desc "virtual IDE CD-ROM"
name "ide-cf", bus IDE, desc "virtual CompactFlash card"
name "ide-hd", bus IDE, desc "virtual IDE disk"
name "imx-usdhc", bus System, no-user
name "isa-fdc", bus ISA, desc "virtual floppy controller"
name "isa-ide", bus ISA
name "lsi53c810", bus PCI
name "lsi53c895a", bus PCI, alias "lsi"
name "megasas", bus PCI, desc "LSI MegaRAID SAS 1078"
name "megasas-gen2", bus PCI, desc "LSI MegaRAID SAS 2108"
name "mptsas1068", bus PCI, desc "LSI SAS 1068"
name "nvdimm", desc "DIMM memory module"
name "nvme", bus PCI, desc "Non-Volatile Memory Express"
name "nvme-ns", bus nvme-bus, desc "Virtual NVMe namespace"
name "nvme-subsys", desc "Virtual NVMe subsystem"
name "piix3-ide", bus PCI
name "piix4-ide", bus PCI
name "pvscsi", bus PCI
name "s3c-sdhci", bus System, no-user
name "scsi-block", bus SCSI, desc "SCSI block device passthrough"
name "scsi-cd", bus SCSI, desc "virtual SCSI CD-ROM"
name "scsi-generic", bus SCSI, desc "pass through generic scsi device (/dev/sg*)"
name "scsi-hd", bus SCSI, desc "virtual SCSI disk"
name "sd-card", bus sd-bus
name "sd-card-spi", bus sd-bus, desc "SD SPI"
name "sdhci-pci", bus PCI
name "sysbus-esp", bus System, no-user
name "ufs", bus PCI, desc "Universal Flash Storage"
name "usb-bot", bus usb-bus
name "usb-mtp", bus usb-bus, desc "USB Media Transfer Protocol device"
name "usb-storage", bus usb-bus
name "usb-uas", bus usb-bus
name "vhost-scsi", bus virtio-bus
name "vhost-scsi-pci", bus PCI
name "vhost-scsi-pci-non-transitional", bus PCI
name "vhost-scsi-pci-transitional", bus PCI
name "vhost-user-blk", bus virtio-bus
name "vhost-user-blk-pci", bus PCI
name "vhost-user-blk-pci-non-transitional", bus PCI
name "vhost-user-blk-pci-transitional", bus PCI
name "vhost-user-fs-device", bus virtio-bus
name "vhost-user-fs-pci", bus PCI
name "vhost-user-scsi", bus virtio-bus
name "vhost-user-scsi-pci", bus PCI
name "vhost-user-scsi-pci-non-transitional", bus PCI
name "vhost-user-scsi-pci-transitional", bus PCI
name "virtio-9p-device", bus virtio-bus
name "virtio-9p-pci", bus PCI, alias "virtio-9p"
name "virtio-9p-pci-non-transitional", bus PCI
name "virtio-9p-pci-transitional", bus PCI
name "virtio-blk-device", bus virtio-bus
name "virtio-blk-pci", bus PCI, alias "virtio-blk"
name "virtio-blk-pci-non-transitional", bus PCI
name "virtio-blk-pci-transitional", bus PCI
name "virtio-dummy-pci", bus PCI, alias "virtio-dummy"
name "virtio-dummy-pci-non-transitional", bus PCI
name "virtio-dummy-pci-transitional", bus PCI
name "virtio-pmem", bus virtio-bus
name "virtio-scsi-device", bus virtio-bus
name "virtio-scsi-pci", bus PCI, alias "virtio-scsi"
name "virtio-scsi-pci-non-transitional", bus PCI
name "virtio-scsi-pci-transitional", bus PCI

Network devices:
name "e1000", bus PCI, alias "e1000-82540em", desc "Intel Gigabit Ethernet"
name "e1000-82544gc", bus PCI, desc "Intel Gigabit Ethernet"
name "e1000-82545em", bus PCI, desc "Intel Gigabit Ethernet"
name "e1000e", bus PCI, desc "Intel 82574L GbE Controller"
name "i82550", bus PCI, desc "Intel i82550 Ethernet"
name "i82551", bus PCI, desc "Intel i82551 Ethernet"
name "i82557a", bus PCI, desc "Intel i82557A Ethernet"
name "i82557b", bus PCI, desc "Intel i82557B Ethernet"
name "i82557c", bus PCI, desc "Intel i82557C Ethernet"
name "i82558a", bus PCI, desc "Intel i82558A Ethernet"
name "i82558b", bus PCI, desc "Intel i82558B Ethernet"
name "i82559a", bus PCI, desc "Intel i82559A Ethernet"
name "i82559b", bus PCI, desc "Intel i82559B Ethernet"
name "i82559c", bus PCI, desc "Intel i82559C Ethernet"
name "i82559er", bus PCI, desc "Intel i82559ER Ethernet"
name "i82562", bus PCI, desc "Intel i82562 Ethernet"
name "i82801", bus PCI, desc "Intel i82801 Ethernet"
name "igb", bus PCI, desc "Intel 82576 Gigabit Ethernet Controller"
name "igbvf", bus PCI, desc "Intel 82576 Virtual Function", no-user
name "ne2k_isa", bus ISA
name "ne2k_pci", bus PCI
name "pcnet", bus PCI
name "rocker", bus PCI, desc "Rocker Switch"
name "rtl8139", bus PCI
name "tulip", bus PCI
name "usb-net", bus usb-bus
name "virtio-net-device", bus virtio-bus
name "virtio-net-pci", bus PCI, alias "virtio-net"
name "virtio-net-pci-non-transitional", bus PCI
name "virtio-net-pci-transitional", bus PCI
name "vmxnet3", bus PCI, desc "VMWare Paravirtualized Ethernet v3"
name "xen-net-device", bus xen-bus

Input devices:
name "i8042", bus ISA
name "i8042-mmio", bus System, no-user
name "ipoctal232", bus IndustryPack, desc "GE IP-Octal 232 8-channel RS-232 IndustryPack"
name "isa-parallel", bus ISA
name "isa-serial", bus ISA
name "pci-serial", bus PCI
name "pci-serial-2x", bus PCI
name "pci-serial-4x", bus PCI
name "ps2-kbd", bus System, no-user
name "ps2-mouse", bus System, no-user
name "tpci200", bus PCI, desc "TEWS TPCI200 IndustryPack carrier"
name "usb-braille", bus usb-bus
name "usb-ccid", bus usb-bus, desc "CCID Rev 1.1 smartcard reader"
name "usb-kbd", bus usb-bus
name "usb-mouse", bus usb-bus
name "usb-serial", bus usb-bus
name "usb-tablet", bus usb-bus
name "usb-wacom-tablet", bus usb-bus, desc "QEMU PenPartner Tablet"
name "vhost-user-device", bus virtio-bus, no-user
name "vhost-user-device-pci", bus PCI, no-user
name "vhost-user-gpio-device", bus virtio-bus
name "vhost-user-gpio-pci", bus PCI
name "vhost-user-i2c-device", bus virtio-bus
name "vhost-user-i2c-pci", bus PCI
name "vhost-user-input", bus virtio-bus
name "vhost-user-input-pci", bus PCI
name "vhost-user-rng", bus virtio-bus
name "vhost-user-rng-pci", bus PCI
name "virtconsole", bus virtio-serial-bus
name "virtio-input-host-device", bus virtio-bus
name "virtio-input-host-pci", bus PCI, alias "virtio-input-host"
name "virtio-keyboard-device", bus virtio-bus
name "virtio-keyboard-pci", bus PCI, alias "virtio-keyboard"
name "virtio-mouse-device", bus virtio-bus
name "virtio-mouse-pci", bus PCI, alias "virtio-mouse"
name "virtio-multitouch-device", bus virtio-bus
name "virtio-multitouch-pci", bus PCI
name "virtio-serial-device", bus virtio-bus
name "virtio-serial-pci", bus PCI, alias "virtio-serial"
name "virtio-serial-pci-non-transitional", bus PCI
name "virtio-serial-pci-transitional", bus PCI
name "virtio-tablet-device", bus virtio-bus
name "virtio-tablet-pci", bus PCI, alias "virtio-tablet"
name "virtserialport", bus virtio-serial-bus
name "vmmouse", bus ISA

Display devices:
name "ati-vga", bus PCI
name "bochs-display", bus PCI
name "cirrus-vga", bus PCI, desc "Cirrus CLGD 54xx VGA"
name "gpu", bus PCI
name "isa-cirrus-vga", bus ISA
name "isa-vga", bus ISA
name "ramfb", bus System, desc "ram framebuffer standalone device"
name "secondary-vga", bus PCI
name "VGA", bus PCI
name "vhost-user-gpu", bus virtio-bus
name "vhost-user-gpu-pci", bus PCI
name "vhost-user-vga", bus PCI
name "virtio-gpu-device", bus virtio-bus
name "virtio-gpu-gl-device", bus virtio-bus
name "virtio-gpu-gl-pci", bus PCI, alias "virtio-gpu-gl"
name "virtio-gpu-pci", bus PCI, alias "virtio-gpu"
name "virtio-vga", bus PCI
name "virtio-vga-gl", bus PCI
name "vmware-svga", bus PCI

Sound devices:
name "AC97", bus PCI, alias "ac97", desc "Intel 82801AA AC97 Audio"
name "adlib", bus ISA, desc "Yamaha YM3812 (OPL2)"
name "cs4231a", bus ISA, desc "Crystal Semiconductor CS4231A"
name "ES1370", bus PCI, alias "es1370", desc "ENSONIQ AudioPCI ES1370"
name "gus", bus ISA, desc "Gravis Ultrasound GF1"
name "hda-duplex", bus HDA, desc "HDA Audio Codec, duplex (line-out, line-in)"
name "hda-micro", bus HDA, desc "HDA Audio Codec, duplex (speaker, microphone)"
name "hda-output", bus HDA, desc "HDA Audio Codec, output-only (line-out)"
name "ich9-intel-hda", bus PCI, desc "Intel HD Audio Controller (ich9)"
name "intel-hda", bus PCI, desc "Intel HD Audio Controller (ich6)"
name "isa-pcspk", bus ISA, no-user
name "sb16", bus ISA, desc "Creative Sound Blaster 16"
name "usb-audio", bus usb-bus
name "vhost-user-snd", bus virtio-bus
name "vhost-user-snd-pci", bus PCI
name "virtio-sound-device", bus virtio-bus
name "virtio-sound-pci", bus PCI, alias "virtio-sound", desc "Virtio Sound"

Misc devices:
name "acpi-erst", bus PCI, desc "ACPI Error Record Serialization Table (ERST) device"
name "amd-iommu", bus System, desc "AMD IOMMU (AMD-Vi) DMA Remapping device"
name "AMDVI-PCI", bus PCI, desc "AMD IOMMU (AMD-Vi) DMA Remapping device"
name "ctucan_pci", bus PCI, desc "CTU CAN PCI"
name "edu", bus PCI
name "guest-loader", desc "Guest Loader"
name "hv-balloon", bus vmbus
name "hv-syndbg"
name "hyperv-testdev", bus ISA
name "i2c-ddc", bus i2c-bus
name "i2c-echo", bus i2c-bus
name "intel-iommu", bus System, desc "Intel IOMMU (VT-d) DMA Remapping device"
name "isa-applesmc", bus ISA
name "isa-debug-exit", bus ISA
name "isa-debugcon", bus ISA
name "ivshmem-doorbell", bus PCI, desc "Inter-VM shared memory"
name "ivshmem-flat", bus System, no-user
name "ivshmem-plain", bus PCI, desc "Inter-VM shared memory"
name "kvaser_pci", bus PCI, desc "Kvaser PCICANx"
name "loader", desc "Generic Loader"
name "mc146818rtc", bus ISA
name "mioe3680_pci", bus PCI, desc "Mioe3680 PCICANx"
name "pc-testdev", bus ISA
name "pci-testdev", bus PCI, desc "PCI Test Device"
name "pcm3680_pci", bus PCI, desc "Pcm3680i PCICANx"
name "pvpanic", bus ISA
name "pvpanic-pci", bus PCI
name "smbus-eeprom", bus i2c-bus, no-user
name "smbus-ipmi", bus i2c-bus
name "tpm-crb"
name "tpm-tis", bus ISA
name "u2f-passthru", bus usb-bus, desc "QEMU U2F passthrough key"
name "uefi-vars-sysbus", bus System
name "uefi-vars-x64", bus System
name "vfio-pci", bus PCI, desc "VFIO-based PCI device assignment"
name "vfio-pci-nohotplug", bus PCI, desc "VFIO-based PCI device assignment"
name "vfio-user-pci", bus PCI, desc "VFIO over socket PCI device assignment"
name "vhost-user-vsock-device", bus virtio-bus
name "vhost-user-vsock-pci", bus PCI
name "vhost-user-vsock-pci-non-transitional", bus PCI
name "vhost-vdpa-device", bus virtio-bus, desc "VDPA-based generic device assignment"
name "vhost-vdpa-device-pci", bus PCI
name "vhost-vdpa-device-pci-non-transitional", bus PCI
name "vhost-vdpa-device-pci-transitional", bus PCI
name "vhost-vsock-device", bus virtio-bus
name "vhost-vsock-pci", bus PCI
name "vhost-vsock-pci-non-transitional", bus PCI
name "virtio-balloon-device", bus virtio-bus
name "virtio-balloon-pci", bus PCI, alias "virtio-balloon"
name "virtio-balloon-pci-non-transitional", bus PCI
name "virtio-balloon-pci-transitional", bus PCI
name "virtio-crypto-device", bus virtio-bus
name "virtio-crypto-pci", bus PCI
name "virtio-dummy", bus virtio-bus
name "virtio-iommu-device", bus virtio-bus
name "virtio-iommu-pci", bus PCI, alias "virtio-iommu"
name "virtio-mem", bus virtio-bus
name "virtio-mem-pci", bus PCI
name "virtio-mmio", bus System, no-user
name "virtio-pmem-pci", bus PCI
name "virtio-rng-device", bus virtio-bus
name "virtio-rng-pci", bus PCI, alias "virtio-rng"
name "virtio-rng-pci-non-transitional", bus PCI
name "virtio-rng-pci-transitional", bus PCI
name "vmclock"
name "vmcoreinfo"
name "vmgenid"
name "xen-backend", bus xen-sysbus
name "xen-platform", bus PCI, desc "XEN platform pci device"

CPU devices:
name "486-v1-x86_64-cpu"
name "486-x86_64-cpu"
name "athlon-v1-x86_64-cpu"
name "athlon-x86_64-cpu"
name "base-x86_64-cpu"
name "Broadwell-IBRS-x86_64-cpu"
name "Broadwell-noTSX-IBRS-x86_64-cpu"
name "Broadwell-noTSX-x86_64-cpu"
name "Broadwell-v1-x86_64-cpu"
name "Broadwell-v2-x86_64-cpu"
name "Broadwell-v3-x86_64-cpu"
name "Broadwell-v4-x86_64-cpu"
name "Broadwell-x86_64-cpu"
name "Cascadelake-Server-noTSX-x86_64-cpu"
name "Cascadelake-Server-v1-x86_64-cpu"
name "Cascadelake-Server-v2-x86_64-cpu"
name "Cascadelake-Server-v3-x86_64-cpu"
name "Cascadelake-Server-v4-x86_64-cpu"
name "Cascadelake-Server-v5-x86_64-cpu"
name "Cascadelake-Server-x86_64-cpu"
name "ClearwaterForest-v1-x86_64-cpu"
name "ClearwaterForest-x86_64-cpu"
name "Conroe-v1-x86_64-cpu"
name "Conroe-x86_64-cpu"
name "Cooperlake-v1-x86_64-cpu"
name "Cooperlake-v2-x86_64-cpu"
name "Cooperlake-x86_64-cpu"
name "core2duo-v1-x86_64-cpu"
name "core2duo-x86_64-cpu"
name "coreduo-v1-x86_64-cpu"
name "coreduo-x86_64-cpu"
name "Denverton-v1-x86_64-cpu"
name "Denverton-v2-x86_64-cpu"
name "Denverton-v3-x86_64-cpu"
name "Denverton-x86_64-cpu"
name "Dhyana-v1-x86_64-cpu"
name "Dhyana-v2-x86_64-cpu"
name "Dhyana-x86_64-cpu"
name "EPYC-Genoa-v1-x86_64-cpu"
name "EPYC-Genoa-v2-x86_64-cpu"
name "EPYC-Genoa-x86_64-cpu"
name "EPYC-IBPB-x86_64-cpu"
name "EPYC-Milan-v1-x86_64-cpu"
name "EPYC-Milan-v2-x86_64-cpu"
name "EPYC-Milan-v3-x86_64-cpu"
name "EPYC-Milan-x86_64-cpu"
name "EPYC-Rome-v1-x86_64-cpu"
name "EPYC-Rome-v2-x86_64-cpu"
name "EPYC-Rome-v3-x86_64-cpu"
name "EPYC-Rome-v4-x86_64-cpu"
name "EPYC-Rome-v5-x86_64-cpu"
name "EPYC-Rome-x86_64-cpu"
name "EPYC-Turin-v1-x86_64-cpu"
name "EPYC-Turin-x86_64-cpu"
name "EPYC-v1-x86_64-cpu"
name "EPYC-v2-x86_64-cpu"
name "EPYC-v3-x86_64-cpu"
name "EPYC-v4-x86_64-cpu"
name "EPYC-v5-x86_64-cpu"
name "EPYC-x86_64-cpu"
name "GraniteRapids-v1-x86_64-cpu"
name "GraniteRapids-v2-x86_64-cpu"
name "GraniteRapids-v3-x86_64-cpu"
name "GraniteRapids-x86_64-cpu"
name "Haswell-IBRS-x86_64-cpu"
name "Haswell-noTSX-IBRS-x86_64-cpu"
name "Haswell-noTSX-x86_64-cpu"
name "Haswell-v1-x86_64-cpu"
name "Haswell-v2-x86_64-cpu"
name "Haswell-v3-x86_64-cpu"
name "Haswell-v4-x86_64-cpu"
name "Haswell-x86_64-cpu"
name "host-x86_64-cpu"
name "Icelake-Server-noTSX-x86_64-cpu"
name "Icelake-Server-v1-x86_64-cpu"
name "Icelake-Server-v2-x86_64-cpu"
name "Icelake-Server-v3-x86_64-cpu"
name "Icelake-Server-v4-x86_64-cpu"
name "Icelake-Server-v5-x86_64-cpu"
name "Icelake-Server-v6-x86_64-cpu"
name "Icelake-Server-v7-x86_64-cpu"
name "Icelake-Server-x86_64-cpu"
name "IvyBridge-IBRS-x86_64-cpu"
name "IvyBridge-v1-x86_64-cpu"
name "IvyBridge-v2-x86_64-cpu"
name "IvyBridge-x86_64-cpu"
name "KnightsMill-v1-x86_64-cpu"
name "KnightsMill-x86_64-cpu"
name "kvm32-v1-x86_64-cpu"
name "kvm32-x86_64-cpu"
name "kvm64-v1-x86_64-cpu"
name "kvm64-x86_64-cpu"
name "max-x86_64-cpu"
name "n270-v1-x86_64-cpu"
name "n270-x86_64-cpu"
name "Nehalem-IBRS-x86_64-cpu"
name "Nehalem-v1-x86_64-cpu"
name "Nehalem-v2-x86_64-cpu"
name "Nehalem-x86_64-cpu"
name "Opteron_G1-v1-x86_64-cpu"
name "Opteron_G1-x86_64-cpu"
name "Opteron_G2-v1-x86_64-cpu"
name "Opteron_G2-x86_64-cpu"
name "Opteron_G3-v1-x86_64-cpu"
name "Opteron_G3-x86_64-cpu"
name "Opteron_G4-v1-x86_64-cpu"
name "Opteron_G4-x86_64-cpu"
name "Opteron_G5-v1-x86_64-cpu"
name "Opteron_G5-x86_64-cpu"
name "Penryn-v1-x86_64-cpu"
name "Penryn-x86_64-cpu"
name "pentium-v1-x86_64-cpu"
name "pentium-x86_64-cpu"
name "pentium2-v1-x86_64-cpu"
name "pentium2-x86_64-cpu"
name "pentium3-v1-x86_64-cpu"
name "pentium3-x86_64-cpu"
name "phenom-v1-x86_64-cpu"
name "phenom-x86_64-cpu"
name "qemu32-v1-x86_64-cpu"
name "qemu32-x86_64-cpu"
name "qemu64-v1-x86_64-cpu"
name "qemu64-x86_64-cpu"
name "SandyBridge-IBRS-x86_64-cpu"
name "SandyBridge-v1-x86_64-cpu"
name "SandyBridge-v2-x86_64-cpu"
name "SandyBridge-x86_64-cpu"
name "SapphireRapids-v1-x86_64-cpu"
name "SapphireRapids-v2-x86_64-cpu"
name "SapphireRapids-v3-x86_64-cpu"
name "SapphireRapids-v4-x86_64-cpu"
name "SapphireRapids-x86_64-cpu"
name "SierraForest-v1-x86_64-cpu"
name "SierraForest-v2-x86_64-cpu"
name "SierraForest-v3-x86_64-cpu"
name "SierraForest-x86_64-cpu"
name "Skylake-Client-IBRS-x86_64-cpu"
name "Skylake-Client-noTSX-IBRS-x86_64-cpu"
name "Skylake-Client-v1-x86_64-cpu"
name "Skylake-Client-v2-x86_64-cpu"
name "Skylake-Client-v3-x86_64-cpu"
name "Skylake-Client-v4-x86_64-cpu"
name "Skylake-Client-x86_64-cpu"
name "Skylake-Server-IBRS-x86_64-cpu"
name "Skylake-Server-noTSX-IBRS-x86_64-cpu"
name "Skylake-Server-v1-x86_64-cpu"
name "Skylake-Server-v2-x86_64-cpu"
name "Skylake-Server-v3-x86_64-cpu"
name "Skylake-Server-v4-x86_64-cpu"
name "Skylake-Server-v5-x86_64-cpu"
name "Skylake-Server-x86_64-cpu"
name "Snowridge-v1-x86_64-cpu"
name "Snowridge-v2-x86_64-cpu"
name "Snowridge-v3-x86_64-cpu"
name "Snowridge-v4-x86_64-cpu"
name "Snowridge-x86_64-cpu"
name "Westmere-IBRS-x86_64-cpu"
name "Westmere-v1-x86_64-cpu"
name "Westmere-v2-x86_64-cpu"
name "Westmere-x86_64-cpu"
name "YongFeng-v1-x86_64-cpu"
name "YongFeng-v2-x86_64-cpu"
name "YongFeng-v3-x86_64-cpu"
name "YongFeng-x86_64-cpu"

Watchdog devices:
name "i6300esb", bus PCI, desc "Intel 6300ESB"
name "ib700", bus ISA, desc "iBASE 700"

Uncategorized devices:
name "acpi-ged", bus System, desc "ACPI Generic Event Device", no-user
name "apic", no-user
name "base-xhci", no-user
name "cxl-fmw", bus System, desc "CXL Fixed Memory Window", no-user
name "cxl-switch-mailbox-cci", bus PCI, desc "CXL Switch Mailbox CCI"
name "fw_cfg_io", bus System, no-user
name "fw_cfg_mem", bus System, no-user
name "hpet", bus System, no-user
name "hyperv-synic", no-user
name "i440FX", bus PCI, desc "Host bridge", no-user
name "i440FX-pcihost", bus System, no-user
name "i8257", bus ISA, no-user
name "ICH9-SMB", bus PCI, desc "ICH9 SMBUS Bridge", no-user
name "imx.usbphy", bus System, desc "i.MX USB PHY Module", no-user
name "ioapic", bus System, no-user
name "ipmi-bmc-extern"
name "ipmi-bmc-sim"
name "isa-i8259", bus ISA, no-user
name "isa-ipmi-bt", bus ISA
name "isa-ipmi-kcs", bus ISA
name "isa-pit", bus ISA, no-user
name "kvm-apic", no-user
name "kvm-i8259", bus ISA, no-user
name "kvm-ioapic", bus System, no-user
name "kvm-pit", bus ISA, no-user
name "kvmclock", bus System, no-user
name "kvmvapic", bus System, no-user
name "migration", no-user
name "pc-dimm", desc "DIMM memory module"
name "pci-ipmi-bt", bus PCI, desc "PCI IPMI BT"
name "pci-ipmi-kcs", bus PCI, desc "PCI IPMI KCS"
name "PIIX3", bus PCI, desc "ISA bridge", no-user
name "piix4-isa", bus PCI, desc "ISA bridge", no-user
name "PIIX4_PM", bus PCI, desc "PM", no-user
name "port92", bus ISA, no-user
name "pxb-cxl-host", bus System, no-user
name "pxb-host", bus System, no-user
name "serial", no-user
name "sgx-epc", desc "SGX EPC section", no-user
name "sysbus-xhci", bus System, no-user
name "ufs-lu", bus ufs-bus, desc "Virtual UFS logical unit"
name "vmport", bus ISA, no-user
name "x-pci-proxy-dev", bus PCI
name "xen-bridge", bus System, no-user
name "xen-cdrom", bus xen-bus, desc "Xen CD-ROM Device"
name "xen-console", bus xen-bus
name "xen-disk", bus xen-bus, desc "Xen Disk Device"
name "xen-evtchn", bus System, no-user
name "xen-gnttab", bus System, no-user
name "xen-overlay", bus System, no-user
name "xen-primary-console", bus System, no-user
name "xen-sysdev", bus System, no-user
name "xen-xenstore", bus System, no-user
```

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

## 扩展内容
- QemuOptsList::merge_lists : `-smp 2,maxcpus=3` 也可以写为 `-smp 2 -smp maxcpus=3`
- 参数之间存在引用，例如 blockdev 和 drive 直接，具体没有看，但是应该容易的
- [ ] libvirt 如何生成 qemu 的参数的


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
