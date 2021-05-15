# qopt


```c
static void qemu_create_cli_devices(void)
{
    soundhw_init();

    qemu_opts_foreach(qemu_find_opts("fw_cfg"),
                      parse_fw_cfg, fw_cfg_find(), &error_fatal);

    /* init USB devices */
    if (machine_usb(current_machine)) {
        if (foreach_device_config(DEV_USB, usb_parse) < 0)
            exit(1);
    }

    /* init generic devices */
    rom_set_order_override(FW_CFG_ORDER_OVERRIDE_DEVICE);
    qemu_opts_foreach(qemu_find_opts("device"),
                      device_init_func, NULL, &error_fatal);
    rom_reset_order_override();
}
```


```c
void qemu_init(int argc, char **argv, char **envp)
{
    QemuOpts *opts;
    QemuOpts *icount_opts = NULL, *accel_opts = NULL;
    QemuOptsList *olist;
    int optind;
    const char *optarg;
    MachineClass *machine_class;
    bool userconfig = true;
    FILE *vmstate_dump_file = NULL;

    qemu_add_opts(&qemu_drive_opts);
    qemu_add_drive_opts(&qemu_legacy_drive_opts);
    qemu_add_drive_opts(&qemu_common_drive_opts);
    qemu_add_drive_opts(&qemu_drive_opts);
    qemu_add_drive_opts(&bdrv_runtime_opts);
    qemu_add_opts(&qemu_chardev_opts);
    qemu_add_opts(&qemu_device_opts);
    qemu_add_opts(&qemu_netdev_opts);
    qemu_add_opts(&qemu_nic_opts);
    qemu_add_opts(&qemu_net_opts);
    qemu_add_opts(&qemu_rtc_opts);
    qemu_add_opts(&qemu_global_opts);
    qemu_add_opts(&qemu_mon_opts);
    qemu_add_opts(&qemu_trace_opts);
    qemu_plugin_add_opts();
    qemu_add_opts(&qemu_option_rom_opts);
    qemu_add_opts(&qemu_machine_opts);
    qemu_add_opts(&qemu_accel_opts);
    qemu_add_opts(&qemu_mem_opts);
    qemu_add_opts(&qemu_smp_opts);
    qemu_add_opts(&qemu_boot_opts);
    qemu_add_opts(&qemu_add_fd_opts);
    qemu_add_opts(&qemu_object_opts);
    qemu_add_opts(&qemu_tpmdev_opts);
    qemu_add_opts(&qemu_overcommit_opts);
    qemu_add_opts(&qemu_msg_opts);
    qemu_add_opts(&qemu_name_opts);
    qemu_add_opts(&qemu_numa_opts);
    qemu_add_opts(&qemu_icount_opts);
    qemu_add_opts(&qemu_semihosting_config_opts);
    qemu_add_opts(&qemu_fw_cfg_opts);
    qemu_add_opts(&qemu_action_opts);
    module_call_init(MODULE_INIT_OPTS);
```

- [ ] 参数是如何注入到其中的 ?
```c
typedef struct QemuOptDesc {
    const char *name;
    enum QemuOptType type;
    const char *help;
    const char *def_value_str;
} QemuOptDesc;

struct QemuOptsList {
    const char *name;
    const char *implied_opt_name;
    bool merge_lists;  /* Merge multiple uses of option into a single list? */
    QTAILQ_HEAD(, QemuOpts) head;
    QemuOptDesc desc[];
};

struct QemuOpt {
    char *name;
    char *str;

    const QemuOptDesc *desc;
    union {
        bool boolean;
        uint64_t uint;
    } value;

    QemuOpts     *opts;
    QTAILQ_ENTRY(QemuOpt) next;
};

struct QemuOpts {
    char *id;
    QemuOptsList *list;
    Location loc;
    QTAILQ_HEAD(, QemuOpt) head;
    QTAILQ_ENTRY(QemuOpts) next;
};
```

- qemu_add_opts 将 QemuOptsList 添加到数组中间 vm_config_groups

所有的 QEMUOption 的定义在一个全局变量中间，在 qemu_init 的一个巨大的 for(;;) 中 因为参数都是 -cpu 之类的，可以将这些参数一行行的分离开

```c
static const QEMUOption qemu_options[] = {
    { "h", 0, QEMU_OPTION_h, QEMU_ARCH_ALL },
#define QEMU_OPTIONS_GENERATE_OPTIONS
#include "qemu-options-wrapper.h"
    { NULL },
};
```

经过了 opts_parse 之后，只是将字符串解析成为了 QemuOpt 而已

而之后的使用直接进行在链表中间查询就可以了

```c
/*
>>> bt
#0  opts_do_parse (opts=0x5555565cfe30, params=0x7fffffffddc5 "file=/home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2,format=qcow2", firstname=0x0, prepend=false,
 warn_on_flag=true, help_wanted=0x0, errp=0x7fffffffd630) at ../util/qemu-option.c:816
#1  0x0000555555d20823 in opts_parse (list=<optimized out>, params=0x7fffffffddc5 "file=/home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2,format=qcow2", permit_ab
brev=<optimized out>, defaults=<optimized out>, warn_on_flag=<optimized out>, help_wanted=0x0, errp=0x7fffffffd630) at ../util/qemu-option.c:920
#2  0x0000555555d20b76 in qemu_opts_parse_noisily (list=0x55555648dae0 <qemu_drive_opts>, params=<optimized out>, permit_abbrev=permit_abbrev@entry=false) at ../util/qe
mu-option.c:957
#3  0x0000555555bf1b42 in drive_def (optstr=<optimized out>) at ../blockdev.c:203
#4  0x0000555555baf4d2 in qemu_init (argc=18, argv=0x7fffffffd8d8, envp=<optimized out>) at ../softmmu/vl.c:2735
#5  0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
```
