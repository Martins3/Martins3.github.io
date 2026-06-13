# qmp 和 hmp

## 基本使用
- https://www.qemu.org/docs/master/devel/writing-monitor-commands.html
- https://wiki.qemu.org/Documentation/QMP
- https://www.qemu.org/docs/master/devel/writing-monitor-commands.html#writing-a-debugging-aid-returning-unstructured-text

- https://gist.github.com/rgl/dc38c6875a53469fdebb2e9c0a220c6c

## qmp shell
- https://wiki.qemu.org/Documentation/QMP

qmp shell 常见命令:
1. query-cpu-definitions


## qom-get

```json
{ "execute": "qom-get",
             "arguments": { "path": "/machine/peripheral/balloon0",
             "property": "guest-stats" } }
```

- _start
  - __libc_start_main_impl
    - __libc_start_call_main
      - qemu_default_main
        - qemu_main_loop
          - main_loop_wait
            - os_host_main_loop_wait
              - glib_pollfds_poll
                - g_main_context_dispatch
                  - aio_ctx_dispatch
                    - aio_dispatch
                      - aio_bh_poll
                        - aio_bh_call
                          - do_qmp_dispatch_bh
                            - qmp_marshal_qom_get
                              - qmp_qom_get
                                - object_property_get_qobject
                                  - object_property_get
                                    - property_get_alias
                                      - object_property_get
                                        - balloon_stats_get_all

# hmp

- [ ] 可以阅读的文档:
这里描述在 graphic 和 non-graphic 的模式下访问 HMI 的方法，并且说明了从 HMI 中间如何获取各种信息
https://web.archive.org/web/20180104171638/http://nairobi-embedded.org/qemu_monitor_console.html


## 源码分析
- hmp_info_balloon

- _start
  - __libc_start_main_impl
    - __libc_start_call_main
      - qemu_default_main
        - qemu_main_loop
          - main_loop_wait
            - os_host_main_loop_wait
              - glib_pollfds_poll
                - g_main_context_dispatch
                  - tcp_chr_read
                    - monitor_read
                      - readline_handle_byte
                        - monitor_command_cb
                          - handle_hmp_command
                            - handle_hmp_command_exec
                              - handle_hmp_command_exec
                                - hmp_info_balloon

```c
void hmp_info_balloon(Monitor *mon, const QDict *qdict)
{
    BalloonInfo *info;
    Error *err = NULL;

    info = qmp_query_balloon(&err);
    if (hmp_handle_error(mon, err)) {
        return;
    }

    monitor_printf(mon, "balloon: actual=%" PRId64 "\n", info->actual >> 20);

    qapi_free_BalloonInfo(info);
}
```

- [ ] /home/martins3/core/qemu/build/hmp-commands-info.h 是如何生成的
- [ ] /home/martins3/core/qemu/build/qapi/qapi-commands-machine.h 中包含了 qmp_query_balloon

## 总结一些和 qobject 纠缠在一起的功能

- object_register_sugar_prop

```c
static void qemu_process_sugar_options(void)
{
    if (mem_prealloc) {
        QObject *smp = qdict_get(machine_opts_dict, "smp");
        if (smp && qobject_type(smp) == QTYPE_QDICT) {
            QObject *cpus = qdict_get(qobject_to(QDict, smp), "cpus");
            if (cpus && qobject_type(cpus) == QTYPE_QSTRING) {
                const char *val = qstring_get_str(qobject_to(QString, cpus));
                object_register_sugar_prop("memory-backend", "prealloc-threads",
                                           val, false);
            }
        }
        object_register_sugar_prop("memory-backend", "prealloc", "on", false);
    }
}
```

## 如何快速定位到代码
hmp 中提供了一个 mce 命令，如何找到对应的实现:

hmp_mce 直接搜索 hmp_mce 即可

## 分析下
https://qemu.readthedocs.io/en/v8.1.5/interop/qemu-qmp-ref.html

## hmp 中的 sendkey 是如何实现的

此外，相关配置:
https://vncdotool.readthedocs.io/en/latest/usage.html

arm 上面没办法用，难道是这个问题?
https://lists.gnu.org/archive/html/qemu-devel/2018-02/msg06218.html

## qmp shell 中的命令可以看下

```txt
x-query-irq                        x-query-ramblock                   x-query-virtio-queue-element
x-query-jit                        x-query-roms                       x-query-virtio-queue-status
x-query-numa                       x-query-usb                        x-query-virtio-status
x-query-opcount                    x-query-virtio                     x-query-virtio-vhost-queue-status
(QEMU) x-query-irq
{"return": {"human-readable-text": "IRQ statistics for kvm-ioapic:\n 0: 13\n 1: 11\n 3: 2\n 4: 2\n 6: 3\n 8: 1\n10: 156\n12: 15\nIRQ statistics for kvm-i8259:\n 0: 13\n 1: 11\n 3: 2\n 4: 2\n 6: 3\n 8: 1\n10: 156\n12: 15\n"}}
(QEMU) x-query-jit
{"error": {"class": "GenericError", "desc": "JIT information is only available with accel=tcg"}}
```

## hmp 一共支持那些命令
<!-- fe7aeb50-36e7-4909-bf2f-6bd9cb5d7275 -->

```txt
announce_self [interfaces] [id] -- Trigger GARP/RARP announcements
balloon target -- request VM to change its memory allocation (in MB)
block_job_cancel [-f] device -- stop an active background block operation (use -f
                         if you want to abort the operation immediately
                         instead of keep running until data is in sync)
block_job_complete device -- stop an active background block operation
block_job_pause device -- pause an active background block operation
block_job_resume device -- resume a paused background block operation
block_job_set_speed device speed -- set maximum speed for a background block operation
block_resize device size -- resize a block image
block_set_io_throttle device bps bps_rd bps_wr iops iops_rd iops_wr -- change I/O throttle limits for a block drive
block_stream device [speed [base]] -- copy data from a backing file into a block device
boot_set bootdevice -- define new values for the boot device list
calc_dirty_rate [-r] [-b] second [sample_pages_per_GB] -- start a round of guest dirty rate measurement (using -r to
                         specify dirty ring as the method of calculation and
                         -b to specify dirty bitmap as method of calculation)
cancel_vcpu_dirty_limit [cpu_index] -- cancel dirty page rate limit, use cpu_index to cancel
                                         limit on a specified virtual cpu
change device [-f] filename [format [read-only-mode]] -- change a removable medium, optional format, use -f to force the operation
chardev-add args -- add chardev
chardev-change id args -- change chardev
chardev-remove id -- remove chardev
chardev-send-break id -- send a break on chardev
client_migrate_info protocol hostname port tls-port cert-subject -- set migration information for remote display
closefd closefd name -- close a file descriptor previously passed via SCM rights
commit device|all -- commit changes to the disk images (if -snapshot is used) or backing files
cont|c  -- resume emulation
cpu index -- set the default CPU
delvm tag -- delete a VM snapshot from its tag
device_add driver[,prop=value][,...] -- add device, like -device on the command line
device_del device -- remove device
drive_add [-n] [[<domain>:]<bus>:]<slot>
[file=file][,if=type][,bus=n]
[,unit=m][,media=d][,index=i]
[,snapshot=on|off][,cache=on|off]
[,readonly=on|off][,copy-on-read=on|off] -- add drive to PCI storage controller
drive_backup [-n] [-f] [-c] device target [format] -- initiates a point-in-time
                        copy for a device. The device's contents are
                        copied to the new image file, excluding data that
                        is written after the command is started.
                        The -n flag requests QEMU to reuse the image found
                        in new-image-file, instead of recreating it from scratch.
                        The -f flag requests QEMU to copy the whole disk,
                        so that the result does not need a backing file.
                        The -c flag requests QEMU to compress backup data
                        (if the target format supports it).

drive_del device -- remove host block device
drive_mirror [-n] [-f] device target [format] -- initiates live storage
                        migration for a device. The device's contents are
                        copied to the new image file, including data that
                        is written after the command is started.
                        The -n flag requests QEMU to reuse the image found
                        in new-image-file, instead of recreating it from scratch.
                        The -f flag requests QEMU to copy the whole disk,
                        so that the result does not need a backing file.

dump-guest-memory [-p] [-d] [-z|-l|-s|-w] [-R] filename [begin length] -- dump guest memory into file 'filename'.
                        -p: do paging to get guest's memory mapping.
                        -d: return immediately (do not wait for completion).
                        -z: dump in kdump-compressed format, with zlib compression.
                        -l: dump in kdump-compressed format, with lzo compression.
                        -s: dump in kdump-compressed format, with snappy compression.
                        -R: when using kdump (-z, -l, -s), use raw rather than makedumpfile-flattened
                            format
                        -w: dump in Windows crashdump format (can be used instead of ELF-dump converting),
                            for Windows x86 and x64 guests with vmcoreinfo driver only.
                        begin: the starting physical address.
                        length: the memory size, in bytes.
dumpdtb filename -- dump the FDT in dtb format to 'filename'
eject [-f] device -- eject a removable medium (use -f to force it)
exit_preconfig  -- exit the preconfig state
expire_password protocol time [-d display] -- set spice/vnc password expire-time
gdbserver [device] -- start gdbserver on given device (default 'tcp::1234'), stop with 'none'
getfd getfd name -- receive a file descriptor via SCM rights and assign it a name
gpa2hpa addr -- print the host physical address corresponding to a guest physical address
gpa2hva addr -- print the host virtual address corresponding to a guest physical address
gva2gpa addr -- print the guest physical address corresponding to a guest virtual address
help|? [cmd] -- show the help
hostfwd_add [netdev_id] [tcp|udp]:[hostaddr]:hostport-[guestaddr]:guestport -- redirect TCP or UDP connections from host to guest (requires -net user)
hostfwd_remove [netdev_id] [tcp|udp]:[hostaddr]:hostport -- remove host-to-guest TCP or UDP redirection
i /fmt addr -- I/O port read
info [subcommand] -- show various information about the system state
loadvm tag -- restore a VM snapshot from its tag
log item1[,...] -- activate logging of the specified items
logfile filename -- output logs to 'filename'
mce [-b] cpu bank status mcgstatus addr misc -- inject a MCE on the given CPU [and broadcast to other CPUs with -b option]
memsave addr size file -- save to disk virtual memory dump starting at 'addr' of size 'size'
migrate [-d] [-r] uri -- migrate to URI (using -d to not wait for completion)
                         -r to resume a paused postcopy migration
migrate_cancel  -- cancel the current VM migration
migrate_continue state -- Continue migration from the given paused state
migrate_incoming uri -- Continue an incoming migration from an -incoming defer
migrate_pause  -- Pause an ongoing migration (postcopy-only)
migrate_recover uri -- Continue a paused incoming postcopy migration
migrate_set_capability capability state -- Enable/Disable the usage of a capability for migration
migrate_set_parameter parameter value -- Set the parameter for migration
migrate_start_postcopy  -- Followup to a migration command to switch the migration to postcopy mode. The postcopy-ram capability must be set on both source and destination before the original migration command .
mouse_button state -- change mouse button state (1=L, 2=M, 4=R)
mouse_move dx dy [dz] -- send mouse move events
mouse_set index -- set which mouse device receives events
nbd_server_add nbd_server_add [-w] device [name] -- export a block device via NBD
nbd_server_remove nbd_server_remove [-f] name -- remove an export previously exposed via NBD
nbd_server_start nbd_server_start [-a] [-w] host:port -- serve block devices on the given host and port
nbd_server_stop nbd_server_stop -- stop serving block devices using the NBD protocol
netdev_add [user|tap|socket|stream|dgram|vde|bridge|hubport|netmap|vhost-user|passt],id=str[,prop=value][,...] -- add host network device
netdev_del id -- remove host network device
nmi  -- inject an NMI
o /fmt addr value -- I/O port write
object_add [qom-type=]type,id=str[,prop=value][,...] -- create QOM object
object_del id -- destroy QOM object
one-insn-per-tb [on|off] -- run emulation with one guest instruction per translation block
pcie_aer_inject_error [-a] [-c] id <error_status> [<tlp header> [<tlp header prefix>]] -- inject pcie aer error
                         -a for advisory non fatal error
                         -c for correctable error
                        <id> = qdev device id
                        <error_status> = error string or 32bit
                        <tlp header> = 32bit x 4
                        <tlp header prefix> = 32bit x 4
pmemsave addr size file -- save to disk physical memory dump starting at 'addr' of size 'size'
print|p /fmt expr -- print expression value (use $reg for CPU register access)
qemu-io [-d] [device] "[command]" -- run a qemu-io command on a block device
                        -d: [device] is a device ID rather than a drive ID or node name
qom-get path property -- print QOM property
qom-list path -- list QOM properties
qom-set [-j] path property value -- set QOM property.
                        -j: the value is specified in json format.
quit|q  -- quit the emulator
replay_break icount -- set breakpoint at the specified instruction count
replay_delete_break  -- remove replay breakpoint
replay_seek icount -- replay execution to the specified instruction count
ringbuf_read device size -- Read from a ring buffer character device
ringbuf_write device data -- Write to a ring buffer character device
savevm tag -- save a VM snapshot. If no tag is provided, a new snapshot is created
screendump filename [-f format] [device [head]] -- save screen from head 'head' of display device 'device'in specified format 'format' as image 'filename'.Currently only 'png' and 'ppm' formats are supported.
sendkey keys [hold_ms] -- send keys to the VM (e.g. 'sendkey ctrl-alt-f1', default hold time=100 ms)
set_link name on|off -- change the link status of a network adapter
set_password protocol password [-d display] [action-if-connected] -- set spice/vnc password
set_vcpu_dirty_limit dirty_rate [cpu_index] -- set dirty page rate limit, use cpu_index to set limit
                                         on a specified virtual cpu
snapshot_blkdev [-n] device [new-image-file] [format] -- initiates a live snapshot
                        of device. If a new image file is specified, the
                        new image file will become the new root image.
                        If format is specified, the snapshot file will
                        be created in that format.
                        The default format is qcow2.  The -n flag requests QEMU
                        to reuse the image found in new-image-file, instead of
                        recreating it from scratch.
snapshot_blkdev_internal device name -- take an internal snapshot of device.
                        The format of the image used by device must
                        support it, such as qcow2.

snapshot_delete_blkdev_internal device name [id] -- delete an internal snapshot of device.
                        If id is specified, qemu will try delete
                        the snapshot matching both id and name.
                        The format of the image used by device must
                        support it, such as qcow2.

stopcapture capture index -- stop capture
stop|s  -- stop emulation
sum addr size -- compute the checksum of a memory region
sync-profile [on|off|reset] -- enable, disable or reset synchronization profiling. With no arguments, prints whether profiling is on or off.
system_powerdown  -- send system power down event
system_reset  -- reset the system
system_wakeup  -- wakeup guest from suspend
trace-event name on|off [vcpu] -- changes status of a specific trace event (vcpu: vCPU to set, default is all)
watchdog_action [reset|shutdown|poweroff|pause|debug|none|inject-nmi] -- change watchdog action
wavcapture path audiodev [frequency [bits [channels]]] -- capture audio to a wave file (default frequency=44100 bits=16 channels=2)
x /fmt addr -- virtual memory dump starting at 'addr'
x_colo_lost_heartbeat  -- Tell COLO that heartbeat is lost,
                        a failover or takeover is needed.
xen-event-inject port -- inject event channel
xen-event-list  -- list event channel state
xp /fmt addr -- physical memory dump starting at 'addr'
```

### info

```txt
(qemu) info chardev
virtiocon0: filename=pty:/dev/pts/6
serial1: filename=pipe
parallel0: filename=vc
gdb: filename=disconnected:tcp:0.0.0.0:1234,server
compat_monitor0: filename=stdio
serial0: filename=pipe
```
如果什么都不配置，结果如下:
```txt
(qemu) info chardev
parallel0: filename=vc
compat_monitor0: filename=stdio
serial0: filename=vc
```

### gpa2hpa
gpa2hpa addr -- print the host physical address corresponding to a guest physical address
gpa2hva addr -- print the host virtual address corresponding to a guest physical address
gva2gpa addr -- print the guest physical address corresponding to a guest virtual address

### hmp 中的 drive_backup 和 drive_mirror 做什么
drive_add     drive_backup  drive_del     drive_mirror

### misc
有趣的功能:
- screendump
- logfile

## 也许有用
给Qemu虚拟机“打信号”：自定义QMP注入SCI中断 - MyStackTrace的文章 - 知乎
https://zhuanlan.zhihu.com/p/1943785816179607321

## hmp info 来看看那些可以观测的内容
<!-- 6150a420-6cd4-4720-80fb-ee982d8ba1d2 -->

```txt
info accel  -- show accelerator info
info balloon  -- show balloon information
info block [-n] [-v] [device] -- show info of one block device or all block devices (-n: show named nodes; -v: show details)
info block-jobs  -- show progress of ongoing block device operations
info blockstats  -- show block device statistics
info capture  -- show capture information
info chardev  -- show the character devices
info cpus  -- show infos for each CPU
info cryptodev  -- show the crypto devices
info dirty_rate  -- show dirty rate information
info dump  -- Display the latest dump status
info history  -- show the command line history
info hotpluggable-cpus  -- Show information about hotpluggable CPUs
info iothreads  -- show iothreads
info irq  -- show the interrupts statistics (if available)
info jit  -- show dynamic compiler info
info kvm  -- show KVM information
info lapic [apic-id] -- show local apic state (apic-id: local apic to read, default is which of current CPU)
info mem  -- show the active virtual memory mappings
info memdev  -- show memory backends
info memory-devices  -- show memory devices
info memory_size_summary  -- show the amount of initially allocated and present hotpluggable (if enabled) memory in bytes.
info mice  -- show which guest mouse is receiving events
info migrate [-a] -- show migration status (-a: all, dump all status)
info migrate_capabilities  -- show current migration capabilities
info migrate_parameters  -- show current migration parameters
info mtree [-f][-d][-o][-D] -- show memory tree (-f: dump flat view for address spaces;-d: dump dispatch tree, valid with -f only);-o: dump region owners/parents;-D: dump disabled regions
info name  -- show the current VM name
info network  -- show the network state
info numa  -- show NUMA information
info pci  -- show PCI info
info pic  -- show PIC state
info qdm  -- show qdev device model list
info qom-tree [path] -- show QOM composition tree
info qtree [-b] -- show device tree (-b: brief, omit properties)
info ramblock  -- Display system ramblock information
info registers [-a|vcpu] -- show the cpu registers (-a: show register info for all cpus; vcpu: specific vCPU to query; show the current CPU's registers if no argument is specified)
info replay  -- show record/replay information
info rocker name -- Show rocker switch
info rocker-of-dpa-flows name [tbl_id] -- Show rocker OF-DPA flow tables
info rocker-of-dpa-groups name [type] -- Show rocker OF-DPA groups
info rocker-ports name -- Show rocker ports
info roms  -- show roms
info sev  -- show SEV information
info sgx  -- show intel SGX information
info snapshots  -- show the currently saved VM snapshots
info stats target [names] [provider] -- show statistics for the given target (vm or vcpu); optionally filter byname (comma-separated list, or * for all) and provider
info status  -- show the current VM status (running|paused)
info sync-profile [-m] [-n] [max] -- show synchronization profiling info, up to max entries (default: 10), sorted by total wait time. (-m: sort by mean wait time; -n: do not coalesce objects with the same call site)
info tlb  -- show virtual to physical memory mappings
info tpm  -- show the TPM device
info trace-events [name] [vcpu] -- show available trace-events & their state (name: event name pattern; vcpu: vCPU to query, default is any)
info usb  -- show guest USB devices
info usbhost  -- show host USB devices
info usernet  -- show user network stack connection states
info uuid  -- show the current VM UUID
info vcpu_dirty_limit  -- show dirty page limit information of all vCPU
info version  -- show the version of QEMU
info virtio  -- List all available virtio devices
info virtio-queue-element path queue [index] -- Display element of a given virtio queue
info virtio-queue-status path queue -- Display status of a given virtio queue
info virtio-status path -- Display status of a given virtio device
info virtio-vhost-queue-status path queue -- Display status of a given vhost queue
info vm-generation-id  -- Show Virtual Machine Generation ID
info vnc  -- show the vnc server status
```

这几个有什么区别?
```txt
info mem  -- show the active virtual memory mappings
info memdev  -- show memory backends
info memory-devices  -- show memory devices
info memory_size_summary  -- show the amount of initially allocated and present hotpluggable (if enabled) memory in bytes.
```

看看这些都是如何使用的:
```txt
info virtio  -- List all available virtio devices
info virtio-queue-element path queue [index] -- Display element of a given virtio queue
info virtio-queue-status path queue -- Display status of a given virtio queue
info virtio-status path -- Display status of a given virtio device
info virtio-vhost-queue-status path queue -- Display status of a given vhost queue
```

其他的有趣的东西:
```txt
info usernet
info vnc # vnc server 总是在 enable 的
```

常看常新的东西:
```txt
info qom-tree [path] -- show QOM composition tree
info qtree [-b] -- show device tree (-b: brief, omit properties)
```

## TODO
```txt
hack/qemu/internals/e1000-2.md:#18 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
hack/qemu/internals/e1000.md:#16 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
hack/acpi/hack-with-qemu.md:#10 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2590
docs/kernel/mm-virtio-balloon.md:- qmp 对外仅仅提供两个功能
docs/qemu/block.md:2. 后面的就是各种 qmp 操作的
docs/qemu/block.md:在 `qmp_transaction` 中的，根据命令来调用这些内容:
docs/qemu/block.md:## qmp ：没办法，不搞的话，dirty bitmap 是没有办法维持生活的
docs/qemu/block.md:- [ ] grep 一下目前对于 qmp 的所有问题，尝试将 qmp 和 qemu option 融合一下
docs/qemu/reset.md:  - `qmp_x_exit_preconfig`
docs/qemu/reset.md:#6  0x0000555555c22788 in qmp_x_exit_preconfig (errp=0x5555567aa610 <error_fatal>) at ../softmmu/vl.c:2602
docs/qemu/migration/yank.md:instances can be called by the 'yank' out-of-band qmp command.
docs/qemu/migration/yank.md:# A yank instance can be yanked with the @yank qmp command to recover from a
docs/qemu/migration/multifd.md:    - `migrate_multifd_channels` : 这个数值是从 qmp 设置的
docs/qemu/migration/migration.md:- `qmp_migrate_incoming` / `qmp_migrate_recover`
docs/qemu/migration/migration.md:- `qmp_migrate`
docs/qemu/migration/migration.md:- `qmp_query_migrate_parameters`
docs/qemu/options.md:-qmp unix:/home/maritns3/core/vn/hack/qemu/x64-e1000/test.socket,server,nowait \
docs/qemu/options.md:[qmp] : [unix:/home/maritns3/core/vn/hack/qemu/x64-e1000/test.socket,server,nowait]
docs/qemu/sh/alpine.sh:  ${arg_qmp} ${arg_vfio} ${arg_smbios} ${arg_scsi}"
docs/qemu/todo-1.md:- [ ] docs/devel/qapi-code-gen.txt 和 qmp 如何工作的，是如何生成的。
docs/qemu/todo-1.md:## qmp
docs/qemu/todo-1.md:- [ ] `qmp_block_commit` 的唯一调用者是如何被生成的。
docs/qemu/todo-1.md:qmp 让 virsh 可以和 qemu 交互
docs/qemu/qom.md:#13 0x0000555555cdaf85 in qmp_x_exit_preconfig (errp=0x5555567a94b0 <error_fatal>) at ../softmmu/vl.c:2600
docs/qemu/qom.md:#18 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2689
docs/qemu/qom.md:#19 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2682
docs/qemu/seabios.md:#13 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
```


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
