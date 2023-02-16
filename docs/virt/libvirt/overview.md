# libvirt

- https://wiki.libvirt.org/page/Main_Page
- https://libvirt.org/drvqemu.html
- https://libvirt.org/manpages/virsh.html


## 使用 virsh 安装系统
- https://unix.stackexchange.com/questions/309788/how-to-create-a-vm-from-scratch-with-virsh

## Ctrl+] 从 virsh console 中退出
参考 https://superuser.com/questions/637669/how-to-exit-a-virsh-console-connection

## 环境搭建: https://wiki.libvirt.org/page/UbuntuKVMWalkthrough

- [ ] python-virtinst : 是做什么的?

## 正式操作: https://wiki.libvirt.org/page/QEMUSwitchToLibvirt


## what's the domiain in the libvirt

## patch
- https://libvirt.org/intro.html : not found

## TODO

> 将 source code 的 reading 放到之后

- 使用 Python 开发的吗?

## 使用 libvirt
两个配合使用:
- https://wiki.libvirt.org/page/UbuntuKVMWalkthrough
- https://www.technicalsourcery.net/posts/nixos-in-libvirt/

nixos 专属内容:
- Check if `/dev/kvm` exists, and check the contents of the file opened with `virsh edit <your vm name>`.
This should list /run/libvirt/nix-emulators/qemu-kvm in the <emulator> tag. If both are the case, the VM should be KVM accelerated.

## 重新创建虚拟机的
```sh
kvm : no hardware support
```

## 使用 virsh 来操作 hmp
- https://gist.github.com/orimanabu/815fc2453966f50f5d5281ea58b0058e

## 使用 virsh 操作 qmp

## 使用 virsh 安装系统
```sh
virt-install  \
  --name martins3 \
  --memory 1024             \
  --vcpus=2,maxvcpus=4      \
  --cpu host                \
  --disk size=2,format=qcow2  \
  --network user            \
  --virt-type kvm \
  --cdrom $HOME/Downloads/arch-linux_install.iso
```

## virsh 基本使用
```txt
 Domain Management (help keyword 'domain')
    attach-device                  attach device from an XML file
    attach-disk                    attach disk device
    attach-interface               attach network interface
    autostart                      autostart a domain
    blkdeviotune                   Set or query a block device I/O tuning parameters.
    blkiotune                      Get or set blkio parameters
    blockcommit                    Start a block commit operation.
    blockcopy                      Start a block copy operation.
    blockjob                       Manage active block operations
    blockpull                      Populate a disk from its backing image.
    blockresize                    Resize block device of domain.
    change-media                   Change media of CD or floppy drive
    console                        connect to the guest console
    cpu-stats                      show domain cpu statistics
    create                         create a domain from an XML file
    define                         define (but don't start) a domain from an XML file
    desc                           show or set domain's description or title
    destroy                        destroy (stop) a domain
    detach-device                  detach device from an XML file
    detach-device-alias            detach device from an alias
    detach-disk                    detach disk device
    detach-interface               detach network interface
    domdisplay                     domain display connection URI
    domfsfreeze                    Freeze domain's mounted filesystems.
    domfsthaw                      Thaw domain's mounted filesystems.
    domfsinfo                      Get information of domain's mounted filesystems.
    domfstrim                      Invoke fstrim on domain's mounted filesystems.
    domhostname                    print the domain's hostname
    domid                          convert a domain name or UUID to domain id
    domif-setlink                  set link state of a virtual interface
    domiftune                      get/set parameters of a virtual interface
    domjobabort                    abort active domain job
    domjobinfo                     domain job information
    domlaunchsecinfo               Get domain launch security info
    domsetlaunchsecstate           Set domain launch security state
    domname                        convert a domain id or UUID to domain name
    domrename                      rename a domain
    dompmsuspend                   suspend a domain gracefully using power management functions
    dompmwakeup                    wakeup a domain from pmsuspended state
    domuuid                        convert a domain name or id to domain UUID
    domxml-from-native             Convert native config to domain XML
    domxml-to-native               Convert domain XML to native config
    dump                           dump the core of a domain to a file for analysis
    dumpxml                        domain information in XML
    edit                           edit XML configuration for a domain
    event                          Domain Events
    get-user-sshkeys               list authorized SSH keys for given user (via agent)
    inject-nmi                     Inject NMI to the guest
    iothreadinfo                   view domain IOThreads
    iothreadpin                    control domain IOThread affinity
    iothreadadd                    add an IOThread to the guest domain
    iothreadset                    modifies an existing IOThread of the guest domain
    iothreaddel                    delete an IOThread from the guest domain
    send-key                       Send keycodes to the guest
    send-process-signal            Send signals to processes
    lxc-enter-namespace            LXC Guest Enter Namespace
    managedsave                    managed save of a domain state
    managedsave-remove             Remove managed save of a domain
    managedsave-edit               edit XML for a domain's managed save state file
    managedsave-dumpxml            Domain information of managed save state file in XML
    managedsave-define             redefine the XML for a domain's managed save state file
    memtune                        Get or set memory parameters
    perf                           Get or set perf event
    metadata                       show or set domain's custom XML metadata
    migrate                        migrate domain to another host
    migrate-setmaxdowntime         set maximum tolerable downtime
    migrate-getmaxdowntime         get maximum tolerable downtime
    migrate-compcache              get/set compression cache size
    migrate-setspeed               Set the maximum migration bandwidth
    migrate-getspeed               Get the maximum migration bandwidth
    migrate-postcopy               Switch running migration from pre-copy to post-copy
    numatune                       Get or set numa parameters
    qemu-attach                    QEMU Attach
    qemu-monitor-command           QEMU Monitor Command
    qemu-monitor-event             QEMU Monitor Events
    qemu-agent-command             QEMU Guest Agent Command
    guest-agent-timeout            Set the guest agent timeout
    reboot                         reboot a domain
    reset                          reset a domain
    restore                        restore a domain from a saved state in a file
    resume                         resume a domain
    save                           save a domain state to a file
    save-image-define              redefine the XML for a domain's saved state file
    save-image-dumpxml             saved state domain information in XML
    save-image-edit                edit XML for a domain's saved state file
    schedinfo                      show/set scheduler parameters
    screenshot                     take a screenshot of a current domain console and store it into a file
    set-lifecycle-action           change lifecycle actions
    set-user-sshkeys               manipulate authorized SSH keys file for given user (via agent)
    set-user-password              set the user password inside the domain
    setmaxmem                      change maximum memory limit
    setmem                         change memory allocation
    setvcpus                       change number of virtual CPUs
    shutdown                       gracefully shutdown a domain
    start                          start a (previously defined) inactive domain
    suspend                        suspend a domain
    ttyconsole                     tty console
    undefine                       undefine a domain
    update-device                  update device from an XML file
    update-memory-device           update memory device of a domain
    vcpucount                      domain vcpu counts
    vcpuinfo                       detailed domain vcpu information
    vcpupin                        control or query domain vcpu affinity
    emulatorpin                    control or query domain emulator affinity
    vncdisplay                     vnc display
    guestvcpus                     query or modify state of vcpu in the guest (via agent)
    setvcpu                        attach/detach vcpu or groups of threads
    domblkthreshold                set the threshold for block-threshold event for a given block device or it's backing chain element
    guestinfo                      query information about the guest (via agent)
    domdirtyrate-calc              Calculate a vm's memory dirty rate

 Domain Monitoring (help keyword 'monitor')
    domblkerror                    Show errors on block devices
    domblkinfo                     domain block device size information
    domblklist                     list all domain blocks
    domblkstat                     get device block stats for a domain
    domcontrol                     domain control interface state
    domif-getlink                  get link state of a virtual interface
    domifaddr                      Get network interfaces' addresses for a running domain
    domiflist                      list all domain virtual interfaces
    domifstat                      get network interface stats for a domain
    dominfo                        domain information
    dommemstat                     get memory statistics for a domain
    domstate                       domain state
    domstats                       get statistics about one or multiple domains
    domtime                        domain time
    list                           list domains

 Host and Hypervisor (help keyword 'host')
    allocpages                     Manipulate pages pool size
    capabilities                   capabilities
    cpu-baseline                   compute baseline CPU
    cpu-compare                    compare host CPU with a CPU described by an XML file
    cpu-models                     CPU models
    domcapabilities                domain capabilities
    freecell                       NUMA free memory
    freepages                      NUMA free pages
    hostname                       print the hypervisor hostname
    hypervisor-cpu-baseline        compute baseline CPU usable by a specific hypervisor
    hypervisor-cpu-compare         compare a CPU with the CPU created by a hypervisor on the host
    maxvcpus                       connection vcpu maximum
    node-memory-tune               Get or set node memory parameters
    nodecpumap                     node cpu map
    nodecpustats                   Prints cpu stats of the node.
    nodeinfo                       node information
    nodememstats                   Prints memory stats of the node.
    nodesevinfo                    node SEV information
    nodesuspend                    suspend the host node for a given time duration
    sysinfo                        print the hypervisor sysinfo
    uri                            print the hypervisor canonical URI
    version                        show version

 Checkpoint (help keyword 'checkpoint')
    checkpoint-create              Create a checkpoint from XML
    checkpoint-create-as           Create a checkpoint from a set of args
    checkpoint-delete              Delete a domain checkpoint
    checkpoint-dumpxml             Dump XML for a domain checkpoint
    checkpoint-edit                edit XML for a checkpoint
    checkpoint-info                checkpoint information
    checkpoint-list                List checkpoints for a domain
    checkpoint-parent              Get the name of the parent of a checkpoint

 Interface (help keyword 'interface')
    iface-begin                    create a snapshot of current interfaces settings, which can be later committed (iface-commit) or restored (iface-rollback)
    iface-bridge                   create a bridge device and attach an existing network device to it
    iface-commit                   commit changes made since iface-begin and free restore point
    iface-define                   define an inactive persistent physical host interface or modify an existing persistent one from an XML file
    iface-destroy                  destroy a physical host interface (disable it / "if-down")
    iface-dumpxml                  interface information in XML
    iface-edit                     edit XML configuration for a physical host interface
    iface-list                     list physical host interfaces
    iface-mac                      convert an interface name to interface MAC address
    iface-name                     convert an interface MAC address to interface name
    iface-rollback                 rollback to previous saved configuration created via iface-begin
    iface-start                    start a physical host interface (enable it / "if-up")
    iface-unbridge                 undefine a bridge device after detaching its device(s)
    iface-undefine                 undefine a physical host interface (remove it from configuration)

 Network Filter (help keyword 'filter')
    nwfilter-define                define or update a network filter from an XML file
    nwfilter-dumpxml               network filter information in XML
    nwfilter-edit                  edit XML configuration for a network filter
    nwfilter-list                  list network filters
    nwfilter-undefine              undefine a network filter
    nwfilter-binding-create        create a network filter binding from an XML file
    nwfilter-binding-delete        delete a network filter binding
    nwfilter-binding-dumpxml       network filter information in XML
    nwfilter-binding-list          list network filter bindings

 Networking (help keyword 'network')
    net-autostart                  autostart a network
    net-create                     create a network from an XML file
    net-define                     define an inactive persistent virtual network or modify an existing persistent one from an XML file
    net-destroy                    destroy (stop) a network
    net-dhcp-leases                print lease info for a given network
    net-dumpxml                    network information in XML
    net-edit                       edit XML configuration for a network
    net-event                      Network Events
    net-info                       network information
    net-list                       list networks
    net-name                       convert a network UUID to network name
    net-start                      start a (previously defined) inactive network
    net-undefine                   undefine a persistent network
    net-update                     update parts of an existing network's configuration
    net-uuid                       convert a network name to network UUID
    net-port-list                  list network ports
    net-port-create                create a network port from an XML file
    net-port-dumpxml               network port information in XML
    net-port-delete                delete the specified network port

 Node Device (help keyword 'nodedev')
    nodedev-create                 create a device defined by an XML file on the node
    nodedev-destroy                destroy (stop) a device on the node
    nodedev-detach                 detach node device from its device driver
    nodedev-dumpxml                node device details in XML
    nodedev-list                   enumerate devices on this host
    nodedev-reattach               reattach node device to its device driver
    nodedev-reset                  reset node device
    nodedev-event                  Node Device Events
    nodedev-define                 Define a device by an xml file on a node
    nodedev-undefine               Undefine an inactive node device
    nodedev-start                  Start an inactive node device
    nodedev-autostart              autostart a defined node device
    nodedev-info                   node device information

 Secret (help keyword 'secret')
    secret-define                  define or modify a secret from an XML file
    secret-dumpxml                 secret attributes in XML
    secret-event                   Secret Events
    secret-get-value               Output a secret value
    secret-list                    list secrets
    secret-set-value               set a secret value
    secret-undefine                undefine a secret

 Snapshot (help keyword 'snapshot')
    snapshot-create                Create a snapshot from XML
    snapshot-create-as             Create a snapshot from a set of args
    snapshot-current               Get or set the current snapshot
    snapshot-delete                Delete a domain snapshot
    snapshot-dumpxml               Dump XML for a domain snapshot
    snapshot-edit                  edit XML for a snapshot
    snapshot-info                  snapshot information
    snapshot-list                  List snapshots for a domain
    snapshot-parent                Get the name of the parent of a snapshot
    snapshot-revert                Revert a domain to a snapshot

 Backup (help keyword 'backup')
    backup-begin                   Start a disk backup of a live domain
    backup-dumpxml                 Dump XML for an ongoing domain block backup job

 Storage Pool (help keyword 'pool')
    find-storage-pool-sources-as   find potential storage pool sources
    find-storage-pool-sources      discover potential storage pool sources
    pool-autostart                 autostart a pool
    pool-build                     build a pool
    pool-create-as                 create a pool from a set of args
    pool-create                    create a pool from an XML file
    pool-define-as                 define a pool from a set of args
    pool-define                    define an inactive persistent storage pool or modify an existing persistent one from an XML file
    pool-delete                    delete a pool
    pool-destroy                   destroy (stop) a pool
    pool-dumpxml                   pool information in XML
    pool-edit                      edit XML configuration for a storage pool
    pool-info                      storage pool information
    pool-list                      list pools
    pool-name                      convert a pool UUID to pool name
    pool-refresh                   refresh a pool
    pool-start                     start a (previously defined) inactive pool
    pool-undefine                  undefine an inactive pool
    pool-uuid                      convert a pool name to pool UUID
    pool-event                     Storage Pool Events
    pool-capabilities              storage pool capabilities

 Storage Volume (help keyword 'volume')
    vol-clone                      clone a volume.
    vol-create-as                  create a volume from a set of args
    vol-create                     create a vol from an XML file
    vol-create-from                create a vol, using another volume as input
    vol-delete                     delete a vol
    vol-download                   download volume contents to a file
    vol-dumpxml                    vol information in XML
    vol-info                       storage vol information
    vol-key                        returns the volume key for a given volume name or path
    vol-list                       list vols
    vol-name                       returns the volume name for a given volume key or path
    vol-path                       returns the volume path for a given volume name or key
    vol-pool                       returns the storage pool for a given volume key or path
    vol-resize                     resize a vol
    vol-upload                     upload file contents to a volume
    vol-wipe                       wipe a vol

 Virsh itself (help keyword 'virsh')
    cd                             change the current directory
    echo                           echo arguments. Used for internal testing.
    exit                           quit this interactive terminal
    help                           print help
    pwd                            print the current directory
    quit                           quit this interactive terminal
    connect                        (re)connect to hypervisor
```

## 问题
- 什么叫做 storage pool ?

## 研究下，如何让 libvirt 来替代现在的操作 qemu 的管理
- 打开 memory reporting 机制；
- 开启多个虚拟机。
- 使用 Rust 管理代码。
- 理解其中的网络部署。
- 如何让 virsh 构建一个网络，让本地构建虚拟机迁移。

- 在 ubuntu 上测试吧?


## virsh 观测 memory

1. [root@node-67-81 18:02:04 ~]$virsh dommemstat edab2f44-081c-4031-b4a6-ad064789ad67

2. 使用 qmp
```txt
domain=0de88a46-9331-492e-b978-7c313f13bc6b
virsh qemu-monitor-command $domain  '{"execute": "query-status"}' --pretty
virsh qemu-monitor-command $domain --hmp 'info balloon'
virsh qemu-monitor-command $domain '{ "execute": "qom-list", "arguments": { "path": "/machine/peripheral" } }'
virsh qemu-monitor-command $domain '{ "execute": "qom-set", "arguments": { "path": "/machine/peripheral/balloon0", "property": "guest-stats-polling-interval", "value": 2 } }'
virsh qemu-monitor-command $domain '{ "execute": "qom-get", "arguments": { "path": "/machine/peripheral/balloon0", "property": "guest-stats" } }'
virsh qemu-monitor-command $domain --hmp 'info balloon'
virsh qemu-monitor-command $domain --hmp 'balloon 4000'
```

> 观察 libvirt 的代码，cmdDomMemStat 和 qemuMonitorJSONGetMemoryStats 中 VIR_DOMAIN_MEMORY_STAT_USABLE 这个宏的使用，那么就是 avaliable 的含义了

| 作用        | 说明                |
|-------------|---------------------|
| actual      | QEMU 参数配置的内存 |
| swap_in     |                     |
| swap_out    |                     |
| major_fault |                     |
| minor_fault |                     |
| unused      | MemFree             |
| available   | MemTotal            |
| usable      | MemAvailable        |
| last_update |                     |
| disk_caches | Buffers + Cached    |
| rss         | /proc/$qemu_pid/status | grep RSS                     |

理解下各个字段的含义
```txt
{
  "return": {
    "stats": {
      "stat-swap-out": 0,
      "stat-available-memory": 2083450880, # 2034620
      "stat-free-memory": 999878656, # 976444
      "stat-minor-faults": 482351294,
      "stat-major-faults": 5083,
      "stat-total-memory": 3241218048,
      "stat-swap-in": 0,
      "stat-disk-caches": 1242247168 # 1213132
    },
    "last-update": 1676350443
  },
  "id": "libvirt-23424"
}

actual 4194304
swap_in 0
swap_out 0
major_fault 5083
minor_fault 482351294
unused 976444
available 3165252
usable 2034620
last_update 1676350443
disk_caches 1213132
rss 67288
```

```txt
MemTotal:        3165252 kB
MemFree:          980324 kB
MemAvailable:    2032100 kB
Buffers:          312624 kB
Cached:           895964 kB
SwapCached:            0 kB
Active:           559984 kB
Inactive:         981104 kB
Active(anon):       1212 kB
Inactive(anon):   332704 kB
Active(file):     558772 kB
Inactive(file):   648400 kB
Unevictable:       29272 kB
Mlocked:           27736 kB
SwapTotal:       4194300 kB
SwapFree:        4194300 kB
Dirty:               276 kB
Writeback:             0 kB
AnonPages:        361852 kB
Mapped:           338476 kB
Shmem:              2852 kB
KReclaimable:      95396 kB
Slab:             217388 kB
SReclaimable:      95396 kB
SUnreclaim:       121992 kB
KernelStack:       10912 kB
PageTables:         5840 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:     5776924 kB
Committed_AS:    3076652 kB
VmallocTotal:   34359738367 kB
VmallocUsed:       89020 kB
VmallocChunk:          0 kB
Percpu:           262080 kB
HardwareCorrupted:     0 kB
AnonHugePages:         0 kB
ShmemHugePages:        0 kB
ShmemPmdMapped:        0 kB
FileHugePages:         0 kB
FilePmdMapped:         0 kB
HugePages_Total:       0
HugePages_Free:        0
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
Hugetlb:               0 kB
DirectMap4k:     1498984 kB
DirectMap2M:     2695168 kB
DirectMap1G:     2097152 kB
```

## vish dommemstat 是如何实现的
snoopexec 中
```txt
virtqemud        659083  2800      0 /home/martins3/core/libvirt/build/src/virtqemud --timeout=120
qemu-system-x86  659104  659083    0 /run/current-system/sw/bin/qemu-system-x86_64 -S -no-user-config -nodefaults -nographic -machine none,accel=kvm:tcg -qmp unix:/home/martins3/.config/libvirt/qemu/lib/qmp-LEY2Z1/qmp.monitor,server=on,wait=off -pidfile /home/martins3/.config/libvirt/qemu/lib/qmp-LEY2Z1/qmp.pid -daemonize
.qemu-system-x8  659104  659083    0 /nix/store/lfldwcwazg4qpnb9nwps4nvlxab6zkmk-qemu-7.1.0/bin/.qemu-system-x86_64-wrapped -S -no-user-config -nodefaults -nographic -machine none,accel=kvm:tcg -qmp unix:/home/martins3/.config/libvirt/qemu/lib/qmp-LEY2Z1/qmp.monitor,server=on,wait=off -pidfile /home/martins3/.config/libvirt/qemu/lib/qmp-LEY2Z1/qmp.pid -daemonize
qemu-system-x86  659115  659083    0 /run/current-system/sw/bin/qemu-system-x86_64 -S -no-user-config -nodefaults -nographic -machine none,accel=tcg -qmp unix:/home/martins3/.config/libvirt/qemu/lib/qmp-99VWZ1/qmp.monitor,server=on,wait=off -pidfile /home/martins3/.config/libvirt/qemu/lib/qmp-99VWZ1/qmp.pid -daemonize
.qemu-system-x8  659115  659083    0 /nix/store/lfldwcwazg4qpnb9nwps4nvlxab6zkmk-qemu-7.1.0/bin/.qemu-system-x86_64-wrapped -S -no-user-config -nodefaults -nographic -machine none,accel=tcg -qmp unix:/home/martins3/.config/libvirt/qemu/lib/qmp-99VWZ1/qmp.monitor,server=on,wait=off -pidfile /home/martins3/.config/libvirt/qemu/lib/qmp-99VWZ1/qmp.pid -daemonize
swtpm_setup      659127  659083    0 /home/martins3/.nix-profile/bin/swtpm_setup --print-capabilities
swtpm            659128  659127    0 /home/martins3/.nix-profile/bin/swtpm socket
swtpm            659129  659127    0 /home/martins3/.nix-profile/bin/swtpm socket
```
实际上调用 remoteConnectGetDomainCapabilities
